# Hardware Simulation Guide for AM-CRN Glove (ESP32-S3)

You can simulate this hardware setup in **Wokwi** (the official web and VS Code simulator for ESP32-S3).

---

## What the Simulation Emulates
- **ESP32-S3 DevKit-C**: Dual-core processor running the 1 ms scheduler and 5 kHz LEDC PWM.
- **Dorsal MPU-6050 IMU**: Connected over Fast I2C (SDA = GPIO8, SCL = GPIO9). You can drag the interactive orientation/acceleration controls in Wokwi to simulate hand stillness, reaching, or tremor.
- **5 Motor Indicators (LEDs)**:
  - **Thumb**: Green LED on GPIO4
  - **Index**: Blue LED on GPIO5
  - **Middle**: Yellow LED on GPIO6
  - **Ring**: Orange LED on GPIO7
  - **Little**: Red LED on GPIO15
- **Interactive Verification**: As you tilt or shake the MPU-6050 in the simulator, the serial monitor prints the real-time **Sensory Gating Index $G(t)$**, and the LEDs dynamically brighten to compensate for movement!

---

## Option 1: Run Instantly in Browser (Wokwi Web)

1. Open [https://wokwi.com/projects/new/esp32-s3](https://wokwi.com/projects/new/esp32-s3).
2. Replace `sketch.ino` with the contents of [simulation/simulation.ino](file:///home/topfloorboss/Desktop/iot/simulation/simulation.ino).
3. Switch to the `diagram.json` tab and paste the contents of [simulation/diagram.json](file:///home/topfloorboss/Desktop/iot/simulation/diagram.json).
4. Click the green **Play** button to start the simulation.
5. Click on the MPU6050 sensor to change acceleration and watch the LEDs pulse with desynchronizing Coordinated Reset and adapt their brightness to motion.

---

## Option 2: Run in VS Code via Wokwi Extension

1. Install the **Wokwi Simulator** extension in VS Code.
2. Open this repository folder in VS Code.
3. Open `simulation/diagram.json`.
4. Press `F1` and select `Wokwi: Start Simulator`.

---

## Option 3: Terminal Hardware Simulation (Zero Browser Required)

We also built a terminal hardware simulation bridge directly in Python:
```bash
python3 tools/glove_dashboard.py --monitor
```
This renders a live 20 Hz terminal dashboard displaying:
- Current motion state (`STILL`, `SLOW_MOVE`, `REACH`, `TREMOR`)
- Real-time sensory gating multiplier $G(t)$
- 5-channel ASCII PWM duty cycle bars
