/**
 * @file esp32s3_glove_main.cpp
 * @brief Complete reference firmware entry point for ESP32-S3 AM-CRN Glove.
 * Supports rock-solid non-blocking auto-detection of dorsal IMU (MPU-6050/ICM-42670),
 * automatic 6-axis resting calibration, real-time kinematic gating, and motor sequencing.
 */

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "am_crn_types.h"
#include "am_crn_system.h"

// Hardware Pin Configuration for 5 ERM Vibration Motors (IRLZ44N MOSFET Gates)
static const uint8_t MOTOR_PINS[NUM_FINGERS] = {
    4,  // Thumb  - GPIO4  (LEDC Ch 0)
    5,  // Index  - GPIO5  (LEDC Ch 1)
    6,  // Middle - GPIO6  (LEDC Ch 2)
    7,  // Ring   - GPIO7  (LEDC Ch 3)
    15  // Little - GPIO15 (LEDC Ch 4)
};

static const uint32_t LEDC_FREQ_HZ = 5000; // 5 kHz PWM (above motor acoustic resonance)
static const uint8_t  LEDC_RES_BITS = 8;   // 8-bit duty (0 - 255)

static const char* FINGER_NAMES[NUM_FINGERS] = {
    "Thumb (GPIO4)",
    "Index (GPIO5)",
    "Middle (GPIO6)",
    "Ring (GPIO7)",
    "Little (GPIO15)"
};

static const char* MOTION_STATE_NAMES[] = {
    "STILL",
    "SLOW_MOVE",
    "FAST_MOVE",
    "REACH",
    "TREMOR",
    "UNKNOWN"
};

// ============================================================================
// DORSAL IMU CONFIGURATION & CALIBRATION DATA STRUCTURES
// ============================================================================
static uint8_t g_imu_addr = 0x68;
static int g_imu_sda = 3;  // User specified: SDA on GPIO 3
static int g_imu_scl = 10; // User specified: SCL on GPIO 10
static bool g_imu_detected = false;
static bool g_stream_imu = false;

struct ImuCalibration {
    float gyro_bias_x;
    float gyro_bias_y;
    float gyro_bias_z;
    float accel_scale;
    float accel_bias_x;
    float accel_bias_y;
    float accel_bias_z;
    bool is_calibrated;
};

static ImuCalibration g_imu_calib = {
    0.0f, 0.0f, 0.0f,  // gyro biases (deg/s)
    1.0f,              // accel scale factor
    0.0f, 0.0f, 0.0f,  // accel offsets (g)
    false              // calibrated flag
};

static AmCrnSystem g_am_crn;
static uint32_t g_last_imu_sample_us = 0;
static uint32_t g_last_telemetry_ms = 0;

// ============================================================================
// ULTRA-FAST, NON-BLOCKING SOFTWARE I2C PROBE (Prevents ESP-IDF Bus Locks)
// ============================================================================

static bool i2c_bitbang_check(int sda, int scl, uint8_t addr) {
    // 1. Line health check: both pins must be pulled high
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, INPUT_PULLUP);
    delayMicroseconds(6);

    if (digitalRead(sda) == LOW || digitalRead(scl) == LOW) {
        return false; // Line held low by pulldown or short; invalid for I2C
    }

    // 2. START Condition (SDA goes LOW while SCL is HIGH)
    pinMode(sda, OUTPUT);
    digitalWrite(sda, LOW);
    delayMicroseconds(5);
    pinMode(scl, OUTPUT);
    digitalWrite(scl, LOW);
    delayMicroseconds(5);

    // 3. Send 7-bit Address + Write (0)
    uint8_t byte = (addr << 1);
    for (int b = 7; b >= 0; b--) {
        if (byte & (1 << b)) {
            pinMode(sda, INPUT_PULLUP);
        } else {
            pinMode(sda, OUTPUT);
            digitalWrite(sda, LOW);
        }
        delayMicroseconds(4);
        pinMode(scl, INPUT_PULLUP); // SCL high
        delayMicroseconds(4);
        pinMode(scl, OUTPUT);
        digitalWrite(scl, LOW);    // SCL low
        delayMicroseconds(4);
    }

    // 4. Sample ACK bit from Slave (ACK = LOW)
    pinMode(sda, INPUT_PULLUP);
    delayMicroseconds(4);
    pinMode(scl, INPUT_PULLUP);
    delayMicroseconds(4);
    bool ack = (digitalRead(sda) == LOW);
    pinMode(scl, OUTPUT);
    digitalWrite(scl, LOW);
    delayMicroseconds(4);

    // 5. STOP Condition (SDA goes HIGH while SCL is HIGH)
    pinMode(sda, OUTPUT);
    digitalWrite(sda, LOW);
    delayMicroseconds(4);
    pinMode(scl, INPUT_PULLUP);
    delayMicroseconds(4);
    pinMode(sda, INPUT_PULLUP);
    delayMicroseconds(4);

    return ack;
}

