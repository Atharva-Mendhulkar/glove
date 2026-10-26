# 02. Machine Learning-Based Gesture Recognition Glove: Design and Implementation

- **Authors / Year / Venue:** Sensors Research Team (2024); *Sensors (MDPI)*
- **Identifier:** [10.3390/s24186157](https://pubmed.ncbi.nlm.nih.gov/39338902/)
- **Record Type:** Primary Research (Peer-Reviewed)
- **Local Files:** *(Direct PDF paywalled/unavailable; see DOI / XML)* | [Local Full-Text XML (`fulltext_xml/02_sensors_ml_gesture_glove_PMC11435450.xml`)](../fulltext_xml/02_sensors_ml_gesture_glove_PMC11435450.xml)

---

## 1. Hardware & Sensing Setup
Smart glove equipped with 5 flex sensors, 5 force/tactile sensors, and a 6-axis IMU.

## 2. Methodology & Algorithms
Data acquisition pipeline coupled with 2D Convolutional Neural Network (CNN) for static and dynamic hand gesture classification.

## 3. Reported Findings & Effect Sizes
Achieved ~90% gesture classification accuracy across tested hand movements, confirming feasibility of on-glove gesture inference.

## 4. Limitations & Scope Caveats
Relies on 10 auxiliary finger bend/force sensors unavailable on an IMU-only glove; small participant sample; higher inference compute cost.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Provides baseline neural network architectures and features for glove gesture classification; informs our IMU-only temporal feature extractor (RMS, jerk, stillness).
