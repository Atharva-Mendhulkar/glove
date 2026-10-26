# AM-CRN LaTeX Research Paper Report

This directory contains the formal two-column research paper report for the **AM-CRN Glove** project.

## Authors
- **Atharva Mendhulkar** (Reg. No: 24BIT0103)
- **Ritwij Tripathi** (Reg. No: 24BIT0095)

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
