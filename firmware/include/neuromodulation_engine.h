#ifndef NEUROMODULATION_ENGINE_H
#define NEUROMODULATION_ENGINE_H

#include "am_crn_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float cr_base_frequency_hz;       // e.g. 1.5 Hz (cycle duration ~667 ms)
    uint16_t burst_duration_ms;       // e.g. 70 ms per finger
    uint16_t min_inter_finger_ms;     // e.g. 45 ms spacing between consecutive fingers
    uint16_t jitter_range_ms;         // e.g. 15 ms pseudo-random phase jitter
    uint16_t active_epoch_duration_s; // e.g. 15 s stimulation block
    uint16_t pause_epoch_duration_s;  // e.g. 3 s habituation-reset pause
} NeuromodulationConfig;

typedef struct {
    NeuromodulationConfig config;
    NeuromodulationMode current_mode;
    FingerCalibrationProfile calibration;
    
    // Cycle and timing tracking
    uint32_t cycle_start_time_ms;
    uint32_t epoch_start_time_ms;
    uint32_t current_cycle_duration_ms;
    bool is_in_pause_epoch;

    // Cycle finger permutation
    uint8_t finger_sequence[NUM_FINGERS];
    uint16_t pulse_offsets_ms[NUM_FINGERS];
    uint32_t lcg_seed;

    // Calibration state machine
    uint8_t calib_current_finger;
    uint8_t calib_current_duty;
    uint8_t calib_reversal_count;
    uint8_t calib_step_size;
    bool calib_last_response;
} NeuromodulationEngine;

void neuromod_init(NeuromodulationEngine* engine, const NeuromodulationConfig* config);
void neuromod_set_mode(NeuromodulationEngine* engine, NeuromodulationMode mode, uint32_t now_ms);
void neuromod_set_calibration(NeuromodulationEngine* engine, const FingerCalibrationProfile* profile);

// Generates next stimulus pulse sequence based on current time and sensory gating factor
uint8_t neuromod_generate_pulses(
    NeuromodulationEngine* engine,
    uint32_t now_ms,
    float sensory_gating_factor,
    StimulusPulse* out_pulses,
    uint8_t max_out_pulses
);

// Perceptual calibration controls
void neuromod_calib_start(NeuromodulationEngine* engine, uint32_t now_ms);
bool neuromod_calib_submit_response(NeuromodulationEngine* engine, bool felt_vibration, uint32_t now_ms);
bool neuromod_calib_is_complete(const NeuromodulationEngine* engine);
const FingerCalibrationProfile* neuromod_get_calibration(const NeuromodulationEngine* engine);

#ifdef __cplusplus
}
#endif

#endif // NEUROMODULATION_ENGINE_H
