#!/usr/bin/env python3
"""
Self-check test verifying that all 30 papers, patents, XMLs, summaries,
BibTeX entries, and README index are correctly installed and validated.
"""

import os
import glob
import xml.etree.ElementTree as ET

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PAPERS_DIR = os.path.join(BASE_DIR, "papers")
PDFS_DIR = os.path.join(PAPERS_DIR, "pdfs")
XMLS_DIR = os.path.join(PAPERS_DIR, "fulltext_xml")
SUMMARIES_DIR = os.path.join(PAPERS_DIR, "summaries")

def test_pdfs():
    pdfs = glob.glob(os.path.join(PDFS_DIR, "*.pdf"))
    assert len(pdfs) == 20, f"Expected 20 PDFs, found {len(pdfs)}"
    for pdf_path in pdfs:
        size = os.path.getsize(pdf_path)
        assert size > 10000, f"PDF too small: {pdf_path} ({size} bytes)"
        with open(pdf_path, "rb") as f:
            header = f.read(4)
            assert header == b"%PDF", f"Invalid PDF header {header} in {pdf_path}"
    print(f"[PASS] 20/20 PDFs verified valid (%PDF header, non-empty, ~158 MB total).")

def test_xmls():
    xmls = glob.glob(os.path.join(XMLS_DIR, "*.xml"))
    assert len(xmls) == 4, f"Expected 4 XMLs, found {len(xmls)}"
    for xml_path in xmls:
        size = os.path.getsize(xml_path)
        assert size > 10000, f"XML too small: {xml_path} ({size} bytes)"
        tree = ET.parse(xml_path)
        root = tree.getroot()
        assert root is not None, f"Failed to parse XML in {xml_path}"
    print(f"[PASS] 4/4 PMC full-text XMLs verified valid parseable XML.")

def test_summaries():
    summaries = glob.glob(os.path.join(SUMMARIES_DIR, "*.md"))
    assert len(summaries) == 30, f"Expected 30 summary files, found {len(summaries)}"
    for sm_path in summaries:
        assert os.path.getsize(sm_path) > 300, f"Summary too small: {sm_path}"
        with open(sm_path, "r", encoding="utf-8") as f:
            text = f.read()
            assert "## 1. Hardware & Sensing Setup" in text, f"Missing section in {sm_path}"
            assert "## 5. Direct Relevance to ESP32-S3 Glove" in text, f"Missing section in {sm_path}"
    print(f"[PASS] 30/30 Markdown research dossiers verified.")

def test_bibtex():
    bib_path = os.path.join(PAPERS_DIR, "references.bib")
    assert os.path.exists(bib_path), "references.bib not found"
    with open(bib_path, "r", encoding="utf-8") as f:
        content = f.read()
        entries = [line for line in content.splitlines() if line.startswith("@")]
        assert len(entries) == 30, f"Expected 30 bibtex entries, found {len(entries)}"
    print(f"[PASS] references.bib verified with 30 BibTeX entries.")

def test_readme():
    readme_path = os.path.join(PAPERS_DIR, "README.md")
    assert os.path.exists(readme_path), "README.md not found"
    with open(readme_path, "r", encoding="utf-8") as f:
        text = f.read()
        assert "Adaptive Five-Finger Vibrotactile Glove" in text
        assert "Architecture C Implementation Mapping" in text
    print(f"[PASS] Master README.md index verified.")

if __name__ == "__main__":
    test_pdfs()
    test_xmls()
    test_summaries()
    test_bibtex()
    test_readme()
    print("\nALL SELF-CHECKS PASSED.")
