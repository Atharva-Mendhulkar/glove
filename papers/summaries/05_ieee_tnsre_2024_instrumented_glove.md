# 05. An Instrumented Glove for Restoring Sensorimotor Function of the Hand Through Augmented Sensory Feedback

- **Authors / Year / Venue:** Rehabilitation Robotics & Neural Engineering Lab (2024); *IEEE Transactions on Neural Systems and Rehabilitation Engineering (TNSRE)*
- **Identifier:** [10.1109/TNSRE.2024.3415709](https://pubmed.ncbi.nlm.nih.gov/38885098/)
- **Record Type:** Primary Research (Peer-Reviewed Open Access)
- **Local Files:** [Local PDF (`pdfs/05_ieee_tnsre_instrumented_glove.pdf`)](../pdfs/05_ieee_tnsre_instrumented_glove.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Instrumented textile glove with piezoelectric contact sensors and miniature vibrotactile feedback transducers.

## 2. Methodology & Algorithms
Closed-loop, event-driven sensory augmentation triggering vibrotactile cues on touch/grasp contact events; evaluated with 20 healthy subjects and 2 motor-impaired individuals.

## 3. Reported Findings & Effect Sizes
Achieved 96.2% touch-event detection accuracy with a tight 22 ms end-to-end response latency; observed improved coordination in grasping tasks.

## 4. Limitations & Scope Caveats
Relies entirely on piezoelectric contact/release sensors absent on our ESP32-S3 hardware; small clinical pilot.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Gold-standard model for low-latency event-driven haptic scheduling. On our glove, IMU-derived motion inflection points (jerk peaks) substitute for physical contact switches.
