# AM-CRN LaTeX Research Paper Report

This directory contains the formal two-column research paper report for the **AM-CRN Glove** project.

## Authors
- **Atharva Mendhulkar** (Reg. No: 24BIT0103)
- **Ritwij Tripathi** (Reg. No: 24BIT0095)

---

## VTOP Academic Report Structure
The research paper report strictly follows the 8-part VTOP submission format:
1. **Problem statement**: Sensory gating attenuation ($A \sim e^{-\alpha v}$), mechanical rotor latency ($t_{\text{rise}} \approx 35\,$ms), and single-digit tactile masking during neuromodulation.
2. **Survey**: Survey of neurobiological foundations, desynchronization mechanics (Tass et al., Popowych), and comprehensive 6-item research gap matrix (G1--G6).
3. **Comparison with available products in market**: Detailed competitive benchmarking against CyberGlove III, SenseGlove Nova 2, Neofect RAPAEL, and open Arduino glove systems.
4. **Module description**: Breakdown of Modules 1 through 6 (Kinematic Sensing, Dynamic Gating, Micro-Staircase Calibration, Desynchronization Engine, Deterministic Safety Watchdog, Low-Side Power Driver).
5. **Components used**: Complete 17-item Bill of Materials (BOM), per-motor breakdown, shared power infrastructure, and discrete component photographic gallery.
6. **Circuit diagram**: Full TikZ vector schematic of the low-side IRLZ44N driver circuit, anti-parallel 1N5819 flyback protection, and 5 mandatory pre-power safety checks.
7. **Clear and detailed description of the modules with screenshots and figures**: End-to-end 4-stage pipeline (Figure 3), physical prototype testbed photograph (Figure 4), 5-channel timing oscillogram (Figure 5), and experimental benchmarking results.
8. **Video link of the complete demo step by step clearly**: Google Drive video demonstration link with a 6-step walkthrough of the physical system demonstration.

---

## File Structure
- [main.tex](file:///home/topfloorboss/Desktop/iot/paper/main.tex): Complete LaTeX research paper (including equations, two-column layout, BOM table, physical prototype figures, and built-in TikZ vector diagrams for System Architecture, Timing, and Driver Circuitry).
- [references.bib](file:///home/topfloorboss/Desktop/iot/paper/references.bib): BibTeX bibliography with full citations for all peer-reviewed papers and patents.
- `figures/`: Hardware prototype photographs and discrete electromechanical component images.
- [main.pdf](file:///home/topfloorboss/Desktop/iot/paper/main.pdf): Compiled high-resolution 2-column research paper PDF.

---

## How to Compile to PDF

### Method 1: Overleaf (1-Click Online)
1. Open [Overleaf](https://www.overleaf.com).
2. Click **New Project** $\to$ **Upload Project**.
3. Zip or upload `main.tex` and `references.bib`.
4. Click **Recompile**.

### Method 2: Local Compilation (Linux / macOS / Windows)
If you have TeX Live, MacTeX, or MiKTeX installed:
```bash
pdflatex main.tex
bibtex main
pdflatex main.tex
pdflatex main.tex
```
Or with `latexmk`:
```bash
latexmk -pdf main.tex
```

---

## Video Demonstration
The recorded operational demonstration of the physical AM-CRN glove prototype (displaying 5-digit Coordinated Reset actuation, real-time kinematic gating, and hardware testbed behavior) is accessible at:
- **Google Drive Video Demonstration**: [AM-CRN Video Demonstration](https://drive.google.com/drive/folders/1uONN4YGhQNZigE1ExzX5zXttHGENcdZQ?usp=sharing)
