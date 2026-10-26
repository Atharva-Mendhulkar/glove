# 11. Neuroadaptive Haptics: Comparing Reinforcement Learning from Explicit Ratings and Neural Signals for Adaptive XR Systems

- **Authors / Year / Venue:** Gehrke, Koselevs, Klug, Gramann (2025); *Frontiers in Virtual Reality / arXiv*
- **Identifier:** [10.3389/frvir.2025.1616442](https://arxiv.org/abs/2504.15984)
- **Record Type:** Peer-Reviewed Journal & arXiv Preprint (Open Access)
- **Local Files:** [Local PDF (`pdfs/11_arxiv_neuroadaptive_haptics_rl.pdf`)](../pdfs/11_arxiv_neuroadaptive_haptics_rl.pdf) | *(No PMC XML)*

---

## 1. Hardware & Sensing Setup
Haptic wearable stimulators, 64-channel EEG system, VR headset.

## 2. Methodology & Algorithms
Reinforcement learning (RL) framework adapting haptic feedback policy using either explicit subjective ratings or implicit neural error potentials (ErrP).

## 3. Reported Findings & Effect Sizes
Both explicit rating-based RL and neural RL successfully converged toward optimal perceptual comfort and task performance; explicit rating bandits proved robust with low data samples.

## 4. Limitations & Scope Caveats
High computational overhead; requires lab-grade EEG equipment for neural branch.

## 5. Direct Relevance to ESP32-S3 Glove (Architecture C)
Direct conceptual foundation for Architecture C's contextual Thompson sampling / bandit layer, utilizing explicit subjective clarity/comfort ratings without needing complex EEG.
