#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg') # Χρήση non-interactive backend

import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def load_all_policies(base_dir, policies):
    dfs = []
    for pol in policies:
        csv_path = os.path.join(base_dir, pol, "cache_simulation_results.csv")
        if not os.path.isfile(csv_path):
            print(f"Warning: δεν βρέθηκε {csv_path}")
            continue
        df = pd.read_csv(csv_path)
        df["Policy"] = pol
        dfs.append(df)
    if not dfs:
        raise RuntimeError("Δεν φορτώθηκαν δεδομένα για καμία πολιτική!")
    return pd.concat(dfs, ignore_index=True)

def geometric_mean(series):
    # Διατηρούμε μόνο θετικές τιμές
    arr = series.dropna().astype(float)
    # Αν κάποιο MPKI είναι 0, προσθέτουμε πολύ μικρό offset για να μην έχουμε log(0)
    arr = np.where(arr == 0, 1e-6, arr)
    return float(np.exp(np.log(arr).mean()))

def aggregate_by_capacity(df):
    # Ομαδοποίηση και γεωμ. μέσος
    agg = df.groupby(["L2_Size_KB", "Policy"]).agg({
        "IPC": geometric_mean,
        "L2_MPKI": geometric_mean
    }).rename(columns={
        "IPC": "IPC_GM",
        "L2_MPKI": "MPKI_GM"
    }).reset_index()
    return agg

def plot_by_capacity(agg_df, policies, outdir):
    """
    For each L2 size, draws two bar charts (IPC and MPKI geometric means)
    with gridlines, value annotations, and improved layout.
    """
    os.makedirs(outdir, exist_ok=True)
    capacities = sorted(agg_df["L2_Size_KB"].unique())
    
    for cap in capacities:
        sub = agg_df[agg_df["L2_Size_KB"] == cap].set_index("Policy").reindex(policies)
        x = np.arange(len(policies))
        width = 0.6

        # ——— IPC plot ———
        fig, ax = plt.subplots(figsize=(8, 5))
        bars = ax.bar(x, sub["IPC_GM"].values, width)
        ax.set_xticks(x)
        ax.set_xticklabels(policies, fontsize=12)
        ax.set_title(f"Geometric Mean IPC @ L2 Size {cap} KB", fontsize=14)
        ax.set_xlabel("Policy", fontsize=12)
        ax.set_ylabel("IPC (geometric mean)", fontsize=12)
        ax.grid(axis="y", linestyle="--", linewidth=0.7, alpha=0.7)
        ax.set_ylim(0, sub["IPC_GM"].max() * 1.1)

        # Annotate bar values
        ax.bar_label(bars, fmt="%.2f", padding=4, fontsize=11)
        fig.tight_layout()
        fn_ipc = os.path.join(outdir, f"{cap}_ipc_gm.png")
        fig.savefig(fn_ipc, dpi=300)
        plt.close(fig)

        # ——— MPKI plot ———
        fig, ax = plt.subplots(figsize=(8, 5))
        bars = ax.bar(x, sub["MPKI_GM"].values, width)
        ax.set_xticks(x)
        ax.set_xticklabels(policies, fontsize=12)
        ax.set_title(f"Geometric Mean L2 MPKI @ L2 Size {cap} KB", fontsize=14)
        ax.set_xlabel("Policy", fontsize=12)
        ax.set_ylabel("L2 MPKI (geometric mean)", fontsize=12)
        ax.grid(axis="y", linestyle="--", linewidth=0.7, alpha=0.7)
        ax.set_ylim(0, sub["MPKI_GM"].max() * 1.1)

        # Annotate bar values
        ax.bar_label(bars, fmt="%.2f", padding=4, fontsize=11)
        fig.tight_layout()
        fn_mpki = os.path.join(outdir, f"{cap}_mpki_gm.png")
        fig.savefig(fn_mpki, dpi=300)
        plt.close(fig)

        print(f"Plots for L2 {cap} KB saved: {fn_ipc}, {fn_mpki}")


def main():
    # Ρύθμισέ το ανάλογα με τη δομή σου
    base_dir = "parsed_output/4.3"
    policies = ["LFU", "LIP", "Random", "SRRIP"]
    df_all = load_all_policies(base_dir, policies)
    
    agg_df = aggregate_by_capacity(df_all)
    
    output_dir = os.path.join(base_dir, "plots_by_capacity")
    plot_by_capacity(agg_df, policies, output_dir)
    print(f"Όλα τα γραφήματα σώθηκαν στον φάκελο: {output_dir}")

if __name__ == "__main__":
    main()
