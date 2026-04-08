import subprocess
import numpy as np
import matplotlib.pyplot as plt
import os
import random
import re

# --- Configuration ---
EXECUTABLE = "./mergesort"
TEMP_INPUT = "temp_input.txt"

# N (Data Size: 1M to 10M) and T (Thread Count: 1 to 16)
N_START, N_END, N_DELTA = 100000, 1000000, 100000
T_START, T_END, T_STEP = 2, 99, 1
ITERATIONS = 10 # 반복 횟수:

def generate_random_file(n, filename):
    """Generates an input file with n random integers."""
    with open(filename, 'w') as f:
        numbers = [str(random.randint(-n, n)) for _ in range(n)]
        f.write(" ".join(numbers))

def get_execution_time(filename, t):
    """Runs mergesort and returns the execution time."""
    try:
        result = subprocess.run(
            [EXECUTABLE, filename, str(t)], 
            capture_output=True, text=True, timeout=120
        )
        output = result.stdout.strip()
        
        # Parse execution time from "Cost Time: XXX micro seconds"
        time_match = re.search(r"Cost Time:\s*(\d+)", output)
        if time_match:
            return float(time_match.group(1)) / 1_000_000
        return 0.0
            
    except Exception as e:
        print(f"\n[ERROR] T={t} Execution failed: {e}")
        return 0.0

# --- Data Collection ---
n_range = np.arange(N_START, N_END + 1, N_DELTA)
t_range = np.arange(T_START, T_END + 1, T_STEP)
N, T = np.meshgrid(n_range, t_range)
Z = np.zeros(N.shape)

print("Starting Performance Benchmarking ...")
print("-" * 60)

for j, n in enumerate(n_range):
    print(f"\n[Target N = {n}]")
    generate_random_file(n, TEMP_INPUT)
    
    for i, t in enumerate(t_range):
        total_time = 0.0
        
        for r in range(ITERATIONS):
            total_time += get_execution_time(TEMP_INPUT, t)

        Z[i, j] = total_time / ITERATIONS
        
        print(f"  > Thread {t:2d}: Avg Time = {Z[i, j]:.4f}s")

if os.path.exists(TEMP_INPUT):
    os.remove(TEMP_INPUT)

print("-" * 60)
print("Benchmarking Process Finished Successfully.")
print("make 3D Graph...")

# --- 3D Plotting ---
fig = plt.figure(figsize=(12, 8))
ax = fig.add_subplot(111, projection='3d')

N_scaled = N / 1000
surf = ax.plot_surface(N_scaled, T, Z, cmap='viridis', edgecolor='none', alpha=0.9)

ax.set_xlabel('Number of Elements (K)')
ax.set_ylabel('Number of Threads (T)')
ax.set_zlabel('Execution Time (sec)')
ax.set_title('Merge Sort Performance Analysis')
fig.colorbar(surf, ax=ax, shrink=0.5, aspect=5)

plt.show()