// Check if a GPIO is reserved for motors, UART, USB, flash, or strapping
static bool is_reserved_pin(int pin) {
    // 1. Motor MOSFET pins
    const uint8_t MOTOR_ALL[] = {4, 5, 6, 7, 8, 15};
    for (int i = 0; i < 6; i++) {
        if (pin == MOTOR_ALL[i]) return true;
    }
    // 2. Hardware UART0 (Serial over USB CP2102)
    if (pin == 43 || pin == 44) return true;
    // 3. Native USB D-/D+
    if (pin == 19 || pin == 20) return true;
    // 4. SPI Flash / Octal PSRAM bus
    if (pin >= 26 && pin <= 37) return true;
    // 5. Critical Strapping pins (note: GPIO 3 is intentionally unreserved as it is SDA)
    if (pin == 0 || pin == 45 || pin == 46) return true;

    return false;
}

// Candidate pin pairs to automatically test
struct I2CPinPair {
    int sda;
    int scl;
    const char* desc;
};

static const I2CPinPair CANDIDATE_PAIRS[] = {
    {3, 10, "User Hardware Wiring (SDA=3, SCL=10)"},
    {10, 3, "User Inverted Wiring (SDA=10, SCL=3)"},
    {38, 39, "Dorsal Port A (SDA=38, SCL=39)"},
    {39, 38, "Dorsal Port A Rev (SDA=39, SCL=38)"},
    {1, 2, "Dorsal Port B (SDA=1, SCL=2)"},
    {2, 1, "Dorsal Port B Rev (SDA=2, SCL=1)"},
    {17, 18, "Dorsal Port C (SDA=17, SCL=18)"},
    {18, 17, "Dorsal Port C Rev (SDA=18, SCL=17)"},
    {21, 47, "Dorsal Port D (SDA=21, SCL=47)"},
    {47, 21, "Dorsal Port D Rev (SDA=47, SCL=21)"},
    {11, 12, "Dorsal Port E (SDA=11, SCL=12)"},
    {12, 13, "Dorsal Port F (SDA=12, SCL=13)"},
    {40, 41, "Dorsal Port G (SDA=40, SCL=41)"},
    {41, 42, "Dorsal Port H (SDA=41, SCL=42)"}
};
static const uint8_t NUM_CANDIDATE_PAIRS = sizeof(CANDIDATE_PAIRS) / sizeof(CANDIDATE_PAIRS[0]);

// ============================================================================
// HARDWARE I2C PROTOCOL & REGISTER CONFIGURATION
// ============================================================================

static uint8_t read_who_am_i(uint8_t addr) {
    Wire.beginTransmission(addr);
    Wire.write(0x75); // WHO_AM_I register
    if (Wire.endTransmission(false) != 0) return 0xFF;
    if (Wire.requestFrom((uint16_t)addr, (uint8_t)1) != 1) return 0xFF;
    return Wire.read();
}

static bool configure_imu(uint8_t addr) {
    // 1. Wake up sensor and select PLL with X-axis gyroscope reference clock
    Wire.beginTransmission(addr);
    Wire.write(0x6B); // PWR_MGMT_1
    Wire.write(0x01); // Clock source = PLL X-gyro, SLEEP = 0
    if (Wire.endTransmission() != 0) return false;
    delay(15);

    // 2. Configure DLPF: ~44 Hz bandwidth (CONFIG = 0x03) to filter motor vibration
    Wire.beginTransmission(addr);
    Wire.write(0x1A); // CONFIG
    Wire.write(0x03); // DLPF_CFG = 3 (Accel 44 Hz, Gyro 42 Hz)
    Wire.endTransmission();

    // 3. Configure Gyroscope: +/- 1000 dps (GYRO_CONFIG = 0x10) -> 32.8 LSB/dps
    Wire.beginTransmission(addr);
    Wire.write(0x1B); // GYRO_CONFIG
    Wire.write(0x10);
    Wire.endTransmission();

    // 4. Configure Accelerometer: +/- 4g (ACCEL_CONFIG = 0x08) -> 8192 LSB/g
    Wire.beginTransmission(addr);
    Wire.write(0x1C); // ACCEL_CONFIG
    Wire.write(0x08);
    Wire.endTransmission();

    // 5. Sample rate divider: 100 Hz (SMPLRT_DIV = 0x09: 1kHz / (1 + 9))
    Wire.beginTransmission(addr);
    Wire.write(0x19); // SMPLRT_DIV
    Wire.write(0x09);
    Wire.endTransmission();

    return true;
}

