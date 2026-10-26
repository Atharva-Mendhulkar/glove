#ifndef ERM_HARDWARE_SCHEDULER_H
#define ERM_HARDWARE_SCHEDULER_H

#include "am_crn_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t max_continuous_on_ms;  // Max continuous pulse duration (e.g. 250 ms)
    uint16_t overdrive_duration_ms; // Overdrive boost duration (e.g. 20 ms)
    float overdrive_multiplier;     // Overdrive boost ratio (e.g. 1.35f)
    float max_rolling_duty_cycle;   // Duty cycle ceiling over 5s (e.g. 0.40f = 40%)
    uint8_t gpio_pins[NUM_FINGERS]; // Hardware GPIO pins for the 5 MOSFET gates
} ErmSchedulerConfig;

typedef struct {
    ErmSchedulerConfig config;
    StimulusPulse queue[MAX_SCHEDULED_PULSES];
    uint8_t queue_count;

    // Current output state
    uint8_t current_pwm_duty[NUM_FINGERS];
    uint32_t channel_on_timestamp_ms[NUM_FINGERS];
    bool channel_active[NUM_FINGERS];

    // Safety and metrics
    SafetyStatus safety;
    uint32_t window_start_ms;
    uint32_t active_time_in_window_ms[NUM_FINGERS];
} ErmHardwareScheduler;

void erm_scheduler_init(ErmHardwareScheduler* sched, const ErmSchedulerConfig* config);

// Add pulse to hardware execution queue
bool erm_scheduler_enqueue(ErmHardwareScheduler* sched, const StimulusPulse* pulse);

// 1 ms periodic tick called from RTOS timer or high-priority task
void erm_scheduler_tick(ErmHardwareScheduler* sched, uint32_t now_ms);

// Read current PWM duties for the 5 channels (0-255)
void erm_scheduler_get_duties(const ErmHardwareScheduler* sched, uint8_t out_duties[NUM_FINGERS]);

// Emergency Stop
void erm_scheduler_emergency_stop(ErmHardwareScheduler* sched, uint32_t reason_code);

// Reset safety trip
void erm_scheduler_reset_safety(ErmHardwareScheduler* sched);

// Check if safety trip is active
bool erm_scheduler_is_tripped(const ErmHardwareScheduler* sched);

#ifdef __cplusplus
}
#endif

#endif // ERM_HARDWARE_SCHEDULER_H
