# Adaptive Motion-Gated Coordinated Reset Neuromodulation (AM-CRN)
## Architecture Specification & Theoretical Grounding for ESP32-S3 Five-Finger Glove

---

### Executive Summary

Most wearable vibrotactile gloves fail in neuromodulation and rehabilitation because of two unaddressed neurophysiological barriers documented in recent literature:
1. **Sensory Gating Under Motion**: Active movement attenuates somatosensory evoked potentials and elevates detection thresholds by 30–60% ([Tajdari et al., 2024](file:///home/topfloorboss/Desktop/iot/papers/summaries/03_human_factors_2024_motion_sensitivity.md)). Static stimulation protocols become sub-threshold during reaching and cause sensory overload at rest.
2. **Receptor Habituation & Temporary Threshold Shifts**: Continuous periodic vibration drives rapid mechanoreceptor desensitization and cortical adaptation ([Medicina del Lavoro, 2025](file:///home/topfloorboss/Desktop/iot/papers/summaries/04_med_lav_neurosensory_vibration_PMC12199025.md)), degrading therapeutic efficacy within minutes.

Furthermore, classical **Coordinated Reset (CR)** stimulation—which delivers phase-staggered pulses across digits to desynchronize pathologically synchronous neural populations—assumes ideal square-wave pulses. Physical coin Eccentric Rotating Mass (ERM) motors have mechanical rise times of 30–50 ms and fall times of 40–80 ms, smearing the intended phase shifts unless explicitly compensated.

**AM-CRN** (Adaptive Motion-Gated Coordinated Reset Neuromodulation) is a constrained, zero-extra-sensor architecture designed specifically for an **ESP32-S3**, **one 6-axis dorsal IMU**, and **five finger-mounted ERM motors**. It achieves closed-loop neuromodulation through:
- **Kinematic Gating Compensation**: Computes an instantaneous Gating Index $G_k \in [1.0, 2.2]$ from dorsal IMU jerk and RMS acceleration, dynamically modulating PWM duty cycle to maintain constant somatosensory afference.
- **Inertia-Compensated Coordinated Reset**: Actively coordinates five finger channels with pseudo-random inter-stimulus jitter (avoiding habituation) and pulse overdrive/settling management tailored to coin ERM mechanical time constants.
- **Micro-Staircase Calibration**: A ≤60 s per-finger perceptual thresholding protocol that individualizes baseline intensities to account for non-uniform peripheral receptor innervation.
- **Deterministic Embedded Footprint**: Zero dynamic allocations on the real-time path; runs within FreeRTOS tasks consuming <8% of one ESP32-S3 CPU core and <16 KB RAM.

---

### 1. Hardware Component Physics & Constraints

| Component | Physical Behavior | Constraint & Mitigation in AM-CRN |
|-----------|-------------------|-----------------------------------|
| **ESP32-S3 SoC** | Dual-core Xtensa LX7 @ 240 MHz, 512 KB SRAM, hardware LEDC PWM (8 channels, up to 14-bit). | Real-time loop runs on Core 1; telemetry and calibration on Core 0. Memory allocated statically at boot. |
| **5x Coin ERMs (1027/1030)** | Coupled frequency-voltage curve (~150–220 Hz at 2.5–3.3V). Mechanical rise time $t_{rise} \approx 35\text{ ms}$, decay time $t_{decay} \approx 50\text{ ms}$. | Cannot modulate frequency independently of amplitude. AM-CRN treats *duration, pulse spacing, and finger sequence* as the primary degrees of freedom, applying a 20 ms overdrive kick to cut rise time. |
| **AO3400A N-MOSFETs** | Low $R_{DS(ON)} \approx 28\text{ m}\Omega$, gate threshold $V_{GS(th)} \approx 0.65–1.2\text{V}$, driven directly by 3.3V GPIOs. | No current-shunt feedback. System enforces duty-cycle clamping ($\le 40\%$ average over 5s) and watchdog shutdowns to prevent thermal runaway. |
| **Single 6-Axis IMU** | Dorsal hand mounting, 100 Hz I2C/SPI sampling ($\pm 4g$, $\pm 1000^\circ/\text{s}$). | Measures hand kinematics, not skin contact or perceived vibration. Kinematics serve strictly as a *sensory-gating estimator* and *safety interlock*. |

---

### 2. Neurobiological Foundations & Literature Gaps

```
                  ┌──────────────────────────────────────────────┐
                  │          Sensorimotor Cortex (S1 / M1)       │
                  │   Pathological synchrony / Spasticity / Pain │
                  └───────────────────────▲──────────────────────┘
                                          │ Coordinated Afferent Volleys
                                          │ (Desynchronizing Effect)
                  ┌───────────────────────┴──────────────────────┐
                  │     Adaptive Motion-Gated CR Engine          │
                  │  - Phase-staggered digit bursts (1..5)       │
                  │  - Habituation-breaking jitter               │
                  │  - Motion Gating Modulation: I_eff = I_0 * G │
                  └──────▲──────────────────────────────▲────────┘
                         │                              │
         ┌───────────────┴────────┐            ┌────────┴──────────────┐
         │ Kinematic Motion State │            │ Per-Finger Baseline   │
         │ Dorsal IMU @ 100 Hz    │            │ Micro-Staircase (≤60s)│
         └────────────────────────┘            └───────────────────────┘
```

#### Gap 1: Sensorimotor Gating Mitigation
*Literature Basis*: [Tajdari et al. (2024)](file:///home/topfloorboss/Desktop/iot/papers/summaries/03_human_factors_2024_motion_sensitivity.md) demonstrated that voluntary limb movement significantly degrades vibrotactile detection. The central nervous system suppresses expected sensory feedback during motor execution via efference copy mechanisms.
*AM-CRN Solution*: Computes an instantaneous kinematic activity norm $a_{norm} = \sqrt{a_x^2 + a_y^2 + a_z^2} - 1.0g$ and jerk $j = \frac{d|\vec{a}|}{dt}$. The modulation factor is:
$$G(t) = 1.0 + \alpha \cdot \text{clamp}\left(\frac{a_{norm}}{a_{max}}, 0, 1\right) + \beta \cdot \text{clamp}\left(\frac{|j|}{j_{max}}, 0, 1\right)$$
During quiet rest, $G(t) = 1.0$. During active transport or reaching, $G(t)$ smoothly scales up to $2.0$, preserving constant effective cortical salience without manual intervention.

#### Gap 2: Habituation & Receptor Fatigue
*Literature Basis*: [Medicina del Lavoro (2025)](file:///home/topfloorboss/Desktop/iot/papers/summaries/04_med_lav_neurosensory_vibration_PMC12199025.md) documented significant temporary threshold shifts (TTS) under regular periodic vibration.
*AM-CRN Solution*: The Coordinated Reset sequence incorporates bounded pseudo-random phase jitter:
$$\Delta t_{burst} = T_{base} + \mathcal{U}(-\delta, +\delta), \quad \delta \in [0.15, 0.25] \cdot T_{base}$$
In addition, stimulation blocks (typically 12–15 s) are interspersed with randomized pause epochs (3–5 s), resetting receptor adaptation curves.

#### Gap 3: Multi-Finger Masking Prevention
*Literature Basis*: [Kusnir et al. (2024)](file:///home/topfloorboss/Desktop/iot/papers/summaries/18_app_2024_contralateral_interference.md) and [Luzhnica et al. (2017)](file:///home/topfloorboss/Desktop/iot/papers/summaries/20_acm_2017_sensitivity_prioritisation.md) showed that simultaneous vibrotactile bursts across digits trigger lateral inhibitory masking, collapsing pattern discriminability.
*AM-CRN Solution*: AM-CRN enforces strict *orthogonal time multiplexing*. No two ERM channels are energized simultaneously. Inter-digit pulse delays are kept $\ge 40\text{ ms}$ (exceeding tactile temporal resolution limits of 20–30 ms).

---

### 3. AM-CRN Architecture Pipeline

```text
+---------------------------------------------------------------------------------------+
| Core 1 Real-Time Pipeline (FreeRTOS)                                                  |
|                                                                                       |
|  [IMU Sample @ 100 Hz]                                                                |
|          │                                                                            |
|          ▼                                                                            |
|  [Preprocessing & Gravity Compensation]                                               |
|          │                                                                            |
|          ▼                                                                            |
|  [Feature Extractor] (RMS, Jerk, Stillness Score)                                     |
|          │                                                                            |
|          ▼                                                                            |
|  [Gating & State Estimator] ──► Motion State (STILL / SLOW / FAST / REACH)           |
|                                 Gating Factor G(t) in [1.0, 2.2]                      |
|                                         │                                             |
|  [Pattern Generator]                    │                                             |
|   - Coordinated Reset (CR) Sequences    │                                             |
|   - Stochastic Resonance Cues           │                                             |
|   - Finger Priority Mask                │                                             |
|          │                              │                                             |
|          ▼                              │                                             |
|  [Dynamic Amplitude Modulator] ◄────────┘                                             |
|   Duty[finger] = BaseDuty[finger] * G(t)                                              |
|          │                                                                            |
|          ▼                                                                            |
|  [ERM Mechanical Compensation]                                                        |
|   - 20 ms overdrive pulse at onset                                                    |
|   - Enforce ≥40 ms inter-pulse settling window                                        |
|          │                                                                            |
|          ▼                                                                            |
|  [Safety Watchdog Interlock]                                                          |
|   - Max continuous on-time ≤250 ms                                                    |
|   - Duty cycle ceiling ≤40% over 5s window                                            |
|   - Emergency cutoff if IMU freefall or fault                                         |
|          │                                                                            |
|          ▼                                                                            |
|  [LEDC Hardware Timers] ──► AO3400A MOSFETs ──► 5x Finger ERMs                        |
+---------------------------------------------------------------------------------------+
| Core 0 Asynchronous Control & Telemetry                                               |
|  - Micro-staircase perceptual calibration manager                                     |
|  - Serial/BLE command parsing & streaming diagnostics                                 |
+---------------------------------------------------------------------------------------+
```

---

### 4. Mathematical Formulations

#### 4.1 Kinematic Sensory Gating Factor
Given discrete acceleration samples $\mathbf{a}_k = (a_{x,k}, a_{y,k}, a_{z,k})$ at $\Delta t = 10\text{ ms}$:
1. Dynamic acceleration norm:
   $$d_k = \left| \|\mathbf{a}_k\|_2 - 1.0 \right|$$
2. First-order filtered jerk:
   $$j_k = \frac{|d_k - d_{k-1}|}{\Delta t}$$
3. Exponential moving average smoothing ($\lambda = 0.85$):
   $$\bar{d}_k = \lambda \bar{d}_{k-1} + (1 - \lambda) d_k, \quad \bar{j}_k = \lambda \bar{j}_{k-1} + (1 - \lambda) j_k$$
4. Gating modulation index:
   $$G_k = 1.0 + \min\left(1.0, \frac{\bar{d}_k}{0.8g}\right) \times 0.7 + \min\left(1.0, \frac{\bar{j}_k}{15.0g/\text{s}}\right) \times 0.5$$
   $$G_k \in [1.0, 2.2]$$

#### 4.2 Inertia-Compensated ERM Overdrive
Because ERMs require time to overcome static friction and rotational inertia:
$$\text{LEDC\_Duty}(t) = \begin{cases} 
\min(255, 1.35 \times \text{Duty}_{target}), & 0 \le t < 20\text{ ms} \quad (\text{Overdrive}) \\
\text{Duty}_{target}, & 20\text{ ms} \le t < t_{duration} \\
0, & t \ge t_{duration}
\end{cases}$$

#### 4.3 Coordinated Reset Pattern Spacing
For $N = 5$ fingers with primary cycle frequency $f_{CR} \in [1.0, 3.0\text{ Hz}]$:
$$T_{cycle} = \frac{1000}{f_{CR}}\text{ ms}$$
Sub-burst duration per finger $D = 80\text{ ms}$.
Stagger interval:
$$\tau_i = \frac{T_{cycle}}{5} \cdot \pi(i) + \mathcal{U}(-15\text{ ms}, +15\text{ ms})$$
where $\pi$ is a pseudo-random permutation of $\{0, 1, 2, 3, 4\}$ reshuffled every cycle.
This ensures non-overlapping stimulation, active phase dispersion, and immunity to habituation.