static bool auto_detect_imu() {
    Serial.println("[IMU SCAN] Scanning candidate I2C pin pairs for dorsal 6-axis IMU...");

    for (uint8_t i = 0; i < NUM_CANDIDATE_PAIRS; i++) {
        int sda = CANDIDATE_PAIRS[i].sda;
        int scl = CANDIDATE_PAIRS[i].scl;

        if (is_reserved_pin(sda) || is_reserved_pin(scl)) continue;

        // Check address 0x68 and 0x69 with non-blocking bitbang
        uint8_t detected = 0;
        if (i2c_bitbang_check(sda, scl, 0x68)) detected = 0x68;
        else if (i2c_bitbang_check(sda, scl, 0x69)) detected = 0x69;

        if (detected != 0) {
            Serial.printf("[IMU SCAN] FOUND IMU at 0x%02X on SDA=GPIO%d, SCL=GPIO%d (%s)\n",
                          detected, sda, scl, CANDIDATE_PAIRS[i].desc);

            g_imu_addr = detected;
            g_imu_sda = sda;
            g_imu_scl = scl;
            g_imu_detected = true;

            // Start hardware I2C master
            Wire.begin(g_imu_sda, g_imu_scl, 400000);
            Wire.setTimeOut(25);

            uint8_t who = read_who_am_i(g_imu_addr);
            Serial.printf("[IMU SCAN] WHO_AM_I Register: 0x%02X ", who);
            if (who == 0x68) Serial.println("(MPU-6050 confirmed)");
            else if (who == 0x70) Serial.println("(MPU-6500 confirmed)");
            else if (who == 0x71 || who == 0x73) Serial.println("(MPU-9250 confirmed)");
            else if (who == 0x67) Serial.println("(ICM-42670 confirmed)");
            else Serial.println("(InvenSense compatible sensor)");

            configure_imu(g_imu_addr);
            return true;
        }
    }

    Serial.println("[IMU SCAN] Notice: IMU not responding on candidate pin pairs.");
    Serial.println("[IMU SCAN] Initializing I2C fallback on SDA=GPIO3, SCL=GPIO10.");
    Wire.begin(3, 10, 100000);
    Wire.setTimeOut(25);
    g_imu_detected = false;
    return false;
}

// Read raw acceleration and gyroscope data from IMU over I2C
static bool read_imu_raw(float* ax, float* ay, float* az, float* gx, float* gy, float* gz) {
    if (!g_imu_detected) return false;

    Wire.beginTransmission(g_imu_addr);
    Wire.write(0x3B); // ACCEL_XOUT_H register
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom((uint16_t)g_imu_addr, (uint8_t)14) != 14) return false;

    int16_t ax_raw = (Wire.read() << 8) | Wire.read();
    int16_t ay_raw = (Wire.read() << 8) | Wire.read();
    int16_t az_raw = (Wire.read() << 8) | Wire.read();
    Wire.read(); Wire.read(); // Skip temperature bytes
    int16_t gx_raw = (Wire.read() << 8) | Wire.read();
    int16_t gy_raw = (Wire.read() << 8) | Wire.read();
    int16_t gz_raw = (Wire.read() << 8) | Wire.read();

    // Scale +/- 4g (8192 LSB/g) and +/- 1000 dps (32.8 LSB/dps)
    *ax = (float)ax_raw / 8192.0f;
    *ay = (float)ay_raw / 8192.0f;
    *az = (float)az_raw / 8192.0f;
    *gx = (float)gx_raw / 32.8f;
    *gy = (float)gy_raw / 32.8f;
    *gz = (float)gz_raw / 32.8f;
    return true;
}

