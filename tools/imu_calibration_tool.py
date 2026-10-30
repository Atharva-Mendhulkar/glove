#!/usr/bin/env python3
"""
AM-CRN Glove: Dorsal IMU Calibration & Telemetry Bridge
Connects to ESP32-S3 over /dev/ttyUSB0 at 115200 baud.
Provides live interactive terminal display for IMU calibration,
sensory gating G(t), and motor sequencing.
"""

import sys
import time
import argparse

try:
    import serial
except ImportError:
    sys.exit("Error: pyserial is required. Run with the project Python runtime.")

def main():
    parser = argparse.ArgumentParser(description="AM-CRN Glove IMU Calibration & Telemetry Monitor")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port (default: /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    args = parser.parse_args()

    print("=" * 68)
    print("      AM-CRN GLOVE: DORSAL IMU CALIBRATION & KINEMATICS MONITOR")
    print("=" * 68)
    print(f"Connecting to ESP32-S3 on {args.port} @ {args.baud} baud...")

    try:
        ser = serial.Serial()
        ser.port = args.port
        ser.baudrate = args.baud
        ser.dtr = False
        ser.rts = False
        ser.timeout = 0.1
        ser.open()
    except Exception as e:
        sys.exit(f"Failed to open {args.port}: {e}")

    print("Connected successfully! Listening to real-time glove telemetry...")
    print("\n[KEYBOARD SHORTCUTS IN TERMINAL]")
    print("  'i' + Enter : Toggle High-Resolution Live IMU Stream (10 Hz)")
    print("  'k' + Enter : Run 6-Axis Static Resting Calibration")
    print("  'w' + Enter : Run Full I2C Pin Scanner across all GPIOs")
    print("  't' + Enter : Run 5-Finger Motor Hardware Sweep Test")
    print("  '1'-'5'     : Pulse Individual Finger Motor (Thumb -> Little)")
    print("  'a' + Enter : All Motors ON (2 seconds)")
    print("  's' + Enter : Motors OFF / Standby")
    print("  'c' + Enter : Coordinated Reset Therapeutic Mode")
    print("  'q' + Enter : Quit Monitor\n")
    print("-" * 68)

    try:
        while True:
            line = ser.readline()
            if line:
                try:
                    text = line.decode("utf-8", errors="replace").rstrip()
                    if text:
                        print(text)
                except Exception:
                    pass
    except KeyboardInterrupt:
        print("\nExiting AM-CRN monitor...")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
