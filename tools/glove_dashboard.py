#!/usr/bin/env python3
"""
Host-Side Dashboard, Calibration Bridge, and Benchmark Logger for AM-CRN Glove.
Pure Python 3 standard library (no mandatory external dependencies).
"""

import sys
import os
import time
import json
import math
import random
import argparse

FINGER_NAMES = ["Thumb", "Index", "Middle", "Ring", "Little"]

class MockGloveConnection:
    """Simulates the ESP32-S3 glove firmware for testing when hardware is unplugged."""
    def __init__(self):
        self.mode = "STANDBY"
        self.gating = 1.0
        self.stillness = 1.0
        self.state = 0 # 0=STILL, 1=SLOW, 2=FAST, 3=REACH, 4=TREMOR
        self.duties = [0, 0, 0, 0, 0]
        self.safety = "OK"
        self.sim_time = 0.0
        self.cr_cycle = 0.0
        self.active_finger = -1

    def send_command(self, cmd):
        cmd = cmd.strip()
        if cmd.startswith("MODE "):
            m = cmd[5:]
            self.mode = "CR" if ("CR" in m or "COORDINATED" in m) else m
        elif cmd.startswith("ESTOP"):
            self.safety = "TRIP"
            self.duties = [0, 0, 0, 0, 0]
        elif cmd.startswith("RESET_SAFETY"):
            self.safety = "OK"
        return f"ACK: {cmd}"

    def read_telemetry(self):
        self.sim_time += 0.05
        # Simulate gentle movement waves
        t = self.sim_time
        movement = max(0.0, math.sin(t * 0.8) * 0.7 + math.sin(t * 2.3) * 0.3)
        if movement < 0.15:
            self.state = 0 # STILL
            self.stillness = 1.0 - movement
            self.gating = 1.00
        elif movement < 0.5:
            self.state = 1 # SLOW
            self.stillness = 0.6
            self.gating = 1.25
        else:
            self.state = 3 # REACH
            self.stillness = 0.2
            self.gating = 1.65

        # Simulate Coordinated Reset pulse sequence across the 5 fingers
        if self.mode == "CR" and self.safety == "OK":
            cycle_phase = (self.sim_time % 0.65) / 0.65 # ~650 ms cycle
            active_idx = int(cycle_phase * 5.0)
            if active_idx < 5:
                base_duty = int(110 * self.gating)
                if base_duty > 230: base_duty = 230
                self.duties = [0, 0, 0, 0, 0]
                self.duties[active_idx] = base_duty
            else:
                self.duties = [0, 0, 0, 0, 0]
        else:
            self.duties = [0, 0, 0, 0, 0]

        return f"STATUS state={self.state} gating={self.gating:.2f} still={self.stillness:.2f} duty=[{self.duties[0]},{self.duties[1]},{self.duties[2]},{self.duties[3]},{self.duties[4]}] safety={self.safety}"

class HardwareSerialConnection:
    """Connects to real ESP32-S3 over USB UART."""
    def __init__(self, port, baud=115200):
        try:
            import serial
            self.ser = serial.Serial(port, baud, timeout=0.1)
            time.sleep(1.0)
        except ImportError:
            sys.exit("Error: 'pyserial' not installed. Run 'pip install pyserial' or use '--mock' mode.")

    def send_command(self, cmd):
        self.ser.write((cmd.strip() + "\n").encode("utf-8"))
        return "SENT"

    def read_telemetry(self):
        line = self.ser.readline().decode("utf-8", errors="ignore").strip()
        return line if line.startswith("STATUS") else None

def render_bar(val, max_val=255, width=16):
    filled = int((val / max_val) * width) if max_val > 0 else 0
    return "[" + "#" * filled + " " * (width - filled) + f"] {val:3d}"

