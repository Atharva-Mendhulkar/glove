#include "serial_protocol.h"
#include <stdio.h>
#include <string.h>

bool serial_protocol_parse_line(AmCrnSystem* sys, const char* line, uint32_t now_ms) {
    if (!sys || !line) return false;

    // Skip leading whitespace
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') line++;
    if (*line == '\0') return false;

    if (strncmp(line, "MODE ", 5) == 0) {
        const char* mode_str = line + 5;
        if (strncmp(mode_str, "CR", 2) == 0 || strncmp(mode_str, "COORDINATED_RESET", 17) == 0) {
            am_crn_system_set_mode(sys, MODE_COORDINATED_RESET, now_ms);
            return true;
        } else if (strncmp(mode_str, "STANDBY", 7) == 0) {
            am_crn_system_set_mode(sys, MODE_STANDBY, now_ms);
            return true;
        } else if (strncmp(mode_str, "CALIB", 5) == 0) {
            neuromod_calib_start(&sys->neuromod_engine, now_ms);
            return true;
        }
    } else if (strncmp(line, "CALIB_RESP ", 11) == 0) {
        int response = 0;
        if (sscanf(line + 11, "%d", &response) == 1) {
            bool felt = (response != 0);
            neuromod_calib_submit_response(&sys->neuromod_engine, felt, now_ms);
            return true;
        }
    } else if (strncmp(line, "ESTOP", 5) == 0 || strncmp(line, "EMERGENCY_STOP", 14) == 0) {
        erm_scheduler_emergency_stop(&sys->scheduler, 0xFF);
        return true;
    } else if (strncmp(line, "RESET_SAFETY", 12) == 0) {
        erm_scheduler_reset_safety(&sys->scheduler);
        return true;
    }

    return false;
}

int serial_protocol_format_telemetry(const AmCrnSystem* sys, char* out_buf, size_t max_len) {
    if (!sys || !out_buf || max_len == 0) return 0;

    uint8_t duties[NUM_FINGERS];
    am_crn_system_get_motor_duties(sys, duties);
    float gating = am_crn_system_get_gating_factor(sys);
    MotionState state = am_crn_system_get_motion_state(sys);
    bool tripped = am_crn_system_is_safety_tripped(sys);
    const KinematicFeatures* f = motion_gating_get_features(&sys->motion_engine);
    float stillness = f ? f->stillness_score : 1.0f;

    return snprintf(out_buf, max_len,
        "STATUS state=%d gating=%.2f still=%.2f duty=[%u,%u,%u,%u,%u] safety=%s\n",
        (int)state, gating, stillness,
        duties[0], duties[1], duties[2], duties[3], duties[4],
        tripped ? "TRIP" : "OK");
}
