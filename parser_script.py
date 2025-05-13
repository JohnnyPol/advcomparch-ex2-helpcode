import os
import re
import pandas as pd

def parse_stats_file(filepath):
    """
    Parses a single .out file to extract cache statistics.
    """
    stats = {
        "ipc": None,
        "total_instructions": None,
        "l1_total_misses": None,
        "l2_total_misses": None,
        "l1_mpki": None,
        "l2_mpki": None,
    }
    try:
        with open(filepath, 'r') as f:
            for line in f:
                if "IPC:" in line:
                    match = re.search(r"IPC:\s*([0-9.]+)", line)
                    if match:
                        stats["ipc"] = float(match.group(1))
                elif "Total Instructions:" in line:
                    match = re.search(r"Total Instructions:\s*([0-9]+)", line)
                    if match:
                        stats["total_instructions"] = int(match.group(1))
                elif "L1-Total-Misses:" in line: # Make sure to get the count, not percentage
                    match = re.search(r"L1-Total-Misses:\s*([0-9]+)", line)
                    if match:
                        stats["l1_total_misses"] = int(match.group(1))
                elif "L2-Total-Misses:" in line: # Make sure to get the count, not percentage
                    match = re.search(r"L2-Total-Misses:\s*([0-9]+)", line)
                    if match:
                        stats["l2_total_misses"] = int(match.group(1))

        # Calculate MPKI if data is available
        if stats["total_instructions"] and stats["total_instructions"] > 0:
            if stats["l1_total_misses"] is not None:
                stats["l1_mpki"] = (stats["l1_total_misses"] / stats["total_instructions"]) * 1000
            if stats["l2_total_misses"] is not None:
                stats["l2_mpki"] = (stats["l2_total_misses"] / stats["total_instructions"]) * 1000
        
        # Check if essential stats were found
        if stats["ipc"] is None or stats["total_instructions"] is None:
            print(f"Warning: Could not parse essential stats (IPC/Instructions) from {filepath}")
            return None # Indicate failure to parse essential data

    except Exception as e:
        print(f"Error reading or parsing file {filepath}: {e}")
        return None
    return stats

def extract_params_from_filename(filename):
    """
    Extracts benchmark name and L2 cache parameters from the filename.
    Example: 403.gcc.cslab_cache_stats_L2_LRU_0256_04_064.out
    """
    # Regex to capture benchmark name and L2 parameters
    # It assumes benchmark name can contain dots, followed by ".cslab_cache_stats_L2_LRU_"
    # then L2size (4 digits), L2assoc (2 digits), L2blocksize (3 digits)
    match = re.search(r"^(.*?)\.cslab_cache_stats_L2_(.*?)_(\d{4})_(\d{2})_(\d{3})\.out$", filename)
    if match:
        benchmark_name = match.group(1)
        policy = match.group(2)  # This is the policy, e.g., LRU
        l2_size_kb = int(match.group(3))
        l2_assoc = int(match.group(4))
        l2_block_b = int(match.group(5))
        return benchmark_name, policy,l2_size_kb, l2_assoc, l2_block_b
    else:
        print(f"Warning: Could not extract parameters from filename: {filename}")
        return None, None, None, None

def main():
    output_dir = "output"  # The main directory containing benchmark subdirectories
    all_results = []

    if not os.path.isdir(output_dir):
        print(f"Error: Output directory '{output_dir}' not found.")
        return

    for benchmark_folder_name in os.listdir(output_dir):
        benchmark_path = os.path.join(output_dir, benchmark_folder_name)
        if os.path.isdir(benchmark_path):
            for filename in os.listdir(benchmark_path):
                if filename.endswith(".out"):
                    filepath = os.path.join(benchmark_path, filename)
                    
                    # Extract parameters from filename
                    # The benchmark name from the folder is usually more reliable if filenames are complex
                    # but the filename itself also contains it. Let's use the one from the filename pattern.
                    bench_name_from_file, policy, l2_size, l2_assoc, l2_block = extract_params_from_filename(filename)
                    
                    if bench_name_from_file is None: # Skip if filename parsing failed
                        continue

                    # Parse the statistics from the file content
                    parsed_data = parse_stats_file(filepath)

                    if parsed_data:
                        # Combine filename parameters with parsed statistics
                        result_entry = {
                            "Benchmark": bench_name_from_file,
                            "L2_Size_KB": l2_size,
                            "L2_Assoc": l2_assoc,
                            "L2_Block_B": l2_block,
                            "IPC": parsed_data["ipc"],
                            "L1_MPKI": parsed_data["l1_mpki"],
                            "L2_MPKI": parsed_data["l2_mpki"],
                            "Total_Instructions": parsed_data["total_instructions"],
                            "L1_Total_Misses": parsed_data["l1_total_misses"],
                            "L2_Total_Misses": parsed_data["l2_total_misses"],
                            "Policy": policy,
                            # You can add more raw data if needed
                        }
                        all_results.append(result_entry)

    if not all_results:
        print("No results found or parsed. Exiting.")
        return

    # Create a pandas DataFrame
    df_results = pd.DataFrame(all_results)

    # Optional: Sort the DataFrame for better readability
    df_results = df_results.sort_values(by=["Benchmark", "L2_Size_KB", "L2_Assoc", "L2_Block_B"])

    # Print the DataFrame to console
    print("\n--- Parsed Statistics ---")
    print(df_results.to_string()) # .to_string() prints the whole dataframe

    # Save the DataFrame to a CSV file
    csv_filename = "cache_simulation_results.csv"
    try:
        df_results.to_csv(csv_filename, index=False)
        print(f"\nResults successfully saved to {csv_filename}")
    except Exception as e:
        print(f"\nError saving results to CSV: {e}")

if __name__ == "__main__":
    main()