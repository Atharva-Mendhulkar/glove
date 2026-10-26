# 12. Real-time slip detection with friction-scaled vibrotactile feedback for robotic sixth-finger-assisted manipulation

- **Authors / Year / Venue:** Afzal et al. (2025); *Robotica (Cambridge University Press)*
- **Identifier:** [10.1017/S0263574725102841](https://www.cambridge.org/core/journals/robotica)
- **Record Type:** Peer-Reviewed Journal (Open Access)
- **Local Files:** [Local PDF (`pdfs/12_cambridge_robotica_realtime_slip_detection.pdf`)](../pdfs/12_cambridge_robotica_realtime_slip_detection.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Supernumerary robotic finger, dynamic tactile sensor, vibrotactile feedback motor.

## 2. Methodology & Algorithms
Real-time friction and slip estimation modulating vibrotactile frequency and amplitude proportionally to incipient slip velocity.

## 3. Reported Findings & Effect Sizes
Continuous friction scaling prevents grasp failure and produces intuitive sensory substitution without overwhelming the wearer with binary on/off alerts.

## 4. Limitations & Scope Caveats
Tested on robotic third-party finger rather than standard human anatomical digits.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Demonstrates the necessity of continuous intensity scaling as a function of motion dynamics, directly supporting our motion-context policy scaling.
