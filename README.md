# AM-CRN: Adaptive Motion-Gated Coordinated Reset Neuromodulation Glove

Embedded C99 / FreeRTOS vibrotactile neuromodulation system for an ESP32-S3 microcontroller, dorsal 6-axis IMU, and five coin ERM actuators without contact or flex sensing.

---

## 🎥 Video Demonstration
- **Video Demonstration Folder**: [Google Drive Video Demonstration](https://drive.google.com/drive/folders/1uONN4YGhQNZigE1ExzX5zXttHGENcdZQ?usp=sharing)
- **GitHub Repository**: [https://github.com/Atharva-Mendhulkar/glove.git](https://github.com/Atharva-Mendhulkar/glove.git)

---

## 👥 Authors
- **Atharva Mendhulkar** (Reg. No: 24BIT0103)
- **Ritwij Tripathi** (Reg. No: 24BIT0095)  
*Department of Information Technology, School of Computer Science Engineering and Information Systems (SCORE), Vellore Institute of Technology, Vellore, Tamil Nadu, India*

---

## 📄 Research Paper Report
- **Formal Paper PDF**: [AM_CRN_Research_Paper.pdf](file:///home/topfloorboss/Desktop/iot/AM_CRN_Research_Paper.pdf) (or in [paper/main.pdf](file:///home/topfloorboss/Desktop/iot/paper/main.pdf))
- **LaTeX Source**: [paper/main.tex](file:///home/topfloorboss/Desktop/iot/paper/main.tex) and [paper/references.bib](file:///home/topfloorboss/Desktop/iot/paper/references.bib)

---

## 🛠️ Complete Bill of Materials (BOM)

| #  | Component                       | Quantity | Technical Specification                                          | Subsystem Role                                    |
|:--:|---------------------------------|:--------:|------------------------------------------------------------------|---------------------------------------------------|
| 1  | ESP32-S3 development board      | 1        | Dual-core Xtensa 240 MHz, 512 KB SRAM, 8 MB Flash, 5 kHz PWM    | Main system controller                            |
| 2  | Coin vibration motors           | 5        | Model 1027 pancake ERM, 10 mm $\times$ 2.7 mm, rated 2.3 V       | Cutaneous vibrotactile stimulator (1 per digit)   |
| 3  | IRLZ44N N-channel MOSFET        | 5        | Logic-level ($V_{\text{GS(th)}} = 1.0$--$2.0$ V), TO-220         | Low-side motor power switch (1 per motor)         |
| 4  | Gate series resistors           | 5        | 220 $\Omega$, ¼ W metal film ($\pm 5\%$)                         | Damps GPIO switching inrush and LC ringing        |
| 5  | Gate pulldown resistors         | 5        | 10 k$\Omega$, ¼ W carbon/metal film                              | Bleeds gate charge; prevents floating ON at boot  |
| 6  | Schottky flyback diodes         | 5        | 1N5819, $V_R = 40$ V, $I_F = 1$ A, low $V_F \le 0.45$ V          | Clamps inductive de-energization back-EMF         |
| 7  | Electrolytic bulk capacitor     | 1        | 100 $\mu$F, radial leaded, rated $\ge 6.3$ V (low-ESR)           | Absorbs transient supply sags during motor start  |
| 8  | Ceramic bypass capacitor        | 1        | 100 nF multi-layer ceramic (MLCC)                                | Filters high-frequency PWM switching harmonics    |
| 9  | Regulated motor power supply    | 1        | 2.3 V DC output, continuous $\ge 1.0$ A peak current rating     | Dedicated isolated actuator drive rail            |
| 10 | Main power switch               | 1        | SPST rocker/toggle switch, rated for supply voltage/current      | Master circuit disconnect                         |
| 11 | Battery holder / battery pack   | 1        | 2$\times$AA or 1S LiPo pack with inline linear/buck regulator    | Portable DC energy reservoir                      |
| 12 | ESP32 USB cable                 | 1        | Shielded USB Type-C / Micro-B data cable                         | Program flashing and serial telemetry logging     |
| 13 | Solderless breadboard           | 1        | 830 tie-point dual-bus breadboard                                | Modular rapid prototyping testbed                 |
| 14 | Solid jumper wires              | 1 set    | 22 AWG male-to-male and male-to-female DuPont leads              | Inter-module and bus distribution                 |
| 15 | Flexible hookup wire            | 5 pr.    | 28 AWG multi-strand flexible silicone wire                       | Low-mechanical-impedance fingertip routing        |
| 16 | Wearable glove substrate        | 1        | Form-fitting cotton/synthetic glove                              | Anatomical mounting of distal actuators           |
| 17 | Heat-shrink insulation tubing   | Req.     | Polyolefin heat-shrink sleeves (1.5 mm / 2.5 mm)                 | Terminal insulation and joint strain relief       |

---

## ⚡ Hardware Safety Protocols
1. **Supply Current Headroom**: Dedicated supply capable of handling $\ge 600$\,mA concurrent start inrush for 5 actuators.
2. **MOSFET Gate Drive**: Driven directly by ESP32-S3 3.3\,V GPIO logic levels ($R_{\text{DS(ON)}} \approx 0.045\,\Omega$, $<0.32$\,mW conduction dissipation).
3. **Diode Cathode Orientation**: Silver cathode stripe strictly connected to $+2.3$\,V rail to prevent direct short circuits.
4. **Star-Ground Topology**: Common ground junction uniting MOSFET sources, ESP32 GND, and DC power return to isolate inductive return noise.
5. **Voltage Clamping**: Motor coils strictly regulated to 2.3\,V nominal to protect miniature internal bearings.

---

## 📂 Repository Layout
- `firmware/`: C99/FreeRTOS core engine, state machine, motion gating, and scheduler.
- `simulation/`: Full Wokwi virtual simulation diagram and code (`diagram.json`, `simulation.ino`).
- `paper/`: Formal two-column IEEE-style research paper source (`main.tex`, `references.bib`, `figures/`, `main.pdf`).
- `papers/`: Curated repository of 30 research papers, XML full texts, and critical summaries.
- `tools/`: Python live dashboard and telemetry tools (`glove_dashboard.py`).
- `AM_CRN_Research_Paper.pdf`: Compiled research paper report.