// Read calibrated IMU sample into standard ImuRawSample packet
static bool read_imu_sample(ImuRawSample* sample) {
    float raw_ax, raw_ay, raw_az, raw_gx, raw_gy, raw_gz;
    if (!read_imu_raw(&raw_ax, &raw_ay, &raw_az, &raw_gx, &raw_gy, &raw_gz)) {
        return false;
    }

    sample->timestamp_us = micros();

    if (g_imu_calib.is_calibrated) {
        // Gyro zero-rate bias subtraction
        sample->gx_dps = raw_gx - g_imu_calib.gyro_bias_x;
        sample->gy_dps = raw_gy - g_imu_calib.gyro_bias_y;
        sample->gz_dps = raw_gz - g_imu_calib.gyro_bias_z;

        // Accelerometer planar bias subtraction and scale normalization
        sample->ax_g = (raw_ax - g_imu_calib.accel_bias_x) * g_imu_calib.accel_scale;
        sample->ay_g = (raw_ay - g_imu_calib.accel_bias_y) * g_imu_calib.accel_scale;
        sample->az_g = (raw_az - g_imu_calib.accel_bias_z) * g_imu_calib.accel_scale;
    } else {
        sample->ax_g = raw_ax;
        sample->ay_g = raw_ay;
        sample->az_g = raw_az;
        sample->gx_dps = raw_gx;
        sample->gy_dps = raw_gy;
        sample->gz_dps = raw_gz;
    }
    sample->status = 0;
    return true;
}

// ============================================================================
// CALIBRATION ROUTINE: ZERO-RATE GYRO BIAS & 1.0G ACCELERATION NORMALIZATION
// ============================================================================

void run_imu_calibration() {
    if (!g_imu_detected) {
        Serial.println("\n[CALIB ERROR] Cannot calibrate: No IMU detected on I2C bus!");
        Serial.println("[CALIB] Check wiring (VCC->3.3V, GND->GND, SDA->Pin, SCL->Pin).");
        return;
    }

    Serial.println("\n=======================================================");
    Serial.println("       AM-CRN DORSAL IMU 6-AXIS CALIBRATION           ");
    Serial.println("=======================================================");
    Serial.println("[CALIB] Place glove completely STILL and FLAT on table...");
    Serial.println("[CALIB] Collecting 250 resting kinematic samples (~1.8s)...");

    float sum_ax = 0.0f, sum_ay = 0.0f, sum_az = 0.0f;
    float sum_gx = 0.0f, sum_gy = 0.0f, sum_gz = 0.0f;
    const int NUM_SAMPLES = 250;
    int valid_count = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        float ax, ay, az, gx, gy, gz;
        if (read_imu_raw(&ax, &ay, &az, &gx, &gy, &gz)) {
            sum_ax += ax; sum_ay += ay; sum_az += az;
            sum_gx += gx; sum_gy += gy; sum_gz += gz;
            valid_count++;
        }
        delay(7); // ~140 Hz sampling
        if ((i + 1) % 50 == 0) {
            Serial.printf("[CALIB] Progress: %3d / %3d samples collected...\n", i + 1, NUM_SAMPLES);
        }
    }

    if (valid_count < 100) {
        Serial.println("[CALIB ERROR] I2C communication unstable (<100 valid samples). Aborted.");
        return;
    }

    float mean_ax = sum_ax / valid_count;
    float mean_ay = sum_ay / valid_count;
    float mean_az = sum_az / valid_count;
    float mean_gx = sum_gx / valid_count;
    float mean_gy = sum_gy / valid_count;
    float mean_gz = sum_gz / valid_count;

    // 1. Gyro zero-rate bias: static angular velocity should be 0.0 deg/s
    g_imu_calib.gyro_bias_x = mean_gx;
    g_imu_calib.gyro_bias_y = mean_gy;
    g_imu_calib.gyro_bias_z = mean_gz;

    // 2. Accelerometer resting norm must equal exactly 1.000 g (Earth gravity)
    float raw_mag = sqrtf(mean_ax * mean_ax + mean_ay * mean_ay + mean_az * mean_az);
    if (raw_mag > 0.4f && raw_mag < 2.5f) {
        g_imu_calib.accel_scale = 1.000f / raw_mag;
    } else {
        g_imu_calib.accel_scale = 1.0f;
    }

    // 3. Planar horizontal offsets: when resting flat, X and Y should average 0.0g
    if (fabsf(mean_az) > fabsf(mean_ax) && fabsf(mean_az) > fabsf(mean_ay)) {
        g_imu_calib.accel_bias_x = mean_ax;
        g_imu_calib.accel_bias_y = mean_ay;
        float expected_z = (mean_az >= 0.0f) ? (1.0f / g_imu_calib.accel_scale) : (-1.0f / g_imu_calib.accel_scale);
        g_imu_calib.accel_bias_z = mean_az - expected_z;
    } else {
        g_imu_calib.accel_bias_x = 0.0f;
        g_imu_calib.accel_bias_y = 0.0f;
        g_imu_calib.accel_bias_z = 0.0f;
    }

    g_imu_calib.is_calibrated = true;

    // Verify calibration with an immediate verification read
    delay(40);
    ImuRawSample check;
    read_imu_sample(&check);
    float check_mag = sqrtf(check.ax_g * check.ax_g + check.ay_g * check.ay_g + check.az_g * check.az_g);
    float check_gyro = sqrtf(check.gx_dps * check.gx_dps + check.gy_dps * check.gy_dps + check.gz_dps * check.gz_dps);

    Serial.println("\n---------------- CALIBRATION SUMMARY -----------------");
    Serial.printf("  Raw Static Accel Mean : [%+.3fg, %+.3fg, %+.3fg] -> Norm = %.3fg\n",
                  mean_ax, mean_ay, mean_az, raw_mag);
    Serial.printf("  Applied Scale Factor  : %.4f (Normalizes static norm to 1.000g)\n",
                  g_imu_calib.accel_scale);
    Serial.printf("  Accel Planar Offsets  : dx=%+.3fg, dy=%+.3fg, dz=%+.3fg\n",
                  g_imu_calib.accel_bias_x, g_imu_calib.accel_bias_y, g_imu_calib.accel_bias_z);
    Serial.printf("  Gyro Drift Biases     : [%+.2f, %+.2f, %+.2f] deg/s\n",
                  mean_gx, mean_gy, mean_gz);
    Serial.printf("  CALIBRATED CHECK READ : ||a|| = %.3fg, ||w|| = %.2f deg/s\n",
                  check_mag, check_gyro);
    Serial.println("-------------------------------------------------------");
    Serial.println("[CALIB] SUCCESS: Dorsal 6-axis IMU calibration profile active!\n");
}

