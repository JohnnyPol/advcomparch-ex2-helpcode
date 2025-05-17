"""
Script for Exercise 4.2:
1. Load the CSV file containing cache simulation results.
2. Calculate the geometric mean of IPC and MPKI values across benchmarks for each configuration.
3. Generate heatmaps of the geometric mean IPC vs. (Associativity, Block Size) for each L2 capacity.
4. Generate heatmaps of the geometric mean IPC and MPKI vs. (Associativity, Block Size) for each L2 capacity.
5. Save the heatmaps in a specified output folder.
"""
import matplotlib
matplotlib.use('Agg') # Χρήση non-interactive backend

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns # Χρήσιμο για heatmaps
import math

# Συνάρτηση για γεωμετρικό μέσο όρο (ίδια με πριν)
def geometric_mean_overflow_safe(series):
    valid_series = series.dropna()
    valid_series = valid_series[valid_series > 0]
    if valid_series.empty:
         return np.nan
    try:
        log_values = np.log(valid_series)
        mean_log = np.mean(log_values)
        geo_mean = np.exp(mean_log)
        if math.isnan(geo_mean) or math.isinf(geo_mean):
             return np.nan
        return geo_mean
    except Exception:
        return np.nan

def plot_ipc_mpki_heatmaps(csv_filepath="cache_simulation_results.csv", output_folder="parsed_output/4.2"):
    """
    Loads results, calculates geometric means (IPC, L1_MPKI, L2_MPKI), 
    and generates heatmaps vs. (Associativity, Block Size) for each L2 capacity.
    """
    # --- Φόρτωση Δεδομένων ---
    try:
        df = pd.read_csv(csv_filepath)
        print(f"Successfully loaded {csv_filepath}")
    except FileNotFoundError:
        print(f"Error: CSV file '{csv_filepath}' not found.")
        return
    except Exception as e:
        print(f"Error loading CSV file: {e}")
        return

    # --- Υπολογισμός Γεωμετρικών Μέσων ---
    print("\nCalculating geometric means across benchmarks for each configuration...")
    try:
        df_geom_means = df.groupby(['L2_Size_KB', 'L2_Assoc', 'L2_Block_B']).agg(
            IPC=('IPC', geometric_mean_overflow_safe),
            # Προσθήκη υπολογισμού για L1_MPKI και L2_MPKI
            L1_MPKI=('L1_MPKI', geometric_mean_overflow_safe), 
            L2_MPKI=('L2_MPKI', geometric_mean_overflow_safe)  
        ).reset_index()
        
        print(f"Calculated geometric means for {len(df_geom_means)} unique configurations.")
        
        # Δεν αφαιρούμε NaN ακόμα, γιατί μπορεί να υπάρχει έγκυρο IPC αλλά όχι MPKI
        # Θα το χειριστούμε κατά το pivoting/plotting.
        
    except Exception as e:
        print(f"Error during geometric mean calculation: {e}")
        return

    # --- Δημιουργία Heatmaps ανά Χωρητικότητα L2 ---
    l2_capacities = sorted(df_geom_means["L2_Size_KB"].unique())
    print(f"\nGenerating heatmaps for L2 capacities: {l2_capacities}")

    for capacity in l2_capacities:
        print(f"  Processing L2 Capacity: {capacity} KB")
        df_capacity = df_geom_means[df_geom_means["L2_Size_KB"] == capacity].copy()

        if df_capacity.empty:
            print(f"    Warning: No data found for {capacity} KB.")
            continue

        # --- Heatmap για IPC ---
        try:
            pivot_ipc = df_capacity.pivot_table(index='L2_Assoc', columns='L2_Block_B', values='IPC')
            if not pivot_ipc.empty:
                plt.figure(figsize=(10, 7))
                sns.heatmap(pivot_ipc, annot=True, fmt=".4f", linewidths=.5, cmap="viridis") 
                plt.title(f"Geometric Mean IPC for L2 Cache Size = {capacity} KB")
                plt.xlabel("L2 Block Size (B)")
                plt.ylabel("L2 Associativity (ways)")
                plt.xticks(rotation=0); plt.yticks(rotation=0)
                plt.tight_layout()
                filename_ipc = f"{output_folder}/heatmap_ipc_l2_{capacity}KB.png"
                plt.savefig(filename_ipc, dpi=300)
                print(f"    Saved IPC heatmap: {filename_ipc}")
                plt.close()
            else:
                 print(f"    Warning: Pivot table for IPC is empty for {capacity} KB.")
        except Exception as e:
            print(f"    Error generating IPC heatmap for {capacity} KB: {e}")

        # --- Heatmap για L1 MPKI ---
        try:
            pivot_l1mpki = df_capacity.pivot_table(index='L2_Assoc', columns='L2_Block_B', values='L1_MPKI')
            if not pivot_l1mpki.empty:
                plt.figure(figsize=(10, 7))
                # Χρησιμοποιούμε αντίστροφη παλέτα π.χ., 'viridis_r' ή 'rocket_r', όπου ανοιχτά χρώματα = χαμηλό MPKI (καλύτερο)
                sns.heatmap(pivot_l1mpki, annot=True, fmt=".2f", linewidths=.5, cmap="viridis_r") 
                plt.title(f"Geometric Mean L1 MPKI for L2 Cache Size = {capacity} KB")
                plt.xlabel("L2 Block Size (B)")
                plt.ylabel("L2 Associativity (ways)")
                plt.xticks(rotation=0); plt.yticks(rotation=0)
                plt.tight_layout()
                filename_l1mpki = f"{output_folder}/heatmap_l1mpki_l2_{capacity}KB.png"
                plt.savefig(filename_l1mpki, dpi=300)
                print(f"    Saved L1 MPKI heatmap: {filename_l1mpki}")
                plt.close()
            else:
                 print(f"    Warning: Pivot table for L1 MPKI is empty for {capacity} KB.")
        except Exception as e:
            print(f"    Error generating L1 MPKI heatmap for {capacity} KB: {e}")

        # --- Heatmap για L2 MPKI ---
        try:
            pivot_l2mpki = df_capacity.pivot_table(index='L2_Assoc', columns='L2_Block_B', values='L2_MPKI')
            if not pivot_l2mpki.empty:
                plt.figure(figsize=(10, 7))
                # Χρησιμοποιούμε αντίστροφη παλέτα π.χ., 'viridis_r' ή 'rocket_r'
                sns.heatmap(pivot_l2mpki, annot=True, fmt=".2f", linewidths=.5, cmap="viridis_r") 
                plt.title(f"Geometric Mean L2 MPKI for L2 Cache Size = {capacity} KB")
                plt.xlabel("L2 Block Size (B)")
                plt.ylabel("L2 Associativity (ways)")
                plt.xticks(rotation=0); plt.yticks(rotation=0)
                plt.tight_layout()
                filename_l2mpki = f"{output_folder}/heatmap_l2mpki_l2_{capacity}KB.png"
                plt.savefig(filename_l2mpki, dpi=300)
                print(f"    Saved L2 MPKI heatmap: {filename_l2mpki}")
                plt.close()
            else:
                 print(f"    Warning: Pivot table for L2 MPKI is empty for {capacity} KB.")
        except Exception as e:
            print(f"    Error generating L2 MPKI heatmap for {capacity} KB: {e}")


