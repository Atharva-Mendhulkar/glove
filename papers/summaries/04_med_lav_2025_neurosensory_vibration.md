# 04. Neurosensory Response of the Hand and Foot to Vibration Exposure

- **Authors / Year / Venue:** Occupational Health Research Group (2025); *La Medicina del Lavoro (Med Lav)*
- **Identifier:** [10.23749/mdl.v116i3.17149](https://pubmed.ncbi.nlm.nih.gov/40536297/)
- **Record Type:** Primary Research (Peer-Reviewed)
- **Local Files:** *(Direct PDF paywalled/unavailable; see DOI / XML)* | [Local Full-Text XML (`fulltext_xml/04_med_lav_neurosensory_vibration_PMC12199025.xml`)](../fulltext_xml/04_med_lav_neurosensory_vibration_PMC12199025.xml)

---

## 1. Hardware & Sensing Setup
Calibrated vibrotactile perception threshold (VPT) meters, thermal probes.

## 2. Methodology & Algorithms
Measured vibrotactile perception thresholds in 28 healthy participants before, during, and after continuous vibration exposure, evaluating temporary threshold shifts (TTS).

## 3. Reported Findings & Effect Sizes
Observed frequency-, skin-temperature-, and exposure-time-dependent threshold shifts. Sustained vibration leads to sensory desensitization and elevated detection thresholds.

## 4. Limitations & Scope Caveats
Study conducted for occupational exposure safety rather than wearable haptic interaction; healthy cohort only.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Motivates cross-session recalibration, maximum duty-cycle enforcement in `safety_watchdog`, and habituation monitoring in long-duration glove sessions.
