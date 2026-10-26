#ifndef AM_CRN_TYPES_H
#define AM_CRN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_FINGERS 5
#define MAX_SCHEDULED_PULSES 32

// Finger indices: 0: Thumb, 1: Index, 2: Middle, 3: Ring, 4: Little
typedef enum {
    FINGER_THUMB  = 0,
    FINGER_INDEX  = 1,
    FINGER_MIDDLE = 2,
    FINGER_RING   = 3,
    FINGER_LITTLE = 4
} FingerIndex;

// Motion states inferred from 6-axis dorsal IMU
typedef enum {
    MOTION_STATE_STILL      = 0,
    MOTION_STATE_SLOW_MOVE  = 1,
    MOTION_STATE_FAST_MOVE  = 2,
    MOTION_STATE_REACH      = 3,
    MOTION_STATE_TREMOR     = 4,
    MOTION_STATE_UNKNOWN    = 5
} MotionState;

// Neuromodulation modes
typedef enum {
    MODE_STANDBY                   = 0,
    MODE_CALIBRATION               = 1,
    MODE_COORDINATED_RESET         = 2, // Desynchronizing phase-staggered pulses
    MODE_STOCHASTIC_RESONANCE      = 3, // Sub-threshold stochastic bursts
    MODE_TACTILE_GUIDANCE          = 4, // Directional / task-based spatial cues
    MODE_HABITUATION_RESET_PAUSE   = 5
} NeuromodulationMode;

// Raw IMU packet from I2C/SPI driver
typedef struct {
    uint32_t timestamp_us;
    float ax_g;     // Acceleration X in g
    float ay_g;     // Acceleration Y in g
    float az_g;     // Acceleration Z in g
    float gx_dps;   // Gyro X in deg/s
    float gy_dps;   // Gyro Y in deg/s
    float gz_dps;   // Gyro Z in deg/s
    int8_t status;  // 0 = OK, negative = error
} ImuRawSample;

// Processed kinematic features and sensory gating index
typedef struct {
    uint32_t timestamp_us;
    float dynamic_acc_norm;     // ||a|| - 1.0g
    float filtered_acc_norm;    // EMA smoothed
    float jerk_g_per_s;         // Rate of change of acceleration
    float gyro_magnitude_dps;   // Angular rate norm
    float stillness_score;      // 0.0 (high movement) to 1.0 (dead still)
    float sensory_gating_factor;// G(t) in [1.0, 2.2]
    MotionState motion_state;
} KinematicFeatures;

// Per-finger perceptual calibration profile (derived from staircase protocol)
typedef struct {
    uint8_t threshold_duty[NUM_FINGERS];  // Base detection threshold (0-255)
    uint8_t target_duty[NUM_FINGERS];     // Therapeutic target duty (0-255)
    uint8_t max_comfortable_duty[NUM_FINGERS]; // Comfort ceiling (0-255)
    float sensitivity_weight[NUM_FINGERS]; // Relative sensitivity normalizer
    bool is_calibrated;
} FingerCalibrationProfile;

// Discrete hardware pulse command
typedef struct {
    uint32_t start_time_ms;
    uint16_t duration_ms;
    uint8_t finger_index;
    uint8_t target_duty;
    bool use_overdrive;       // 20ms initial boost to overcome ERM inertia
    bool is_active;
} StimulusPulse;

// Safety and watchdog metrics
typedef struct {
    uint32_t continuous_on_ms[NUM_FINGERS];
    uint32_t total_active_ms_5s[NUM_FINGERS];
    float rolling_duty_cycle[NUM_FINGERS];
    bool safety_trip_latched;
    uint32_t trip_reason_code;
} SafetyStatus;

#ifdef __cplusplus
}
#endif

#endif // AM_CRN_TYPES_H