def plot_ipc_heatmaps(csv_filepath="cache_simulation_results.csv", output_folder="parsed_output/4.2"):
    """
    Loads results, calculates geometric mean IPC, and generates 
    heatmaps of Geo Mean IPC vs. (Associativity, Block Size) 
    for each L2 capacity.
    """
    # --- Φόρτωση Δεδομένων ---
    try:
        df = pd.read_csv(csv_filepath)
        print(f"Successfully loaded {csv_filepath}")
    except FileNotFoundError:
        print(f"Error: CSV file '{csv_filepath}' not found.")
        return
    except Exception as e:
        print(f"Error loading CSV file: {e}")
        return

    # --- Υπολογισμός Γεωμετρικών Μέσων ---
    print("\nCalculating geometric means across benchmarks for each configuration...")
    try:
        df_geom_means = df.groupby(['L2_Size_KB', 'L2_Assoc', 'L2_Block_B']).agg(
            IPC=('IPC', geometric_mean_overflow_safe)
            # Μπορείτε να προσθέσετε και MPKI αν θέλετε heatmaps και γι' αυτά
            # L1_MPKI=('L1_MPKI', geometric_mean_overflow_safe),
            # L2_MPKI=('L2_MPKI', geometric_mean_overflow_safe)
        ).reset_index()
        
        # Αφαίρεση γραμμών όπου ο υπολογισμός απέτυχε (π.χ., NaN)
        df_geom_means.dropna(subset=['IPC'], inplace=True)
        if len(df_geom_means) == 0:
             print("Error: No valid geometric mean IPC values found.")
             return
        print(f"Calculated geometric means for {len(df_geom_means)} unique configurations.")
        
    except Exception as e:
        print(f"Error during geometric mean calculation: {e}")
        return

    # --- Δημιουργία Heatmaps ανά Χωρητικότητα L2 ---
    l2_capacities = sorted(df_geom_means["L2_Size_KB"].unique())
    print(f"\nGenerating heatmaps for L2 capacities: {l2_capacities}")

    for capacity in l2_capacities:
        print(f"  Processing L2 Capacity: {capacity} KB")
        # Φιλτράρισμα δεδομένων για την τρέχουσα χωρητικότητα
        df_capacity = df_geom_means[df_geom_means["L2_Size_KB"] == capacity].copy()

        if df_capacity.empty:
            print(f"    Warning: No data found for {capacity} KB.")
            continue

        try:
            # Δημιουργία pivot table: Index=Assoc, Columns=Block_B, Values=IPC
            # Αυτό μετατρέπει τα δεδομένα σε μορφή πίνακα κατάλληλη για heatmap
            pivot_table = df_capacity.pivot_table(index='L2_Assoc', 
                                                  columns='L2_Block_B', 
                                                  values='IPC')
            
            # Έλεγχος αν ο pivot table δημιουργήθηκε σωστά
            if pivot_table.empty:
                 print(f"    Warning: Pivot table is empty for {capacity} KB. Skipping heatmap.")
                 continue

            # Δημιουργία του heatmap
            plt.figure(figsize=(10, 7)) # Μέγεθος εικόνας
            sns.heatmap(
                pivot_table, 
                annot=True,     # Εμφάνιση τιμών IPC στα κελιά
                fmt=".4f",      # Μορφοποίηση των τιμών (4 δεκαδικά)
                linewidths=.5,  # Γραμμές μεταξύ κελιών
                cmap="viridis"  # Χρωματική παλέτα (άλλες: "plasma", "inferno", "magma", "cividis", "coolwarm")
            )
            
            plt.title(f"Geometric Mean IPC for L2 Cache Size = {capacity} KB") # Τίτλος
            plt.xlabel("L2 Block Size (B)")       # Ετικέτα άξονα Χ
            plt.ylabel("L2 Associativity (ways)") # Ετικέτα άξονα Υ
            # Ensure axes ticks are correctly displayed (optional adjustment)
            plt.xticks(rotation=0) # Keep block size labels horizontal
            plt.yticks(rotation=0) # Keep associativity labels horizontal
            plt.tight_layout() # Αυτόματη προσαρμογή για να χωράνε όλα

            # Αποθήκευση του heatmap σε αρχείο PNG
            filename = f"{output_folder}/heatmap_ipc_l2_{capacity}KB.png"
            plt.savefig(filename, dpi=300) # Αποθήκευση με 300 DPI
            print(f"    Saved heatmap: {filename}")
            plt.close() # Κλείσιμο της φιγούρας για εξοικονόμηση μνήμης

        except KeyError as e:
             print(f"    Error creating pivot table for {capacity} KB. Missing column? {e}")
             print(f"    Available columns in filtered data: {df_capacity.columns.tolist()}")
        except Exception as e:
            print(f"    Error generating heatmap for {capacity} KB: {e}")

    print("\nScript finished.")

if __name__ == "__main__":
    # Βεβαιωθείτε ότι το όνομα του αρχείου CSV είναι σωστό
    csv_filepath = "parsed_output/4.3/Random/cache_simulation_results.csv"
    output_folder = "parsed_output/4.3/Random"
    plot_ipc_heatmaps(csv_filepath=csv_filepath, output_folder=output_folder)
    plot_ipc_mpki_heatmaps(csv_filepath=csv_filepath, output_folder=output_folder)
