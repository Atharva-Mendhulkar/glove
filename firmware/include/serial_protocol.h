#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <stddef.h>
#include <stdbool.h>
#include "am_crn_system.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parse incoming ASCII command line (e.g. "MODE CR", "CALIB_RESP 1", "ESTOP")
// Returns true if command was recognized and handled
bool serial_protocol_parse_line(AmCrnSystem* sys, const char* line, uint32_t now_ms);

// Format current system status into telemetry string (e.g. for UART/BLE output)
// Returns length of formatted string
int serial_protocol_format_telemetry(const AmCrnSystem* sys, char* out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // SERIAL_PROTOCOL_H
