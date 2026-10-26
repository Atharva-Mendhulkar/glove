#include "neuromodulation_engine.h"
#include <string.h>

// Simple deterministic 32-bit LCG random generator for embedded real-time safety
static uint32_t lcg_rand(uint32_t* seed) {
    *seed = (*seed * 1664525u + 1013904223u);
    return *seed;
}

static inline int16_t lcg_rand_range(uint32_t* seed, int16_t min_v, int16_t max_v) {
    if (min_v >= max_v) return min_v;
    uint32_t r = lcg_rand(seed);
    return min_v + (int16_t)(r % (uint32_t)(max_v - min_v + 1));
}

// Fisher-Yates shuffle for finger permutations
static void shuffle_fingers(uint8_t arr[NUM_FINGERS], uint32_t* seed) {
    for (int i = NUM_FINGERS - 1; i > 0; i--) {
        int j = lcg_rand_range(seed, 0, i);
        uint8_t tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

void neuromod_init(NeuromodulationEngine* engine, const NeuromodulationConfig* config) {
    if (!engine) return;
    memset(engine, 0, sizeof(NeuromodulationEngine));

    if (config) {
        engine->config = *config;
    } else {
        engine->config.cr_base_frequency_hz = 1.6f;  // ~625 ms per desync cycle
        engine->config.burst_duration_ms = 70;       // 70 ms pulse per finger
        engine->config.min_inter_finger_ms = 45;     // 45 ms blanking to prevent masking
        engine->config.jitter_range_ms = 15;         // +/- 15 ms phase jitter
        engine->config.active_epoch_duration_s = 15; // 15 s stimulation
        engine->config.pause_epoch_duration_s = 3;   // 3 s habituation pause
    }

    engine->lcg_seed = 0x12345678;
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        engine->finger_sequence[i] = i;
        // Sensible population defaults prior to calibration
        engine->calibration.threshold_duty[i] = 75;       // ~30% PWM
        engine->calibration.target_duty[i] = 115;         // ~45% PWM
        engine->calibration.max_comfortable_duty[i] = 190;// ~75% PWM
        engine->calibration.sensitivity_weight[i] = 1.0f;
    }
    engine->calibration.is_calibrated = false;

    engine->current_mode = MODE_STANDBY;
}

void neuromod_set_mode(NeuromodulationEngine* engine, NeuromodulationMode mode, uint32_t now_ms) {
    if (!engine) return;
    engine->current_mode = mode;
    engine->cycle_start_time_ms = 0; // Trigger immediately on first generate_pulses
    engine->epoch_start_time_ms = now_ms;
    engine->is_in_pause_epoch = false;
    shuffle_fingers(engine->finger_sequence, &engine->lcg_seed);
}

void neuromod_set_calibration(NeuromodulationEngine* engine, const FingerCalibrationProfile* profile) {
    if (!engine || !profile) return;
    engine->calibration = *profile;
}

uint8_t neuromod_generate_pulses(
    NeuromodulationEngine* engine,
    uint32_t now_ms,
    float sensory_gating_factor,
    StimulusPulse* out_pulses,
    uint8_t max_out_pulses
) {
    if (!engine || !out_pulses || max_out_pulses < NUM_FINGERS) return 0;
    if (engine->current_mode != MODE_COORDINATED_RESET) return 0;

    // 1. Check Habituation Epochs (Active Block vs. Pause Reset Epoch)
    uint32_t epoch_elapsed_ms = now_ms - engine->epoch_start_time_ms;
    if (!engine->is_in_pause_epoch) {
        if (epoch_elapsed_ms >= (uint32_t)engine->config.active_epoch_duration_s * 1000) {
            engine->is_in_pause_epoch = true;
            engine->epoch_start_time_ms = now_ms;
            return 0; // Pause epoch begins: no pulses generated
        }
    } else {
        if (epoch_elapsed_ms >= (uint32_t)engine->config.pause_epoch_duration_s * 1000) {
            engine->is_in_pause_epoch = false;
            engine->epoch_start_time_ms = now_ms;
            engine->cycle_start_time_ms = 0;
        } else {
            return 0; // In pause epoch
        }
    }

    // 2. Check if current CR cycle completed
    uint32_t base_cycle_ms = (uint32_t)(1000.0f / engine->config.cr_base_frequency_hz);
    if (base_cycle_ms < 500) base_cycle_ms = 500;

    if (engine->cycle_start_time_ms == 0 || (now_ms - engine->cycle_start_time_ms) >= base_cycle_ms) {
        engine->cycle_start_time_ms = now_ms;
        // Reshuffle fingers for desynchronizing Coordinated Reset
        shuffle_fingers(engine->finger_sequence, &engine->lcg_seed);
    } else {
        // Still inside current cycle; pulses already generated
        return 0;
    }

    // 3. Generate Coordinated Reset pulses across the 5 fingers
    uint8_t count = 0;
    uint32_t current_offset = 0;

    for (uint8_t i = 0; i < NUM_FINGERS && count < max_out_pulses; i++) {
        uint8_t finger = engine->finger_sequence[i];

        // Add phase jitter
        int16_t jitter = lcg_rand_range(&engine->lcg_seed,
                                       -(int16_t)engine->config.jitter_range_ms,
                                       +(int16_t)engine->config.jitter_range_ms);
        int32_t scheduled_offset = (int32_t)current_offset + jitter;
        if (scheduled_offset < 0) scheduled_offset = 0;

        // Apply Sensory Gating Compensation
        uint8_t base_duty = engine->calibration.target_duty[finger];
        float scaled_duty_f = (float)base_duty * sensory_gating_factor;
        if (scaled_duty_f > (float)engine->calibration.max_comfortable_duty[finger]) {
            scaled_duty_f = (float)engine->calibration.max_comfortable_duty[finger];
        }
        if (scaled_duty_f > 240.0f) scaled_duty_f = 240.0f; // Keep within hardware limits

        StimulusPulse* p = &out_pulses[count++];
        p->start_time_ms = engine->cycle_start_time_ms + (uint32_t)scheduled_offset;
        p->duration_ms = engine->config.burst_duration_ms;
        p->finger_index = finger;
        p->target_duty = (uint8_t)scaled_duty_f;
        p->use_overdrive = true; // Overdrive pulse to cut ERM latency
        p->is_active = true;

        // Advance offset with non-overlapping safety margin
        current_offset += engine->config.burst_duration_ms + engine->config.min_inter_finger_ms;
    }

    return count;
}

// Staircase calibration implementation
void neuromod_calib_start(NeuromodulationEngine* engine, uint32_t now_ms) {
    (void)now_ms;
    if (!engine) return;
    engine->current_mode = MODE_CALIBRATION;
    engine->calib_current_finger = 0;
    engine->calib_current_duty = 80;
    engine->calib_reversal_count = 0;
    engine->calib_step_size = 16;
    engine->calib_last_response = false;
    engine->calibration.is_calibrated = false;
}

bool neuromod_calib_submit_response(NeuromodulationEngine* engine, bool felt_vibration, uint32_t now_ms) {
    (void)now_ms;
    if (!engine || engine->current_mode != MODE_CALIBRATION) return false;

    // Detect direction reversal
    if (engine->calib_reversal_count > 0 && felt_vibration != engine->calib_last_response) {
        if (engine->calib_step_size > 4) {
            engine->calib_step_size /= 2;
        }
    }
    if (felt_vibration != engine->calib_last_response) {
        engine->calib_reversal_count++;
    }
    engine->calib_last_response = felt_vibration;

    // Adjust staircase
    if (felt_vibration) {
        if (engine->calib_current_duty > engine->calib_step_size) {
            engine->calib_current_duty -= engine->calib_step_size;
        }
    } else {
        if (engine->calib_current_duty + engine->calib_step_size <= 220) {
            engine->calib_current_duty += engine->calib_step_size;
        }
    }

    // Check if this finger has converged (>= 3 reversals)
    if (engine->calib_reversal_count >= 3) {
        uint8_t f = engine->calib_current_finger;
        engine->calibration.threshold_duty[f] = engine->calib_current_duty;
        engine->calibration.target_duty[f] = (uint8_t)((float)engine->calib_current_duty * 1.4f);
        engine->calibration.max_comfortable_duty[f] = (uint8_t)((float)engine->calib_current_duty * 2.2f);
        if (engine->calibration.max_comfortable_duty[f] > 230) {
            engine->calibration.max_comfortable_duty[f] = 230;
        }

        // Move to next finger
        engine->calib_current_finger++;
        engine->calib_current_duty = 80;
        engine->calib_reversal_count = 0;
        engine->calib_step_size = 16;
        engine->calib_last_response = false;

        if (engine->calib_current_finger >= NUM_FINGERS) {
            engine->calibration.is_calibrated = true;
            engine->current_mode = MODE_STANDBY;
            return true; // Entire hand calibration complete!
        }
    }

    return false;
}

bool neuromod_calib_is_complete(const NeuromodulationEngine* engine) {
    if (!engine) return false;
    return engine->calibration.is_calibrated;
}

const FingerCalibrationProfile* neuromod_get_calibration(const NeuromodulationEngine* engine) {
    if (!engine) return NULL;
    return &engine->calibration;
}
