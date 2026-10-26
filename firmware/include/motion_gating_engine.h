#ifndef MOTION_GATING_ENGINE_H
#define MOTION_GATING_ENGINE_H

#include "am_crn_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ema_alpha;            // Smoothing factor for acceleration (e.g. 0.85)
    float acc_still_thresh_g;   // Stillness threshold (e.g. 0.05g)
    float acc_fast_thresh_g;    // Fast movement threshold (e.g. 0.60g)
    float jerk_reach_thresh;    // Jerk threshold for intentional reaching (e.g. 8.0 g/s)
    float gating_scale_acc;     // Scaling contribution of acceleration (e.g. 0.70)
    float gating_scale_jerk;    // Scaling contribution of jerk (e.g. 0.50)
    float max_gating_factor;    // Hard ceiling for G(t) (e.g. 2.20)
} MotionGatingConfig;

typedef struct {
    MotionGatingConfig config;
    KinematicFeatures current_features;
    float prev_acc_norm;
    uint32_t prev_timestamp_us;
    float filtered_acc_norm;
    float filtered_jerk;
    uint32_t sample_count;
} MotionGatingEngine;

void motion_gating_init(MotionGatingEngine* engine, const MotionGatingConfig* config);
void motion_gating_process_sample(MotionGatingEngine* engine, const ImuRawSample* sample);
const KinematicFeatures* motion_gating_get_features(const MotionGatingEngine* engine);
float motion_gating_get_factor(const MotionGatingEngine* engine);
MotionState motion_gating_get_state(const MotionGatingEngine* engine);

#ifdef __cplusplus
}
#endif

#endif // MOTION_GATING_ENGINE_H
