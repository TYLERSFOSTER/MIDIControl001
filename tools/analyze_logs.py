#!/usr/bin/env python3
"""
Analyzer exit codes:
    0  = success, no anomalies
    1  = mild warnings (flat RMS segments only)
    2  = non-fatal anomalies (clipping or similar)
    99 = unexpected crash (should be caught by bash wrapper)
"""

import re
import sys
import numpy as np
from pathlib import Path

# ============================================================
# Phase 4-E: Unified Analyzer
# ============================================================

def parse_voice_log(path: Path):
    rms, active = [], []
    with open(path) as f:
        for line in f:
            if "pre-gain RMS=" in line:
                m = re.search(r"RMS=([\d.eE+-]+).*active=(\d+)", line)
                if m:
                    rms.append(float(m.group(1)))
                    active.append(int(m.group(2)))
    return np.array(rms), np.array(active)


def parse_block_log(path: Path):
    noteon = sum("NoteOn" in line for line in open(path))
    noteoff = sum("NoteOff" in line for line in open(path))
    return noteon, noteoff


def detect_anomalies(rms_series, active_series):
    anomalies = []
    flat_rms_found = False
    clipping_found = False

    if len(rms_series) == 0:
        return ["No data parsed (empty log)"], flat_rms_found, clipping_found

    # 1. RMS flatline detection
    streak = 0
    for i in range(1, len(rms_series)):
        if abs(rms_series[i] - rms_series[i - 1]) < 1e-6:
            streak += 1
        else:
            if streak >= 5:
                anomalies.append(f"⚠️ RMS flat for {streak} blocks near index {i}")
                flat_rms_found = True
            streak = 0
    if streak >= 5:
        anomalies.append(f"⚠️ RMS flat for {streak} blocks at end of log")
        flat_rms_found = True

    # 2. Residual audio but no active voices
    for i, (r, a) in enumerate(zip(rms_series, active_series)):
        if r > 1e-4 and a == 0:
            anomalies.append(
                f"⚠️ Residual audio (RMS={r:.4f}) while no active voices at block {i}"
            )

    # 3. Voice overflow
    for i, a in enumerate(active_series):
        if a > 16:
            anomalies.append(f"⚠️ Possible voice leak ({a} voices) at block {i}")

    # 4. Clipping
    for i, r in enumerate(rms_series):
        if r > 1.1:
            anomalies.append(f"🔥 Possible clipping (RMS={r:.2f}) at block {i}")
            clipping_found = True

    return anomalies or ["No anomalies detected."], flat_rms_found, clipping_found


def main():
    root = Path(__file__).resolve().parents[1]
    voice_log = root / "voice_debug.txt"
    block_log = root / "process_block.log"
    report_file = root / "report_summary.txt"

    try:
        if not voice_log.exists():
            print("Missing voice_debug.txt")
            sys.exit(99)

        rms, active = parse_voice_log(voice_log)
        noteon, noteoff = parse_block_log(block_log) if block_log.exists() else (0, 0)

        n_blocks = len(rms)
        avg_rms = float(np.mean(rms)) if n_blocks else 0.0
        max_rms = float(np.max(rms)) if n_blocks else 0.0
        avg_active = float(np.mean(active)) if n_blocks else 0.0
        duration_s = n_blocks * 512 / 48000.0  # 512-sample buffer @48kHz

        anomalies, flat_rms_found, clipping_found = detect_anomalies(rms, active)

        report = [
            "=== MIDIControl001 Session Report ===",
            f"Blocks processed         : {n_blocks}",
            f"Estimated duration (s)   : {duration_s:.2f}",
            f"Avg RMS                  : {avg_rms:.6f}",
            f"Max RMS                  : {max_rms:.6f}",
            f"Avg active voices        : {avg_active:.3f}",
            f"NoteOn events            : {noteon}",
            f"NoteOff events           : {noteoff}",
            f"Voice log size           : {voice_log.stat().st_size/1024:.1f} KB",
            f"Block log size           : {block_log.stat().st_size/1024:.1f} KB"
            if block_log.exists()
            else "",
            "",
            "=== Anomaly Diagnostics ===",
            *anomalies,
            "",
            f"[Report written to] {report_file}",
        ]

        text = "\n".join(report)
        print(text)
        report_file.write_text(text)

        # Exit code logic
        if clipping_found:
            sys.exit(2)
        elif flat_rms_found:
            sys.exit(1)
        else:
            sys.exit(0)

    except Exception as e:
        print(f"Unexpected analyzer failure: {e}")
        sys.exit(99)


if __name__ == "__main__":
    main()
