# 01. Adaptive tactile interaction transfer via digitally embroidered smart gloves

- **Authors / Year / Venue:** Yang et al. (2024); *Nature Communications*
- **Identifier:** [10.1038/s41467-024-45059-8](https://www.nature.com/articles/s41467-024-45059-8)
- **Record Type:** Primary Research (Peer-Reviewed)
- **Local Files:** [Local PDF (`pdfs/01_nature_comms_adaptive_tactile_glove.pdf`)](../pdfs/01_nature_comms_adaptive_tactile_glove.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Digitally embroidered textile glove with integrated piezoresistive/pressure tactile sensors and miniature vibrotactile actuators across fingers.

## 2. Methodology & Algorithms
Machine learning pipeline that captures individual tactile perception responses through active tactile feedback. Calibrates to new users using as little as 15 seconds of interaction data.

## 3. Reported Findings & Effect Sizes
Per-user haptic modeling significantly reduces tactile skill transfer latency. Demonstrates that rapid human-in-the-loop adaptation can compensate for non-uniform fingertip sensitivity.

## 4. Limitations & Scope Caveats
Focuses on tactile skill transfer and manipulation; relies on high-density textile tactile sensing rather than IMU-only motion estimation; does not address 5-channel ERM resonant dynamics.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Closest academic precedent for rapid (≤60s) haptic personalization. Grounds the calibration module in Architecture C and serves as primary benchmark for low-data adaptation.
