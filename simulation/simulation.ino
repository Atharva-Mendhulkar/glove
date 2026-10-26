/**
 * AM-CRN Glove: Hardware Simulation Sketch for Wokwi (ESP32-S3)
 * Simulates:
 *   - 1x Dorsal MPU-6050 IMU on Fast I2C (SDA=GPIO8, SCL=GPIO9)
 *   - 5x ERM Motor indicator LEDs on GPIO4, 5, 6, 7, 15
 *   - Real-time kinematic sensory-gating compensation
 *   - Desynchronizing Coordinated Reset (CR) pulses
 */

#include <Arduino.h>
#include <Wire.h>

#define NUM_FINGERS 5
const uint8_t MOTOR_PINS[NUM_FINGERS] = {4, 5, 6, 7, 15}; // Thumb, Index, Middle, Ring, Little
const uint8_t IMU_ADDR = 0x68;

// --- AM-CRN Core Logic ---
struct Kinematics {
    float dyn_acc_norm;
    float filtered_norm;
    float jerk;
    float gating_factor;
    int motion_state; // 0=STILL, 1=SLOW, 2=FAST, 3=REACH
};

static Kinematics g_kin = {0.0f, 0.0f, 0.0f, 1.0f, 0};
static float g_prev_norm = 0.0f;
static uint32_t g_last_imu_ms = 0;

static inline float clampf(float v, float min_v, float max_v) {
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

// Simple LCG random generator
static uint32_t g_seed = 0xABCD1234;
static uint32_t lcg_rand() {
    g_seed = (g_seed * 1664525u + 1013904223u);
    return g_seed;
}

// Coordinated Reset Timing & Sequence State
static uint8_t g_finger_seq[NUM_FINGERS] = {0, 1, 2, 3, 4};
static uint32_t g_cycle_start_ms = 0;
static const uint32_t CYCLE_PERIOD_MS = 650;
static const uint16_t BURST_MS = 75;
static const uint16_t SPACING_MS = 45;

static uint8_t g_active_duties[NUM_FINGERS] = {0};

void shuffle_fingers() {
    for (int i = NUM_FINGERS - 1; i > 0; i--) {
        int j = lcg_rand() % (i + 1);
        uint8_t t = g_finger_seq[i];
        g_finger_seq[i] = g_finger_seq[j];
        g_finger_seq[j] = t;
    }
}

void update_kinematics(float ax, float ay, float az, float gx, float gy, float gz, float dt_s) {
    (void)gx; (void)gy; (void)gz;
    float acc_mag = sqrtf(ax * ax + ay * ay + az * az);
    float dyn_norm = fabsf(acc_mag - 1.0f);
    float raw_jerk = (dt_s > 0) ? (fabsf(dyn_norm - g_prev_norm) / dt_s) : 0.0f;
    g_prev_norm = dyn_norm;

    // EMA smoothing
    g_kin.filtered_norm = (0.85f * g_kin.filtered_norm) + (0.15f * dyn_norm);
    g_kin.jerk = (0.85f * g_kin.jerk) + (0.15f * raw_jerk);

    // Gating index G(t) in [1.0, 2.2]
    float acc_contrib = clampf(g_kin.filtered_norm / 0.80f, 0.0f, 1.0f) * 0.70f;
    float jerk_contrib = clampf(g_kin.jerk / 15.0f, 0.0f, 1.0f) * 0.50f;
    g_kin.gating_factor = clampf(1.0f + acc_contrib + jerk_contrib, 1.0f, 2.20f);

    if (g_kin.filtered_norm < 0.06f) {
        g_kin.motion_state = 0; // STILL
    } else if (g_kin.jerk > 7.0f) {
        g_kin.motion_state = 3; // REACH
    } else if (g_kin.filtered_norm > 0.55f) {
        g_kin.motion_state = 2; // FAST
    } else {
        g_kin.motion_state = 1; // SLOW
    }
}

bool read_mpu6050(float* ax, float* ay, float* az, float* gx, float* gy, float* gz) {
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint16_t)IMU_ADDR, (uint8_t)14) != 14) return false;

    int16_t ax_raw = (Wire.read() << 8) | Wire.read();
    int16_t ay_raw = (Wire.read() << 8) | Wire.read();
    int16_t az_raw = (Wire.read() << 8) | Wire.read();
    Wire.read(); Wire.read(); // Skip temp
    int16_t gx_raw = (Wire.read() << 8) | Wire.read();
    int16_t gy_raw = (Wire.read() << 8) | Wire.read();
    int16_t gz_raw = (Wire.read() << 8) | Wire.read();

    *ax = (float)ax_raw / 16384.0f; // MPU6050 default +/- 2g scale
    *ay = (float)ay_raw / 16384.0f;
    *az = (float)az_raw / 16384.0f;
    *gx = (float)gx_raw / 131.0f;
    *gy = (float)gy_raw / 131.0f;
    *gz = (float)gz_raw / 131.0f;
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=======================================================");
    Serial.println("  AM-CRN GLOVE: ESP32-S3 HARDWARE SIMULATION (WOKWI)");
    Serial.println("=======================================================");

    // 1. Initialize PWM Pins for LEDs
    for (int i = 0; i < NUM_FINGERS; i++) {
        pinMode(MOTOR_PINS[i], OUTPUT);
        ledcAttach(MOTOR_PINS[i], 5000, 8); // 5 kHz, 8-bit
        ledcWrite(MOTOR_PINS[i], 0);
    }
    Serial.println("[OK] 5x Motor LED channels initialized on GPIO 4, 5, 6, 7, 15");

    // 2. Initialize I2C for MPU-6050 (SDA=GPIO8, SCL=GPIO9)
    Wire.begin(8, 9, 400000);
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(0x6B); // Power Management 1
    Wire.write(0x00); // Wake up MPU-6050
    Wire.endTransmission();
    Serial.println("[OK] MPU-6050 IMU initialized on I2C (SDA=8, SCL=9)");
    Serial.println("[INFO] Click the MPU6050 in simulator to drag acceleration/gyro values!");
    Serial.println("[INFO] Watch the LEDs brightness scale as motion increases!\n");

    g_cycle_start_ms = millis();
    shuffle_fingers();
}

