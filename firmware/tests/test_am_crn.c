#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "am_crn_types.h"
#include "motion_gating_engine.h"
#include "neuromodulation_engine.h"
#include "erm_hardware_scheduler.h"
#include "am_crn_system.h"
#include "serial_protocol.h"

static void test_motion_gating(void) {
    printf("[TEST] Running test_motion_gating...\n");
    MotionGatingEngine engine;
    motion_gating_init(&engine, NULL);

    // 1. Feed static samples (hand resting on table)
    for (uint32_t t = 0; t < 100; t++) {
        ImuRawSample s = {
            .timestamp_us = t * 10000,
            .ax_g = 0.0f, .ay_g = 0.0f, .az_g = 1.0f,
            .gx_dps = 0.0f, .gy_dps = 0.0f, .gz_dps = 0.0f,
            .status = 0
        };
        motion_gating_process_sample(&engine, &s);
    }
    KinematicFeatures f_still = *motion_gating_get_features(&engine);
    assert(f_still.motion_state == MOTION_STATE_STILL);
    assert(f_still.stillness_score > 0.90f);
    assert(f_still.sensory_gating_factor >= 1.0f && f_still.sensory_gating_factor < 1.05f);

    // 2. Feed dynamic samples (reaching movement)
    for (uint32_t t = 100; t < 150; t++) {
        float phase = (float)(t - 100) * 0.1f;
        ImuRawSample s = {
            .timestamp_us = t * 10000,
            .ax_g = 0.5f * sinf(phase),
            .ay_g = 0.8f * cosf(phase),
            .az_g = 1.0f + 0.6f * sinf(phase * 2.0f),
            .gx_dps = 60.0f, .gy_dps = 80.0f, .gz_dps = 40.0f,
            .status = 0
        };
        motion_gating_process_sample(&engine, &s);
    }
    KinematicFeatures f_move = *motion_gating_get_features(&engine);
    assert(f_move.motion_state == MOTION_STATE_FAST_MOVE || f_move.motion_state == MOTION_STATE_REACH);
    assert(f_move.stillness_score < 0.50f);
    assert(f_move.sensory_gating_factor > 1.30f); // Gating index scaled up!
    printf("       -> Static gating: %.3f, Dynamic gating: %.3f (PASS)\n",
           f_still.sensory_gating_factor, f_move.sensory_gating_factor);
}

static void test_coordinated_reset(void) {
    printf("[TEST] Running test_coordinated_reset...\n");
    NeuromodulationEngine engine;
    neuromod_init(&engine, NULL);

    neuromod_set_mode(&engine, MODE_COORDINATED_RESET, 1000);

    StimulusPulse pulses[NUM_FINGERS];
    uint8_t count = neuromod_generate_pulses(&engine, 1000, 1.0f, pulses, NUM_FINGERS);
    assert(count == NUM_FINGERS);

    // Verify non-overlapping pulses
    for (uint8_t i = 0; i < count - 1; i++) {
        uint32_t current_end = pulses[i].start_time_ms + pulses[i].duration_ms;
        assert(pulses[i + 1].start_time_ms >= current_end);
        assert(pulses[i].target_duty > 0);
        assert(pulses[i].use_overdrive == true);
    }

    // Verify dynamic amplitude scaling under motion gating
    StimulusPulse gated_pulses[NUM_FINGERS];
    neuromod_generate_pulses(&engine, 2500, 1.6f, gated_pulses, NUM_FINGERS);
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        assert(gated_pulses[i].target_duty >= pulses[i].target_duty);
    }
    printf("       -> 5 pulses generated, non-overlapping verified, gating scaling verified (PASS)\n");
}

static void test_scheduler_overdrive_and_safety(void) {
    printf("[TEST] Running test_scheduler_overdrive_and_safety...\n");
    ErmHardwareScheduler sched;
    erm_scheduler_init(&sched, NULL);

    StimulusPulse p = {
        .start_time_ms = 100,
        .duration_ms = 80,
        .finger_index = FINGER_INDEX,
        .target_duty = 100,
        .use_overdrive = true,
        .is_active = true
    };
    bool enq_ok = erm_scheduler_enqueue(&sched, &p);
    assert(enq_ok);

    // 1. At t=105 ms (inside 20ms overdrive window)
    erm_scheduler_tick(&sched, 105);
    uint8_t duties[NUM_FINGERS];
    erm_scheduler_get_duties(&sched, duties);
    // Overdrive = 100 * 1.35 = 135
    assert(duties[FINGER_INDEX] == 135);
    assert(duties[FINGER_THUMB] == 0);

    // 2. At t=130 ms (after overdrive window, steady state)
    erm_scheduler_tick(&sched, 130);
    erm_scheduler_get_duties(&sched, duties);
    assert(duties[FINGER_INDEX] == 100);

    // 3. At t=190 ms (after pulse completion)
    erm_scheduler_tick(&sched, 190);
    erm_scheduler_get_duties(&sched, duties);
    assert(duties[FINGER_INDEX] == 0);

    // 4. Test Safety Watchdog (excessive continuous duration)
    StimulusPulse stuck_pulse = {
        .start_time_ms = 1000,
        .duration_ms = 500, // Exceeds 250 ms limit
        .finger_index = FINGER_MIDDLE,
        .target_duty = 120,
        .use_overdrive = false,
        .is_active = true
    };
    erm_scheduler_enqueue(&sched, &stuck_pulse);
    for (uint32_t t = 1000; t <= 1260; t++) {
        erm_scheduler_tick(&sched, t);
    }
    // Watchdog should trip after 250 ms continuous activation
    assert(erm_scheduler_is_tripped(&sched));
    erm_scheduler_get_duties(&sched, duties);
    assert(duties[FINGER_MIDDLE] == 0); // Forced cutoff!
    printf("       -> Overdrive kick, steady-state, and watchdog cutoff verified (PASS)\n");
}

