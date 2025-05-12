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

def plot_ipc_heatmaps(csv_filepath="cache_simulation_results.csv"):
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
            filename = f"heatmap_ipc_l2_{capacity}KB.png"
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
    plot_ipc_heatmaps(csv_filepath="cache_simulation_results.csv")