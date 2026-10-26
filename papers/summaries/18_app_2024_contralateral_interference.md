# 18. Hello from the other side: Robust contralateral interference in tactile detection

- **Authors / Year / Venue:** Kusnir, Pesin, Landau (2024); *Attention, Perception, & Psychophysics (Springer)*
- **Identifier:** [10.3758/s13414-023-02801-6](https://link.springer.com/article/10.3758/s13414-023-02801-6)
- **Record Type:** Primary Psychophysical Research (Peer-Reviewed Open Access)
- **Local Files:** [Local PDF (`pdfs/18_springer_contralateral_interference.pdf`)](../pdfs/18_springer_contralateral_interference.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Computer-controlled mechanical vibrotactile tactors applied to fingertips of both hands.

## 2. Methodology & Algorithms
Two-alternative forced-choice (2AFC) detection tasks measuring perceptual threshold changes when simultaneous tactile cues are delivered to opposing fingers.

## 3. Reported Findings & Effect Sizes
Demonstrates robust tactile interference: concurrent stimulation across digits degrades detection accuracy by 15–28% and increases perceptual confusion.

## 4. Limitations & Scope Caveats
Investigated contralateral (cross-hand) fingers; wearable gloves also suffer from ipsilateral adjacent-finger bone conduction interference.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Critical foundation for `pattern_encoder`: proves that simultaneous multi-finger buzzing must be avoided. Channels must use temporal staggering (e.g., 30–50 ms offset).
