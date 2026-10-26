# 24. Haptic feedback glove

- **Authors / Year / Venue:** Sony Interactive Entertainment Inc. (2018); *US Patent Application US 2018/0335842 A1*
- **Identifier:** [US20180335842A1](https://patents.google.com/patent/US20180335842A1/en)
- **Record Type:** Published Patent Application (USPTO)
- **Local Files:** [Local PDF (`pdfs/24_patent_US20180335842A1_haptic_feedback_glove.pdf`)](../pdfs/24_patent_US20180335842A1_haptic_feedback_glove.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Glove with vibration actuators configured for operation across 20–300 Hz.

## 2. Methodology & Algorithms
Drive methods generating varying virtual texture sensations through frequency and amplitude modulation.

## 3. Reported Findings & Effect Sizes
Specifies optimal actuation envelopes for human cutaneous mechanoreceptor excitation (Meissner vs. Pacinian corpuscles).

## 4. Limitations & Scope Caveats
Designed around voice-coil or LRA actuators; our coin ERM motors couple frequency and amplitude to PWM voltage.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Provides mechanoreceptor frequency guidelines, dictating that our ERM motors should be driven near their 200 Hz resonance for maximum Pacinian sensitivity.
