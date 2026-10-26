#!/usr/bin/env python3
"""
Downloads and organizes the 30 papers, preprints, and patents for the
Adaptive Five-Finger Vibrotactile Glove research architecture.
Uses Python standard library only.
"""

import os
import sys
import json
import urllib.request
import xml.etree.ElementTree as ET

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PAPERS_DIR = os.path.join(BASE_DIR, "papers")
PDFS_DIR = os.path.join(PAPERS_DIR, "pdfs")
XMLS_DIR = os.path.join(PAPERS_DIR, "fulltext_xml")
SUMMARIES_DIR = os.path.join(PAPERS_DIR, "summaries")

HEADERS = {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/119.0"}

def download_file(url, out_path, min_bytes=1000):
    if os.path.exists(out_path) and os.path.getsize(out_path) >= min_bytes:
        print(f"[OK] {os.path.basename(out_path)} already exists ({os.path.getsize(out_path)} bytes).")
        return True
    try:
        req = urllib.request.Request(url, headers=HEADERS)
        with urllib.request.urlopen(req, timeout=30) as resp:
            data = resp.read()
            if len(data) < min_bytes or (out_path.endswith('.pdf') and not data.startswith(b'%PDF')):
                # Sometimes a redirect returns HTML instead of PDF
                print(f"[WARN] {url} returned non-PDF or truncated data ({len(data)} bytes). Skipping write.")
                return False
            with open(out_path, "wb") as f:
                f.write(data)
            print(f"[DOWNLOADED] {os.path.basename(out_path)} ({len(data)} bytes)")
            return True
    except Exception as e:
        print(f"[FAIL] {url} -> {e}")
        return False

def fetch_pmc_xml(pmcid, out_path):
    if os.path.exists(out_path) and os.path.getsize(out_path) > 1000:
        return True
    num_id = pmcid.replace("PMC", "")
    url = f"https://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi?db=pmc&id={num_id}&retmode=xml"
    try:
        req = urllib.request.Request(url, headers=HEADERS)
        with urllib.request.urlopen(req, timeout=30) as resp:
            xml_data = resp.read()
            if len(xml_data) > 1000:
                with open(out_path, "wb") as f:
                    f.write(xml_data)
                print(f"[XML FETCHED] {os.path.basename(out_path)} ({len(xml_data)} bytes)")
                return True
    except Exception as e:
        print(f"[FAIL XML] {pmcid} -> {e}")
    return False

def main():
    os.makedirs(PDFS_DIR, exist_ok=True)
    os.makedirs(XMLS_DIR, exist_ok=True)
    os.makedirs(SUMMARIES_DIR, exist_ok=True)

    # 1. Direct PDF targets
    pdf_targets = [
        # Primary research
        ("01_nature_comms_adaptive_tactile_glove.pdf", "https://www.nature.com/articles/s41467-024-45059-8.pdf"),
        ("05_ieee_tnsre_instrumented_glove.pdf", "https://ieeexplore.ieee.org/stampPDF/getPDF.jsp?tp=&arnumber=10559868"),
        ("07_springer_smart_glove_systematic_review.pdf", "https://link.springer.com/content/pdf/10.1186/s44147-026-01084-6.pdf"),
        ("08_arxiv_navigating_with_haptic_gloves.pdf", "https://arxiv.org/pdf/2503.23484.pdf"),
        ("11_arxiv_neuroadaptive_haptics_rl.pdf", "https://arxiv.org/pdf/2504.15984.pdf"),
        ("12_cambridge_robotica_realtime_slip_detection.pdf", "https://www.cambridge.org/core/services/aop-cambridge-core/content/view/594DC0426FD4D26FECC4D7AA20415F43/S0263574725102841a.pdf/real-time-slip-detection-with-friction-scaled-vibrotactile-feedback-for-robotic-sixth-finger-assisted-manipulation.pdf"),
        ("13_arxiv_unobstructed_fingerpad_force_vibration.pdf", "https://arxiv.org/pdf/2506.21417.pdf"),
        ("14_frontiers_surgical_training_haptics.pdf", "https://www.frontiersin.org/journals/robotics-and-ai/articles/10.3389/frobt.2025.1567955/pdf"),
        ("17_bmc_bci_soft_robotic_glove.pdf", "https://jneuroengrehab.biomedcentral.com/counter/pdf/10.1186/s12984-025-01704-x.pdf"),
        ("18_springer_contralateral_interference.pdf", "https://link.springer.com/content/pdf/10.3758/s13414-023-02801-6.pdf"),
        ("19_eth_jneurophysiol_vibrotactile_discrimination.pdf", "https://www.research-collection.ethz.ch/server/api/core/bitstreams/b7cd5d40-a18f-4cca-ad0f-b1c0508ef846/content"),
        # Patents & foundational
        ("21_patent_US11989347B2_haptic_glove_apparatus.pdf", "https://patentimages.storage.googleapis.com/a7/3b/e8/0a13505f809e16/US11989347.pdf"),
        ("22_patent_WO2015083183A1_navigation_device.pdf", "https://patentimages.storage.googleapis.com/17/ef/3d/048693f3b439dc/WO2015083183A1.pdf"),
        ("23_patent_WO2025023899A1_haptic_glove.pdf", "https://patentimages.storage.googleapis.com/94/8d/e6/3a4c808fefd923/WO2025023899A1.pdf"),
        ("24_patent_US20180335842A1_haptic_feedback_glove.pdf", "https://patentimages.storage.googleapis.com/4f/22/8e/5ccbe8623ba46a/US20180335842A1.pdf"),
        ("25_patent_US20220326769A1_multimodal_haptic_device.pdf", "https://patentimages.storage.googleapis.com/b5/e4/8c/6be257ea43b090/US20220326769A1.pdf"),
        ("26_patent_US10698492B2_wearable_multisensory_interface.pdf", "https://patentimages.storage.googleapis.com/ac/6f/58/e8fce2fb31edf9/US10698492.pdf"),
        ("28_arxiv_closed_loop_haptic_biofeedback_smartwatch.pdf", "https://arxiv.org/pdf/2507.02432.pdf"),
        ("29_frontiers_electrotactile_displays_taxonomy.pdf", "https://www.frontiersin.org/journals/virtual-reality/articles/10.3389/frvir.2024.1406923/pdf"),
        ("30_arxiv_tactile_perception_prostheses.pdf", "https://arxiv.org/pdf/2402.12989.pdf"),
    ]

    print("=== Downloading Available Open Access PDFs & Patents ===")
    pdf_success = 0
    for filename, url in pdf_targets:
        target = os.path.join(PDFS_DIR, filename)
        if download_file(url, target):
            pdf_success += 1

    # 2. Open Access Full-Text XMLs from NCBI PMC
    pmc_targets = [
        ("02_sensors_ml_gesture_glove_PMC11435450.xml", "PMC11435450"),
        ("04_med_lav_neurosensory_vibration_PMC12199025.xml", "PMC12199025"),
        ("15_bmjopen_vibrotactile_soft_glove_rct_protocol_PMC12918676.xml", "PMC12918676"),
        ("16_frontiers_vibrotactile_neurorehab_review_PMC13597832.xml", "PMC13597832"),
    ]

    print("\n=== Fetching Full-Text PMC XMLs ===")
    xml_success = 0
    for filename, pmcid in pmc_targets:
        target = os.path.join(XMLS_DIR, filename)
        if fetch_pmc_xml(pmcid, target):
            xml_success += 1

    print(f"\nDone: {pdf_success}/{len(pdf_targets)} PDFs ready, {xml_success}/{len(pmc_targets)} XMLs ready.")

if __name__ == "__main__":
    main()
