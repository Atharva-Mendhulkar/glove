#include "am_crn_system.h"
#include <string.h>

void am_crn_system_init(AmCrnSystem* sys) {
    if (!sys) return;
    memset(sys, 0, sizeof(AmCrnSystem));

    motion_gating_init(&sys->motion_engine, NULL);
    neuromod_init(&sys->neuromod_engine, NULL);
    erm_scheduler_init(&sys->scheduler, NULL);

    sys->is_initialized = true;
}

void am_crn_system_update_imu(AmCrnSystem* sys, const ImuRawSample* sample) {
    if (!sys || !sample || !sys->is_initialized) return;
    motion_gating_process_sample(&sys->motion_engine, sample);
    sys->last_imu_time_ms = sample->timestamp_us / 1000;
}

void am_crn_system_tick_1ms(AmCrnSystem* sys, uint32_t now_ms) {
    if (!sys || !sys->is_initialized) return;
    sys->system_uptime_ms = now_ms;

    // 1. Get current sensory gating factor G(t)
    float gating_factor = motion_gating_get_factor(&sys->motion_engine);

    // 2. Generate new pulses if at a cycle boundary
    StimulusPulse candidate_pulses[NUM_FINGERS];
    uint8_t count = neuromod_generate_pulses(&sys->neuromod_engine, now_ms, gating_factor, candidate_pulses, NUM_FINGERS);

    // 3. Enqueue candidate pulses into hardware scheduler
    for (uint8_t i = 0; i < count; i++) {
        erm_scheduler_enqueue(&sys->scheduler, &candidate_pulses[i]);
    }

    // 4. Run hardware scheduler 1ms tick
    erm_scheduler_tick(&sys->scheduler, now_ms);
}

void am_crn_system_set_mode(AmCrnSystem* sys, NeuromodulationMode mode, uint32_t now_ms) {
    if (!sys) return;
    neuromod_set_mode(&sys->neuromod_engine, mode, now_ms);
}

void am_crn_system_get_motor_duties(const AmCrnSystem* sys, uint8_t out_duties[NUM_FINGERS]) {
    if (!sys) return;
    erm_scheduler_get_duties(&sys->scheduler, out_duties);
}

float am_crn_system_get_gating_factor(const AmCrnSystem* sys) {
    if (!sys) return 1.0f;
    return motion_gating_get_factor(&sys->motion_engine);
}

MotionState am_crn_system_get_motion_state(const AmCrnSystem* sys) {
    if (!sys) return MOTION_STATE_UNKNOWN;
    return motion_gating_get_state(&sys->motion_engine);
}

bool am_crn_system_is_safety_tripped(const AmCrnSystem* sys) {
    if (!sys) return false;
    return erm_scheduler_is_tripped(&sys->scheduler);
}