static void test_staircase_calibration(void) {
    printf("[TEST] Running test_staircase_calibration...\n");
    NeuromodulationEngine engine;
    neuromod_init(&engine, NULL);

    neuromod_calib_start(&engine, 0);
    assert(!neuromod_calib_is_complete(&engine));

    // Simulate 5 fingers responding to staircase
    uint32_t t = 0;
    while (!neuromod_calib_is_complete(&engine) && t < 200) {
        // Mock user perceptual response: feels vibration if duty >= 90
        bool felt = (engine.calib_current_duty >= 90);
        neuromod_calib_submit_response(&engine, felt, t * 100);
        t++;
    }

    assert(neuromod_calib_is_complete(&engine));
    const FingerCalibrationProfile* prof = neuromod_get_calibration(&engine);
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        assert(prof->threshold_duty[i] >= 70 && prof->threshold_duty[i] <= 110);
        assert(prof->target_duty[i] > prof->threshold_duty[i]);
    }
    printf("       -> Calibration completed in %u steps; profiles validated (PASS)\n", t);
}

static void test_full_system_loop(void) {
    printf("[TEST] Running test_full_system_loop (5-second simulation)...\n");
    AmCrnSystem sys;
    am_crn_system_init(&sys);

    am_crn_system_set_mode(&sys, MODE_COORDINATED_RESET, 0);

    uint32_t active_ticks = 0;
    for (uint32_t t_ms = 0; t_ms < 5000; t_ms++) {
        // Feed 100 Hz IMU samples
        if (t_ms % 10 == 0) {
            float phase = (float)t_ms * 0.005f;
            ImuRawSample s = {
                .timestamp_us = t_ms * 1000,
                .ax_g = 0.2f * sinf(phase),
                .ay_g = 0.1f * cosf(phase),
                .az_g = 1.0f + 0.15f * sinf(phase * 1.5f),
                .gx_dps = 20.0f * sinf(phase),
                .gy_dps = 15.0f * cosf(phase),
                .gz_dps = 5.0f,
                .status = 0
            };
            am_crn_system_update_imu(&sys, &s);
        }

        // 1 ms tick
        am_crn_system_tick_1ms(&sys, t_ms);

        uint8_t duties[NUM_FINGERS];
        am_crn_system_get_motor_duties(&sys, duties);
        for (uint8_t i = 0; i < NUM_FINGERS; i++) {
            if (duties[i] > 0) active_ticks++;
        }
    }

    assert(!am_crn_system_is_safety_tripped(&sys));
    assert(active_ticks > 0);
    printf("       -> 5000 ms simulated successfully, %u motor activations, safety intact (PASS)\n", active_ticks);
}

static void test_serial_protocol(void) {
    printf("[TEST] Running test_serial_protocol...\n");
    AmCrnSystem sys;
    am_crn_system_init(&sys);

    // Test command: MODE CR
    bool ok = serial_protocol_parse_line(&sys, "MODE CR", 100);
    assert(ok);
    assert(sys.neuromod_engine.current_mode == MODE_COORDINATED_RESET);

    // Test command: ESTOP
    ok = serial_protocol_parse_line(&sys, "ESTOP", 150);
    assert(ok);
    assert(erm_scheduler_is_tripped(&sys.scheduler));

    // Test command: RESET_SAFETY
    ok = serial_protocol_parse_line(&sys, "RESET_SAFETY", 200);
    assert(ok);
    assert(!erm_scheduler_is_tripped(&sys.scheduler));

    // Test telemetry formatting
    char buf[128];
    int len = serial_protocol_format_telemetry(&sys, buf, sizeof(buf));
    assert(len > 20);
    assert(strstr(buf, "STATUS") != NULL);
    assert(strstr(buf, "safety=OK") != NULL);
    printf("       -> Formatted telemetry: %s", buf);
    printf("       -> Command parsing and telemetry formatting verified (PASS)\n");
}

int main(void) {
    printf("\n=== AM-CRN Firmware Verification Suite ===\n");
    test_motion_gating();
    test_coordinated_reset();
    test_scheduler_overdrive_and_safety();
    test_staircase_calibration();
    test_full_system_loop();
    test_serial_protocol();
    printf("\nALL FIRMWARE TESTS PASSED.\n\n");
    return 0;
}