// Comprehensive I2C bus scanner across all general GPIOs
void run_full_i2c_scan() {
    Serial.println("\n=======================================================");
    Serial.println("       ESP32-S3 COMPREHENSIVE I2C BUS SCANNER          ");
    Serial.println("=======================================================");
    Serial.println("Probing GPIO combinations via non-blocking bitbang...");

    const uint8_t TEST_GPIOS[] = {1, 2, 3, 9, 10, 11, 12, 13, 14, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48};
    const uint8_t NUM_TEST = sizeof(TEST_GPIOS) / sizeof(TEST_GPIOS[0]);
    bool any_found = false;

    for (uint8_t i = 0; i < NUM_TEST; i++) {
        for (uint8_t j = 0; j < NUM_TEST; j++) {
            if (i == j) continue;
            int sda = TEST_GPIOS[i];
            int scl = TEST_GPIOS[j];

            if (is_reserved_pin(sda) || is_reserved_pin(scl)) continue;

            if (i2c_bitbang_check(sda, scl, 0x68) || i2c_bitbang_check(sda, scl, 0x69)) {
                uint8_t addr = i2c_bitbang_check(sda, scl, 0x68) ? 0x68 : 0x69;
                Serial.printf(">>> FOUND IMU Device at 0x%02X on SDA=GPIO%d, SCL=GPIO%d <<<\n", addr, sda, scl);
                g_imu_addr = addr;
                g_imu_sda = sda;
                g_imu_scl = scl;
                g_imu_detected = true;
                any_found = true;

                // Bind hardware I2C to this pair
                Wire.end();
                Wire.begin(g_imu_sda, g_imu_scl, 400000);
                Wire.setTimeOut(25);
                configure_imu(g_imu_addr);
                run_imu_calibration();
                return;
            }
        }
    }

    if (!any_found) {
        Serial.println("[I2C SCAN] No I2C devices responded on any tested GPIO pairs.");
        Serial.println("  1) Verify VCC is connected to 3.3V and GND to GND.");
        Serial.println("  2) Verify SDA and SCL jumper wires are firmly connected.");
        Serial.println("  3) Check if MPU-6050 LED is lit on the module.");
    }
    Serial.println("=======================================================\n");
}

