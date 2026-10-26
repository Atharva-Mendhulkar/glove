# 14. Role of haptic feedback technologies and novel engineering developments for surgical training and robot-assisted surgery

- **Authors / Year / Venue:** Boul-Atarass et al. (2025); *Frontiers in Robotics and AI*
- **Identifier:** [10.3389/frobt.2025.1567955](https://www.frontiersin.org/journals/robotics-and-ai/articles/10.3389/frobt.2025.1567955/full)
- **Record Type:** Review Article (Peer-Reviewed Open Access)
- **Local Files:** [Local PDF (`pdfs/14_frontiers_surgical_training_haptics.pdf`)](../pdfs/14_frontiers_surgical_training_haptics.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Comprehensive review of wearable haptic masters, tactile gloves, and vibrotactile displays in high-stakes robotic surgery.

## 2. Methodology & Algorithms
Survey of latency, frequency response, stability, and human factors requirements for teleoperation and robotic simulation.

## 3. Reported Findings & Effect Sizes
End-to-end feedback latency must remain strictly below 50 ms to avoid sensorimotor instability; excessive vibration induces hand fatigue and sensory degradation within 10–15 minutes.

## 4. Limitations & Scope Caveats
Focused on teleoperated surgical consoles rather than standalone embedded wearable gloves.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Provides strict engineering guardrails for our glove: hard latency bound (<50 ms) and thermal/duty limits in `safety_watchdog` to prevent user sensory fatigue.
