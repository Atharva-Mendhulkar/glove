#!/usr/bin/env python3
"""
AM-CRN Glove: Dorsal IMU Calibration & Telemetry Bridge
Connects to ESP32-S3 over /dev/ttyUSB0 at 115200 baud.
Full bidirectional interactive terminal for IMU calibration,
real-time kinematics streaming, and motor testing.
"""

import sys
import time
import threading
import argparse

try:
    import serial
except ImportError:
    sys.exit("Error: pyserial is required. Run with the project Python runtime.")

def stdin_thread_func(ser, running_flag):
    """Reads keyboard input from user and forwards commands to ESP32-S3."""
    while running_flag[0]:
        try:
            cmd = sys.stdin.readline()
            if not cmd:
                break
            cmd = cmd.strip()
            if not cmd:
                continue
            if cmd.lower() == 'q':
                running_flag[0] = False
                break
            # Send character or command to ESP32
            ser.write((cmd + "\n").encode("utf-8"))
            ser.flush()
            print(f">> [SENT TO GLOVE]: '{cmd}'")
        except Exception as e:
            break

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
    print("\n[INTERACTIVE COMMANDS - Type letter and press Enter]:")
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

    running_flag = [True]
    t = threading.Thread(target=stdin_thread_func, args=(ser, running_flag), daemon=True)
    t.start()

    try:
        while running_flag[0]:
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
        running_flag[0] = False
        ser.close()

if __name__ == "__main__":
    main()