// ============================================================================
// HARDWARE TEST ROUTINES
// ============================================================================

void run_hardware_sweep_test() {
    Serial.println("\n=======================================================");
    Serial.println("  AM-CRN GLOVE: 5-MOTOR HARDWARE STARTUP SWEEP TEST");
    Serial.println("=======================================================");
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        Serial.printf("[TEST] -> Testing Motor %u: %s (ON for 400ms)...\n", i + 1, FINGER_NAMES[i]);
        ledcWrite(MOTOR_PINS[i], 180);
        delay(400);
        ledcWrite(MOTOR_PINS[i], 0);
        delay(100);
    }
    Serial.println("[TEST] -> Testing ALL 5 MOTORS together (ON for 400ms)...");
    for (uint8_t i = 0; i < NUM_FINGERS; i++) ledcWrite(MOTOR_PINS[i], 180);
    delay(400);
    for (uint8_t i = 0; i < NUM_FINGERS; i++) ledcWrite(MOTOR_PINS[i], 0);
    delay(200);
    Serial.println("=======================================================");
    Serial.println("  COMMANDS AVAILABLE IN SERIAL TERMINAL:");
    Serial.println("  * 'i' : Toggle LIVE 10 Hz IMU kinematic stream");
    Serial.println("  * 'k' : Run Dorsal IMU calibration (zero-bias & 1.0g)");
    Serial.println("  * 'w' : Run comprehensive I2C bus/pin scanner");
    Serial.println("  * '1'-'5': Pulse individual finger motor");
    Serial.println("  * 't' : Re-run 5-motor sweep test");
    Serial.println("  * 'a' : All motors ON, 's' : Motors OFF / Standby");
    Serial.println("  * 'c' : Coordinated Reset therapeutic mode");
    Serial.println("=======================================================\n");
}

// ============================================================================
// SETUP & MAIN LOOP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n[AM-CRN] Initializing ESP32-S3 Five-Finger Neuromodulation Glove...");

    // 1. Initialize PWM LEDC Channels
    for (uint8_t i = 0; i < NUM_FINGERS; i++) {
        pinMode(MOTOR_PINS[i], OUTPUT);
        digitalWrite(MOTOR_PINS[i], LOW);
        ledcAttach(MOTOR_PINS[i], LEDC_FREQ_HZ, LEDC_RES_BITS);
        ledcWrite(MOTOR_PINS[i], 0);
    }
    Serial.println("[AM-CRN] 5x IRLZ44N MOSFET LEDC channels initialized (5 kHz, 8-bit).");

    // 2. Run Hardware POST Motor Sweep
    run_hardware_sweep_test();

    // 3. Auto-detect Dorsal IMU with non-blocking probe
    if (auto_detect_imu()) {
        delay(150);
        run_imu_calibration();
    } else {
        Serial.println("[AM-CRN NOTICE] Dorsal IMU offline. Telemetry holding static baseline (G=1.00).");
    }

    // 4. Initialize Core AM-CRN System
    am_crn_system_init(&g_am_crn);
    uint32_t now_ms = millis();
    am_crn_system_set_mode(&g_am_crn, MODE_COORDINATED_RESET, now_ms);
    Serial.println("[AM-CRN] AM-CRN System Online. Mode: Coordinated Reset.");
}

