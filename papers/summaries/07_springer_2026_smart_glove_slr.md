# 07. AI-based smart glove for hand movement recognition and rehabilitation: systematic literature review

- **Authors / Year / Venue:** Systematic Review Group (2026); *BioMedical Engineering OnLine (Springer)*
- **Identifier:** [10.1186/s44147-026-01084-6](https://link.springer.com/article/10.1186/s44147-026-01084-6)
- **Record Type:** Systematic Review (Peer-Reviewed Open Access)
- **Local Files:** [Local PDF (`pdfs/07_springer_smart_glove_systematic_review.pdf`)](../pdfs/07_springer_smart_glove_systematic_review.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Survey of 101 smart glove systems (2011–2025), specifically reviewing ESP32 microcontrollers, IMUs, flex sensors, and vibrotactile feedback.

## 2. Methodology & Algorithms
PRISMA systematic review benchmarking sensing modalities, machine learning pipelines, embedded platforms, and clinical validation standards.

## 3. Reported Findings & Effect Sizes
Identifies ESP32 as a leading platform for real-time inference (citing up to 98.30% accuracy with Random Forest/SVM); reveals that >85% of prior work relies on fragile flex sensors, creating a gap for robust IMU-only designs.

## 4. Limitations & Scope Caveats
Secondary synthesis of prior studies; heterogeneous evaluation criteria across literature.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Confirms the viability of ESP32-S3 for on-glove inference and highlights our key novelty: eliminating brittle flex sensors in favor of motion-conditioned tactile adaptation.
