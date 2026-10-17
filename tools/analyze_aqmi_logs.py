#!/usr/bin/env python3
"""
IoT-Air-Quality-Monitoring-System: Log Analysis & Metrics Validator
Parses the MicroSD CSV log file to compute statistical distributions,
peak exposure events, and environmental correlations.
"""

import os
import sys
import pandas as pd
import numpy as np

def analyze_log(csv_path):
    if not os.path.exists(csv_path):
        print(f"[ERROR] Log file not found: {csv_path}")
        return False

    df = pd.read_csv(csv_path)
    print("=" * 70)
    print(f"  AQMI SENSOR LOG METRIC ANALYSIS: {os.path.basename(csv_path)}")
    print("=" * 70)
    print(f"Total Logged Records : {len(df)}")
    duration_sec = (df['Timestamp_ms'].max() - df['Timestamp_ms'].min()) / 1000.0
    print(f"Total Recording Span : {duration_sec:.1f} seconds ({duration_sec/60:.2f} minutes)")
    print("-" * 70)

    metrics = ['Temp_C', 'Pressure_hPa', 'MQ2_Raw', 'Smoke_ppm', 'LPG_ppm']
    summary = df[metrics].describe().T[['mean', 'std', 'min', 'max']]
    print(summary.to_string())

    print("-" * 70)
    peak_smoke = df['Smoke_ppm'].max()
    peak_idx = df['Smoke_ppm'].idxmax()
    peak_time_sec = df.loc[peak_idx, 'Timestamp_ms'] / 1000.0
    print(f"Peak Smoke Event     : {peak_smoke:.2f} ppm at T={peak_time_sec:.1f}s")
    print(f"Baseline Smoke Level : {df['Smoke_ppm'].iloc[0]:.2f} ppm")
    print(f"Dynamic Range Ratio  : {peak_smoke / df['Smoke_ppm'].iloc[0]:.2f}x baseline elevation")
    print("=" * 70)
    return True

if __name__ == "__main__":
    default_log = os.path.join(os.path.dirname(__file__), "..", "data", "sample_aqmi_log.csv")
    target = sys.argv[1] if len(sys.argv) > 1 else default_log
    analyze_log(target)
