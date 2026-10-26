/**
 * @file esp32s3_glove_main.cpp
 * @brief Complete reference firmware entry point for ESP32-S3 AM-CRN Glove.
 * Compatible with ESP-IDF and Arduino-ESP32 Core v3+.
 */

#include <Arduino.h>
#include <Wire.h>
#include "am_crn_types.h"
#include "am_crn_system.h"

// Hardware Pin Configuration
static const uint8_t MOTOR_PINS[NUM_FINGERS] = {
    4,  // Thumb  - GPIO4  (LEDC Ch 0)
    5,  // Index  - GPIO5  (LEDC Ch 1)
    6,  // Middle - GPIO6  (LEDC Ch 2)
    7,  // Ring   - GPIO7  (LEDC Ch 3)
    15  // Little - GPIO15 (LEDC Ch 4)
};

static const uint32_t LEDC_FREQ_HZ = 5000; // 5 kHz PWM (well above motor acoustic range)
static const uint8_t  LEDC_RES_BITS = 8;   // 8-bit duty (0 - 255)

// Dorsal IMU I2C configuration (e.g., MPU6050 / ICM42670 / LSM6DS3)
static const uint8_t IMU_I2C_ADDR = 0x68;
static const int IMU_SDA_PIN = 8;
static const int IMU_SCL_PIN = 9;

static AmCrnSystem g_am_crn;
static uint32_t g_last_imu_sample_us = 0;
static uint32_t g_last_telemetry_ms = 0;

// Read raw acceleration and gyroscope data from IMU over I2C
static bool read_imu_sample(ImuRawSample* sample) {
    Wire.beginTransmission(IMU_I2C_ADDR);
    Wire.write(0x3B); // ACCEL_XOUT_H register
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom((uint16_t)IMU_I2C_ADDR, (uint8_t)14) != 14) return false;

    int16_t ax_raw = (Wire.read() << 8) | Wire.read();
    int16_t ay_raw = (Wire.read() << 8) | Wire.read();
    int16_t az_raw = (Wire.read() << 8) | Wire.read();
    Wire.read(); Wire.read(); // Skip temperature
    int16_t gx_raw = (Wire.read() << 8) | Wire.read();
    int16_t gy_raw = (Wire.read() << 8) | Wire.read();
    int16_t gz_raw = (Wire.read() << 8) | Wire.read();

    // Standard scaling for +/- 4g and +/- 1000 dps
    sample->timestamp_us = micros();
    sample->ax_g = (float)ax_raw / 8192.0f;
    sample->ay_g = (float)ay_raw / 8192.0f;
    sample->az_g = (float)az_raw / 8192.0f;
    sample->gx_dps = (float)gx_raw / 32.8f;
    sample->gy_dps = (float)gy_raw / 32.8f;
    sample->gz_dps = (float)gz_raw / 32.8f;
    sample->status = 0;
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[AM-CRN] Initializing ESP32-S3 Five-Finger Neuromodulation Glove...");

    // 1. Initialize PWM LEDC Channels
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        pinMode(MOTOR_PINS[i], OUTPUT);
        digitalWrite(MOTOR_PINS[i], LOW);
        ledcAttach(MOTOR_PINS[i], LEDC_FREQ_HZ, LEDC_RES_BITS);
        ledcWrite(MOTOR_PINS[i], 0);
    }
    Serial.println("[AM-CRN] 5x AO3400A MOSFET LEDC channels initialized (5 kHz, 8-bit).");

    // 2. Initialize I2C for Dorsal IMU
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN, 400000); // 400 kHz Fast I2C
    Wire.beginTransmission(IMU_I2C_ADDR);
    Wire.write(0x6B); // PWR_MGMT_1
    Wire.write(0x00); // Wake up sensor
    Wire.endTransmission();
    Serial.println("[AM-CRN] Dorsal 6-axis IMU wake-up sent.");

    // 3. Initialize Core AM-CRN System
    am_crn_system_init(&g_am_crn);
    uint32_t now_ms = millis();
    am_crn_system_set_mode(&g_am_crn, MODE_COORDINATED_RESET, now_ms);
    Serial.println("[AM-CRN] AM-CRN System Online. Mode: Coordinated Reset.");
}

void loop() {
    uint32_t now_us = micros();
    uint32_t now_ms = millis();

    // 1. IMU Acquisition Task at 100 Hz (every 10,000 us)
    if (now_us - g_last_imu_sample_us >= 10000) {
        g_last_imu_sample_us = now_us;
        ImuRawSample sample;
        if (read_imu_sample(&sample)) {
            am_crn_system_update_imu(&g_am_crn, &sample);
        }
    }

    // 2. High-Priority 1 ms Scheduler Tick
    static uint32_t last_tick_ms = 0;
    if (now_ms != last_tick_ms) {
        last_tick_ms = now_ms;
        am_crn_system_tick_1ms(&g_am_crn, now_ms);

        // Dispatch updated PWM duties to hardware LEDC timers
        uint8_t duties[NUM_FINGERS];
        am_crn_system_get_motor_duties(&g_am_crn, duties);
        for (uint8_t i = 0; i < NUM_FINGERS; i++) {
            ledcWrite(MOTOR_PINS[i], duties[i]);
        }
    }

    // 3. Periodic Diagnostics Stream (every 250 ms)
    if (now_ms - g_last_telemetry_ms >= 250) {
        g_last_telemetry_ms = now_ms;
        uint8_t duties[NUM_FINGERS];
        am_crn_system_get_motor_duties(&g_am_crn, duties);
        float gating = am_crn_system_get_gating_factor(&g_am_crn);
        MotionState state = am_crn_system_get_motion_state(&g_am_crn);
        bool tripped = am_crn_system_is_safety_tripped(&g_am_crn);

        Serial.printf("[STATUS] State: %d | Gating G(t): %.2f | Duties: [%3u %3u %3u %3u %3u] | Safety: %s\n",
                      state, gating, duties[0], duties[1], duties[2], duties[3], duties[4],
                      tripped ? "TRIPPED" : "OK");
    }
}
