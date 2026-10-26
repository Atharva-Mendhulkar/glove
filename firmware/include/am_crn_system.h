#ifndef AM_CRN_SYSTEM_H
#define AM_CRN_SYSTEM_H

#include "am_crn_types.h"
#include "motion_gating_engine.h"
#include "neuromodulation_engine.h"
#include "erm_hardware_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    MotionGatingEngine motion_engine;
    NeuromodulationEngine neuromod_engine;
    ErmHardwareScheduler scheduler;
    uint32_t last_imu_time_ms;
    uint32_t system_uptime_ms;
    bool is_initialized;
} AmCrnSystem;

void am_crn_system_init(AmCrnSystem* sys);

// Feed IMU sample at ~100 Hz
void am_crn_system_update_imu(AmCrnSystem* sys, const ImuRawSample* sample);

// Fast 1 ms tick for pulse generation and PWM dispatch
void am_crn_system_tick_1ms(AmCrnSystem* sys, uint32_t now_ms);

// Mode control
void am_crn_system_set_mode(AmCrnSystem* sys, NeuromodulationMode mode, uint32_t now_ms);

// Read current status
void am_crn_system_get_motor_duties(const AmCrnSystem* sys, uint8_t out_duties[NUM_FINGERS]);
float am_crn_system_get_gating_factor(const AmCrnSystem* sys);
MotionState am_crn_system_get_motion_state(const AmCrnSystem* sys);
bool am_crn_system_is_safety_tripped(const AmCrnSystem* sys);

#ifdef __cplusplus
}
#endif

#endif // AM_CRN_SYSTEM_H
