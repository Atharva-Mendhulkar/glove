#include "erm_hardware_scheduler.h"
#include <string.h>

void erm_scheduler_init(ErmHardwareScheduler* sched, const ErmSchedulerConfig* config) {
    if (!sched) return;
    memset(sched, 0, sizeof(ErmHardwareScheduler));

    if (config) {
        sched->config = *config;
    } else {
        sched->config.max_continuous_on_ms = 400;
        sched->config.overdrive_duration_ms = 25;
        sched->config.overdrive_multiplier = 1.35f;
        sched->config.max_rolling_duty_cycle = 0.60f; // 60% duty cap over 5s
        // Default ESP32-S3 GPIO pins for IRLZ44N gates
        sched->config.gpio_pins[0] = 4;  // Motor 1 (GPIO 4)
        sched->config.gpio_pins[1] = 6;  // Motor 2 (GPIO 6)
        sched->config.gpio_pins[2] = 8;  // Motor 3 (GPIO 8)
        sched->config.gpio_pins[3] = 5;  // Motor 4 (GPIO 5)
        sched->config.gpio_pins[4] = 7;  // Motor 5 (GPIO 7)
    }
}

bool erm_scheduler_enqueue(ErmHardwareScheduler* sched, const StimulusPulse* pulse) {
    if (!sched || !pulse) return false;
    if (sched->safety.safety_trip_latched) return false;
    if (pulse->finger_index >= NUM_FINGERS) return false;
    if (sched->queue_count >= MAX_SCHEDULED_PULSES) return false;

    sched->queue[sched->queue_count++] = *pulse;
    return true;
}

void erm_scheduler_emergency_stop(ErmHardwareScheduler* sched, uint32_t reason_code) {
    if (!sched) return;
    sched->safety.safety_trip_latched = true;
    sched->safety.trip_reason_code = reason_code;
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        sched->current_pwm_duty[i] = 0;
        sched->channel_active[i] = false;
    }
    sched->queue_count = 0;
}

void erm_scheduler_reset_safety(ErmHardwareScheduler* sched) {
    if (!sched) return;
    sched->safety.safety_trip_latched = false;
    sched->safety.trip_reason_code = 0;
    sched->window_start_ms = 0;
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        sched->safety.continuous_on_ms[i] = 0;
        sched->safety.total_active_ms_5s[i] = 0;
        sched->active_time_in_window_ms[i] = 0;
    }
}

bool erm_scheduler_is_tripped(const ErmHardwareScheduler* sched) {
    if (!sched) return false;
    return sched->safety.safety_trip_latched;
}

void erm_scheduler_tick(ErmHardwareScheduler* sched, uint32_t now_ms) {
    if (!sched) return;

    // If safety tripped, zero all outputs immediately
    if (sched->safety.safety_trip_latched) {
        for (uint8_t i = 0; i < NUM_FINGERS; i++) {
            sched->current_pwm_duty[i] = 0;
            sched->channel_active[i] = false;
        }
        return;
    }

    // 1. Initialize rolling 5-second safety window
    if (sched->window_start_ms == 0) {
        sched->window_start_ms = now_ms;
    } else if (now_ms - sched->window_start_ms >= 5000) {
        // Roll window and calculate duty cycles
        for (uint8_t i = 0; i < NUM_FINGERS; i++) {
            sched->safety.rolling_duty_cycle[i] = (float)sched->active_time_in_window_ms[i] / 5000.0f;
            if (sched->safety.rolling_duty_cycle[i] > sched->config.max_rolling_duty_cycle) {
                erm_scheduler_emergency_stop(sched, 0x01); // Error 0x01: Duty cycle cap exceeded
                return;
            }
            sched->active_time_in_window_ms[i] = 0;
        }
        sched->window_start_ms = now_ms;
    }

    // 2. Clear proposed duties
    uint8_t next_duty[NUM_FINGERS] = {0};
    int8_t active_finger = -1;

    // 3. Scan queue and process active pulses
    uint8_t valid_count = 0;
    for (uint8_t i = 0; i < sched->queue_count; i++) {
        StimulusPulse* p = &sched->queue[i];
        if (!p->is_active) continue;

        uint32_t p_end = p->start_time_ms + p->duration_ms;
        if (now_ms >= p_end) {
            // Pulse has finished
            p->is_active = false;
            continue;
        }

        if (now_ms >= p->start_time_ms && now_ms < p_end) {
            // Pulse is currently firing!
            uint32_t elapsed_in_pulse = now_ms - p->start_time_ms;
            uint8_t duty = p->target_duty;

            // Apply ERM mechanical overdrive kick
            if (p->use_overdrive && elapsed_in_pulse < sched->config.overdrive_duration_ms) {
                float od_duty = (float)p->target_duty * sched->config.overdrive_multiplier;
                if (od_duty > 255.0f) od_duty = 255.0f;
                duty = (uint8_t)od_duty;
            }

            // Enforce single-finger isolation (no overlapping)
            if (active_finger == -1) {
                active_finger = p->finger_index;
                next_duty[p->finger_index] = duty;
            }
        }

        // Keep pulse in queue for next tick
        sched->queue[valid_count++] = *p;
    }
    sched->queue_count = valid_count;

    // 4. Update channels & enforce continuous duration limit
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        if (next_duty[i] > 0) {
            sched->current_pwm_duty[i] = next_duty[i];
            sched->channel_active[i] = true;
            sched->safety.continuous_on_ms[i]++;
            sched->active_time_in_window_ms[i]++;

            if (sched->safety.continuous_on_ms[i] > sched->config.max_continuous_on_ms) {
                erm_scheduler_emergency_stop(sched, 0x02); // Error 0x02: Continuous duration overrun
                return;
            }
        } else {
            sched->current_pwm_duty[i] = 0;
            sched->channel_active[i] = false;
            sched->safety.continuous_on_ms[i] = 0;
        }
    }
}

void erm_scheduler_get_duties(const ErmHardwareScheduler* sched, uint8_t out_duties[NUM_FINGERS]) {
    if (!sched || !out_duties) return;
    memcpy(out_duties, sched->current_pwm_duty, NUM_FINGERS);
}