void loop() {
    uint32_t now_us = micros();
    uint32_t now_ms = millis();

    // 0. Handle Serial Commands for Manual Testing & Calibration
    if (Serial.available()) {
        char ch = Serial.read();
        if (ch >= '1' && ch <= '5') {
            uint8_t idx = ch - '1';
            Serial.printf("[MANUAL] Buzzing Motor %u (%s) for 1 second...\n", idx + 1, FINGER_NAMES[idx]);
            ledcWrite(MOTOR_PINS[idx], 190);
            delay(1000);
            ledcWrite(MOTOR_PINS[idx], 0);
        } else if (ch == 'i' || ch == 'I') {
            g_stream_imu = !g_stream_imu;
            Serial.printf("[STREAM] Live IMU telemetry stream: %s\n", g_stream_imu ? "ENABLED (10 Hz)" : "DISABLED");
        } else if (ch == 'k' || ch == 'K') {
            run_imu_calibration();
        } else if (ch == 'w' || ch == 'W') {
            run_full_i2c_scan();
        } else if (ch == 't' || ch == 'T') {
            run_hardware_sweep_test();
        } else if (ch == 'a' || ch == 'A') {
            Serial.println("[MANUAL] All 5 motors ON (buzzing 2 seconds)...");
            for (uint8_t i = 0; i < NUM_FINGERS; i++) ledcWrite(MOTOR_PINS[i], 190);
            delay(2000);
            for (uint8_t i = 0; i < NUM_FINGERS; i++) ledcWrite(MOTOR_PINS[i], 0);
        } else if (ch == 'c' || ch == 'C') {
            Serial.println("[MODE] Coordinated Reset desynchronization mode active.");
            am_crn_system_set_mode(&g_am_crn, MODE_COORDINATED_RESET, millis());
        } else if (ch == 's' || ch == 'S') {
            Serial.println("[MODE] Standby mode (motors off).");
            am_crn_system_set_mode(&g_am_crn, MODE_STANDBY, millis());
            for (uint8_t i = 0; i < NUM_FINGERS; i++) ledcWrite(MOTOR_PINS[i], 0);
        }
    }

    // 1. IMU Acquisition Task at 100 Hz (every 10,000 us)
    static ImuRawSample last_sample;
    if (now_us - g_last_imu_sample_us >= 10000) {
        g_last_imu_sample_us = now_us;
        if (read_imu_sample(&last_sample)) {
            am_crn_system_update_imu(&g_am_crn, &last_sample);
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

    // 3. Live IMU High-Resolution Stream (10 Hz when 'i' is toggled)
    static uint32_t last_stream_ms = 0;
    if (g_stream_imu && (now_ms - last_stream_ms >= 100)) {
        last_stream_ms = now_ms;
        float gating = am_crn_system_get_gating_factor(&g_am_crn);
        MotionState state = am_crn_system_get_motion_state(&g_am_crn);
        const KinematicFeatures* feat = motion_gating_get_features(&g_am_crn.motion_engine);
        float norm_a = sqrtf(last_sample.ax_g * last_sample.ax_g +
                             last_sample.ay_g * last_sample.ay_g +
                             last_sample.az_g * last_sample.az_g);
        float norm_w = sqrtf(last_sample.gx_dps * last_sample.gx_dps +
                             last_sample.gy_dps * last_sample.gy_dps +
                             last_sample.gz_dps * last_sample.gz_dps);

        Serial.printf("[IMU LIVE] Ax:%+0.3fg Ay:%+0.3fg Az:%+0.3fg | |a|:%0.3fg | |w|:%5.1f dps (Gx:%+0.1f Gy:%+0.1f Gz:%+0.1f) | Still:%0.2f | G(t):%0.2f | State:%s\n",
                      last_sample.ax_g, last_sample.ay_g, last_sample.az_g, norm_a,
                      norm_w, last_sample.gx_dps, last_sample.gy_dps, last_sample.gz_dps,
                      feat ? feat->stillness_score : 1.0f, gating,
                      (state <= 5) ? MOTION_STATE_NAMES[state] : "UNKNOWN");
    }

    // 4. Periodic Diagnostics Telemetry (every 350 ms when stream is off)
    if (!g_stream_imu && (now_ms - g_last_telemetry_ms >= 350)) {
        g_last_telemetry_ms = now_ms;
        uint8_t duties[NUM_FINGERS];
        am_crn_system_get_motor_duties(&g_am_crn, duties);
        float gating = am_crn_system_get_gating_factor(&g_am_crn);
        MotionState state = am_crn_system_get_motion_state(&g_am_crn);
        bool tripped = am_crn_system_is_safety_tripped(&g_am_crn);
        const KinematicFeatures* feat = motion_gating_get_features(&g_am_crn.motion_engine);

        Serial.printf("[STATUS] State: %-10s | G(t): %.2f | Duties: [%3u %3u %3u %3u %3u] | Stillness: %.2f | Safety: %s | IMU: %s\n",
                      (state <= 5) ? MOTION_STATE_NAMES[state] : "UNKNOWN",
                      gating, duties[0], duties[1], duties[2], duties[3], duties[4],
                      feat ? feat->stillness_score : 1.0f,
                      tripped ? "TRIPPED" : "OK",
                      g_imu_detected ? (g_imu_calib.is_calibrated ? "CALIBRATED" : "RAW") : "OFFLINE");
    }
}