void loop() {
    uint32_t now_ms = millis();

    // 1. Sample IMU @ 100 Hz (every 10 ms)
    if (now_ms - g_last_imu_ms >= 10) {
        float dt_s = (float)(now_ms - g_last_imu_ms) * 0.001f;
        g_last_imu_ms = now_ms;
        float ax, ay, az, gx, gy, gz;
        if (read_mpu6050(&ax, &ay, &az, &gx, &gy, &gz)) {
            update_kinematics(ax, ay, az, gx, gy, gz, dt_s);
        }
    }

    // 2. Generate Coordinated Reset Pulses
    if (now_ms - g_cycle_start_ms >= CYCLE_PERIOD_MS) {
        g_cycle_start_ms = now_ms;
        shuffle_fingers();
    }

    uint32_t elapsed_in_cycle = now_ms - g_cycle_start_ms;
    for (int i = 0; i < NUM_FINGERS; i++) g_active_duties[i] = 0;

    for (int i = 0; i < NUM_FINGERS; i++) {
        uint32_t pulse_start = i * (BURST_MS + SPACING_MS);
        uint32_t pulse_end = pulse_start + BURST_MS;

        if (elapsed_in_cycle >= pulse_start && elapsed_in_cycle < pulse_end) {
            uint8_t finger = g_finger_seq[i];
            float base_duty = 110.0f; // Baseline duty
            // Apply Sensory Gating Index G(t)
            float scaled_duty = base_duty * g_kin.gating_factor;

            // Apply 20 ms ERM Overdrive Kick at pulse onset
            if ((elapsed_in_cycle - pulse_start) < 20) {
                scaled_duty *= 1.35f;
            }
            if (scaled_duty > 255.0f) scaled_duty = 255.0f;
            g_active_duties[finger] = (uint8_t)scaled_duty;
            break; // Non-overlapping single finger
        }
    }

    // 3. Write duties to LED PWM outputs
    for (int i = 0; i < NUM_FINGERS; i++) {
        ledcWrite(MOTOR_PINS[i], g_active_duties[i]);
    }

    // 4. Print Telemetry (every 250 ms)
    static uint32_t last_print = 0;
    if (now_ms - last_print >= 250) {
        last_print = now_ms;
        const char* states[] = {"STILL", "SLOW_MOVE", "FAST_MOVE", "REACH"};
        const char* state_str = (g_kin.motion_state < 4) ? states[g_kin.motion_state] : "UNKNOWN";
        Serial.printf("[AM-CRN] State: %-9s | Gating G(t): %.2fx | Duties: [%3u %3u %3u %3u %3u]\n",
                      state_str, g_kin.gating_factor,
                      g_active_duties[0], g_active_duties[1], g_active_duties[2],
                      g_active_duties[3], g_active_duties[4]);
    }
}
