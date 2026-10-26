# 08. Navigating with Haptic Gloves: Investigating Strategies for Horizontal and Vertical Movement Guidance

- **Authors / Year / Venue:** Spatial Haptics Team (2025); *arXiv preprint*
- **Identifier:** [arXiv:2503.23484](https://arxiv.org/abs/2503.23484)
- **Record Type:** Preprint (Open Access)
- **Local Files:** [Local PDF (`pdfs/08_arxiv_navigating_with_haptic_gloves.pdf`)](../pdfs/08_arxiv_navigating_with_haptic_gloves.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Five-finger haptic glove with independent actuators per digit.

## 2. Methodology & Algorithms
Investigated spatiotemporal actuation sequences across fingers for directional navigation guidance (horizontal and vertical vectors).

## 3. Reported Findings & Effect Sizes
Sequential finger sweeps and rhythmically staggered activations provide significantly clearer directional cues than simultaneous multi-finger buzzing.

## 4. Limitations & Scope Caveats
Preprint record; evaluated primarily in virtual navigation environments without human-in-the-loop preference optimization.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Direct design guide for our `pattern_encoder`: demonstrates why simultaneous 5-finger activation causes perceptual ambiguity and why staggered sequential patterns are superior.
