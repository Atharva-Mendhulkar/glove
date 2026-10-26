#include "motion_gating_engine.h"
#include <math.h>
#include <string.h>

static inline float clampf(float val, float min_val, float max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

void motion_gating_init(MotionGatingEngine* engine, const MotionGatingConfig* config) {
    if (!engine) return;
    memset(engine, 0, sizeof(MotionGatingEngine));

    if (config) {
        engine->config = *config;
    } else {
        // Default biologically-grounded parameters
        engine->config.ema_alpha = 0.85f;
        engine->config.acc_still_thresh_g = 0.05f;
        engine->config.acc_fast_thresh_g = 0.55f;
        engine->config.jerk_reach_thresh = 7.5f;
        engine->config.gating_scale_acc = 0.70f;
        engine->config.gating_scale_jerk = 0.50f;
        engine->config.max_gating_factor = 2.20f;
    }

    engine->current_features.sensory_gating_factor = 1.0f;
    engine->current_features.stillness_score = 1.0f;
    engine->current_features.motion_state = MOTION_STATE_STILL;
}

void motion_gating_process_sample(MotionGatingEngine* engine, const ImuRawSample* sample) {
    if (!engine || !sample) return;

    uint32_t dt_us = 10000; // Default 10 ms (100 Hz)
    if (engine->prev_timestamp_us > 0 && sample->timestamp_us > engine->prev_timestamp_us) {
        dt_us = sample->timestamp_us - engine->prev_timestamp_us;
    }
    float dt_s = (float)dt_us * 1e-6f;
    if (dt_s <= 0.0f || dt_s > 0.5f) {
        dt_s = 0.01f;
    }

    // 1. Calculate magnitude of acceleration vector in g
    float ax = sample->ax_g;
    float ay = sample->ay_g;
    float az = sample->az_g;
    float acc_mag = sqrtf(ax * ax + ay * ay + az * az);

    // 2. Dynamic acceleration norm (deviation from static 1g gravity)
    float dynamic_norm = fabsf(acc_mag - 1.0f);

    // 3. Gyro magnitude
    float gx = sample->gx_dps;
    float gy = sample->gy_dps;
    float gz = sample->gz_dps;
    float gyro_mag = sqrtf(gx * gx + gy * gy + gz * gz);

    // 4. Jerk (rate of change of acceleration norm)
    float raw_jerk = 0.0f;
    if (engine->sample_count > 0) {
        raw_jerk = fabsf(dynamic_norm - engine->prev_acc_norm) / dt_s;
    }

    // 5. Exponential moving average filtering
    float alpha = engine->config.ema_alpha;
    engine->filtered_acc_norm = (alpha * engine->filtered_acc_norm) + ((1.0f - alpha) * dynamic_norm);
    engine->filtered_jerk = (alpha * engine->filtered_jerk) + ((1.0f - alpha) * raw_jerk);

    // 6. Stillness score: 1.0 when static, decays to 0.0 under movement
    float stillness = 1.0f - clampf(engine->filtered_acc_norm / 0.20f, 0.0f, 1.0f);

    // 7. Sensory gating index G(t)
    // Formula: G(t) = 1.0 + scale_acc * (norm / 0.8g) + scale_jerk * (jerk / 15g/s)
    float acc_term = clampf(engine->filtered_acc_norm / 0.80f, 0.0f, 1.0f) * engine->config.gating_scale_acc;
    float jerk_term = clampf(engine->filtered_jerk / 15.0f, 0.0f, 1.0f) * engine->config.gating_scale_jerk;
    float gating_factor = 1.0f + acc_term + jerk_term;
    gating_factor = clampf(gating_factor, 1.0f, engine->config.max_gating_factor);

    // 8. Motion State classification state machine
    MotionState state = MOTION_STATE_STILL;
    if (engine->filtered_acc_norm < engine->config.acc_still_thresh_g && gyro_mag < 15.0f) {
        state = MOTION_STATE_STILL;
    } else if (engine->filtered_jerk > engine->config.jerk_reach_thresh && engine->filtered_acc_norm > 0.25f) {
        state = MOTION_STATE_REACH;
    } else if (engine->filtered_acc_norm >= engine->config.acc_fast_thresh_g || gyro_mag > 120.0f) {
        state = MOTION_STATE_FAST_MOVE;
    } else if (gyro_mag > 40.0f && dynamic_norm < 0.25f && raw_jerk > 4.0f) {
        state = MOTION_STATE_TREMOR;
    } else {
        state = MOTION_STATE_SLOW_MOVE;
    }

    // Update state struct
    engine->current_features.timestamp_us = sample->timestamp_us;
    engine->current_features.dynamic_acc_norm = dynamic_norm;
    engine->current_features.filtered_acc_norm = engine->filtered_acc_norm;
    engine->current_features.jerk_g_per_s = engine->filtered_jerk;
    engine->current_features.gyro_magnitude_dps = gyro_mag;
    engine->current_features.stillness_score = stillness;
    engine->current_features.sensory_gating_factor = gating_factor;
    engine->current_features.motion_state = state;

    engine->prev_acc_norm = dynamic_norm;
    engine->prev_timestamp_us = sample->timestamp_us;
    engine->sample_count++;
}

const KinematicFeatures* motion_gating_get_features(const MotionGatingEngine* engine) {
    if (!engine) return NULL;
    return &engine->current_features;
}

float motion_gating_get_factor(const MotionGatingEngine* engine) {
    if (!engine) return 1.0f;
    return engine->current_features.sensory_gating_factor;
}

MotionState motion_gating_get_state(const MotionGatingEngine* engine) {
    if (!engine) return MOTION_STATE_UNKNOWN;
    return engine->current_features.motion_state;
}