def run_monitor(conn, duration=10.0):
    print("\n" + "=" * 65)
    print("      AM-CRN GLOVE: REAL-TIME TELEMETRY MONITOR")
    print("=" * 65)
    print(" Press Ctrl+C to stop monitor.\n")
    start_t = time.time()
    try:
        while time.time() - start_t < duration:
            line = conn.read_telemetry()
            if not line:
                time.sleep(0.05)
                continue

            # Parse line: STATUS state=X gating=X.XX still=X.XX duty=[X,X,X,X,X] safety=XX
            try:
                parts = line.split()
                data = {}
                for p in parts[1:]:
                    k, v = p.split("=")
                    data[k] = v

                state_names = ["STILL", "SLOW_MOVE", "FAST_MOVE", "REACH", "TREMOR", "UNKNOWN"]
                state_idx = int(data.get("state", 0))
                state_str = state_names[state_idx] if state_idx < len(state_names) else "UNKNOWN"
                gating = float(data.get("gating", 1.0))
                stillness = float(data.get("still", 1.0))
                safety = data.get("safety", "OK")
                raw_duties = data.get("duty", "[0,0,0,0,0]").strip("[]").split(",")
                duties = [int(x) for x in raw_duties]

                sys.stdout.write("\033[H\033[J") # Clear terminal screen
                print("=================================================================")
                print(f" Mode: {getattr(conn, 'mode', 'ACTIVE')}  | Motion State: {state_str:<9} | Safety: {safety}")
                print(f" Sensory Gating Index G(t): {gating:.2f}x | Hand Stillness: {stillness*100:.1f}%")
                print("-----------------------------------------------------------------")
                print(" Actuator Channels (PWM Duty 0-255):")
                for i, name in enumerate(FINGER_NAMES):
                    bar = render_bar(duties[i])
                    print(f"   [{i}] {name:<7}: {bar}")
                print("=================================================================")
                print(" Telemetry Rate: ~20 Hz | Press Ctrl+C to exit")
            except Exception:
                pass
            time.sleep(0.05)
    except KeyboardInterrupt:
        print("\nMonitor stopped.")

def run_benchmark(conn, out_log="benchmark_session.jsonl", trials=10):
    print(f"\n[BENCHMARK] Starting {trials} simulated trials...")
    conn.send_command("MODE CR")
    records = []
    for trial_idx in range(trials):
        time.sleep(0.3)
        line = conn.read_telemetry()
        record = {
            "trial_id": trial_idx + 1,
            "timestamp": time.time(),
            "raw_telemetry": line
        }
        records.append(record)
        print(f"  Trial {trial_idx+1:2d}/{trials}: {line}")

    with open(out_log, "w", encoding="utf-8") as f:
        for r in records:
            f.write(json.dumps(r) + "\n")
    print(f"[OK] Logged {len(records)} trials to {out_log}")

def main():
    parser = argparse.ArgumentParser(description="AM-CRN Glove Host Software Tool")
    parser.add_argument("--port", type=str, default="/dev/ttyUSB0", help="Serial port for glove")
    parser.add_argument("--mock", action="store_true", default=True, help="Use mock connection (default: True)")
    parser.add_argument("--hw", action="store_true", help="Connect to physical hardware serial port")
    parser.add_argument("--monitor", action="store_true", help="Run real-time terminal monitor")
    parser.add_argument("--benchmark", action="store_true", help="Run automated benchmark logging session")
    parser.add_argument("--duration", type=float, default=6.0, help="Monitor run duration in seconds")
    parser.add_argument("--cmd", type=str, help="Send raw command to glove")
    args = parser.parse_args()

    if args.hw:
        print(f"[HOST] Connecting to hardware on {args.port}...")
        conn = HardwareSerialConnection(args.port)
    else:
        print("[HOST] Running in simulated (mock) glove mode...")
        conn = MockGloveConnection()

    if args.cmd:
        res = conn.send_command(args.cmd)
        print(f"[CMD] {res}")

    if args.benchmark:
        run_benchmark(conn)
    elif args.monitor or not args.cmd:
        conn.send_command("MODE CR")
        run_monitor(conn, duration=args.duration)

if __name__ == "__main__":
    main()
