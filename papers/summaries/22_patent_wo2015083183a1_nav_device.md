# 22. Hand wearable haptic feedback based navigation device

- **Authors / Year / Venue:** Tata Consultancy Services Ltd. (2015); *WIPO PCT Application WO 2015/083183 A1*
- **Identifier:** [WO2015083183A1](https://patents.google.com/patent/WO2015083183A1/en)
- **Record Type:** Published PCT Patent Application
- **Local Files:** [Local PDF (`pdfs/22_patent_WO2015083183A1_navigation_device.pdf`)](../pdfs/22_patent_WO2015083183A1_navigation_device.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Wearable glove with five fingertip vibration motors and distance/obstacle sensors.

## 2. Methodology & Algorithms
Fingertip vibration motors driven proportionally to obstacle distance to provide directional navigation cues.

## 3. Reported Findings & Effect Sizes
Validates directional mapping across 5 digits (thumb to pinky) for orientation guidance.

## 4. Limitations & Scope Caveats
Relies on external ultrasonic/IR distance sensors; fixed rule-based mapping without user adaptation.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Prior art for 5-finger spatial mapping. Our system differentiates by using internal IMU motion states rather than external distance sensors.
