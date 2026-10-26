# 20. Vibrotactile patterns using sensitivity prioritisation

- **Authors / Year / Venue:** Luzhnica, Veas, et al. (2017); *ACM International Symposium on Wearable Computers (ISWC)*
- **Identifier:** [10.1145/3123021.3123032](https://dl.acm.org/doi/10.1145/3123021.3123032)
- **Record Type:** Conference Paper (Peer-Reviewed ACM)
- **Local Files:** *(Direct PDF paywalled/unavailable; see DOI / XML)* | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Multi-tactor body-worn vibrotactile array.

## 2. Methodology & Algorithms
Formulated pattern design as an optimization problem prioritizing tactors with higher empirical sensitivity thresholds to maximize overall pattern recognition.

## 3. Reported Findings & Effect Sizes
Sensitivity-prioritized patterns significantly lowered pattern recognition error rates compared to uniform intensity/timing schemes across heterogeneous body locations.

## 4. Limitations & Scope Caveats
Applied to forearm and torso tactors; evaluated static body postures.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Foundational mathematical concept for our `pattern_encoder`: since index and thumb have higher receptor density than little finger, patterns must assign higher salience to less sensitive digits.
