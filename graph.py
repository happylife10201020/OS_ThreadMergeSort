# main.py
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Button, Slider
import os
import random
import re
import tkinter as tk
from tkinter import messagebox, ttk
import threading

EXECUTABLE = "./mergesort"
TEMP_INPUT = "temp_input.txt"
DATA_FILE = "benchmark_data.npz"
LOG_FILE = "output.txt"

# ---------------------------
# 스타일 설정
# ---------------------------
def setup_style():
    style = ttk.Style()
    style.theme_use("clam")

    # 전체 색상
    bg = "#1e1e1e"
    fg = "#ffffff"
    accent = "#4CAF50"

    style.configure(".", background=bg, foreground=fg, font=("Segoe UI", 10))
    style.configure("TLabel", background=bg, foreground=fg)
    style.configure("TFrame", background=bg)
    style.configure("TEntry", fieldbackground="#2a2a2a", foreground=fg)
    
    style.configure(
        "Accent.TButton",
        background=accent,
        foreground="white",
        padding=10
    )
    style.map(
        "Accent.TButton",
        background=[("active", "#45a049")]
    )

    return bg


def generate_random_file(n, filename):
    with open(filename, 'w') as f:
        numbers = [str(random.randint(-n, n)) for _ in range(n)]
        f.write(" ".join(numbers))

def get_execution_time(filename, t):
    try:
        result = subprocess.run(
            [EXECUTABLE, filename, str(t)], 
            capture_output=True, text=True, timeout=120
        )
        output = result.stdout.strip()
        time_match = re.search(r"Cost Time:\s*(\d+)", output)
        if time_match:
            return float(time_match.group(1)) / 1_000_000
        return 0.0
    except:
        return 0.0

# ---------------------------
# Benchmark Thread
# ---------------------------
def run_benchmark_thread(config, btn_run):
    try:
        n_range = np.arange(config['n_start'], config['n_end'] + 1, config['n_delta'])
        t_range = np.arange(config['t_start'], config['t_end'] + 1, config['t_step'])
        N, T = np.meshgrid(n_range, t_range)
        Z = np.zeros(N.shape)
        
        with open(LOG_FILE, 'w') as log:
            log.write("--- Merge Sort Benchmark Log ---\n")
            log.write(f"N: {config['n_start']} ~ {config['n_end']}\n")
            log.write(f"T: {config['t_start']} ~ {config['t_end']}\n\n")

            for j, n in enumerate(n_range):
                generate_random_file(n, TEMP_INPUT)
                
                log.write(f"\n[ N = {n} ]\n")
                print(f"[ N = {n} ]")

                for i, t in enumerate(t_range):
                    total_time = 0.0
                    for _ in range(config['iterations']):
                        total_time += get_execution_time(TEMP_INPUT, t)

                    avg_time = total_time / config['iterations']
                    Z[i, j] = avg_time

                    msg = f"Thread {t:2d} | Avg Time = {avg_time:.6f}s"
                    print(msg)
                    log.write(msg + "\n")

        np.savez(DATA_FILE, N=N, T=T, Z=Z)

        if os.path.exists(TEMP_INPUT):
            os.remove(TEMP_INPUT)

        root.after(0, lambda: messagebox.showinfo("완료", f"로그 저장됨: {LOG_FILE}"))

    except Exception as e:
        root.after(0, lambda: messagebox.showerror("에러", str(e)))
    finally:
        root.after(0, lambda: btn_run.config(state=tk.NORMAL, text="Run Benchmark"))

# ---------------------------
# 그래프
# ---------------------------
def show_graphs():
    if not os.path.exists(DATA_FILE):
        messagebox.showwarning("No Data", "먼저 실행하세요")
        return

    data = np.load(DATA_FILE)
    N, T, Z = data['N'], data['T'], data['Z']
    n_unique = N[0, :]

    fig = plt.figure(figsize=(10, 7))
    plt.subplots_adjust(bottom=0.25)

    ctx = {
        'mode': '3D',
        'ax': fig.add_subplot(111, projection='3d')
    }

    ax_button = plt.axes([0.8, 0.05, 0.15, 0.075])
    btn_toggle = Button(ax_button, '2D')

    ax_slider = plt.axes([0.2, 0.1, 0.5, 0.03])
    slider_n = Slider(ax_slider, 'N', 0, len(n_unique)-1, valinit=0, valstep=1)
    ax_slider.set_visible(False)

    def draw_3d():
        ctx['ax'].clear()
        fig.delaxes(ctx['ax'])
        ctx['ax'] = fig.add_subplot(111, projection='3d')
        ctx['ax'].plot_surface(N/1000, T, Z, cmap='viridis')
        ctx['ax'].set_title('3D Performance')
        fig.canvas.draw_idle()

    def draw_2d(val=None):
        if ctx['mode'] == '3D':
            return
        idx = int(slider_n.val)
        ctx['ax'].clear()
        ctx['ax'].plot(T[:, idx], Z[:, idx])
        ctx['ax'].set_title(f'N = {n_unique[idx]}')
        fig.canvas.draw_idle()

    def toggle_mode(event):
        if ctx['mode'] == '3D':
            ctx['mode'] = '2D'
            btn_toggle.label.set_text('3D')
            ax_slider.set_visible(True)
            fig.delaxes(ctx['ax'])
            ctx['ax'] = fig.add_subplot(111)
            draw_2d()
        else:
            ctx['mode'] = '3D'
            btn_toggle.label.set_text('2D')
            ax_slider.set_visible(False)
            draw_3d()

    btn_toggle.on_clicked(toggle_mode)
    slider_n.on_changed(draw_2d)

    draw_3d()
    plt.show()

# ---------------------------
# GUI
# ---------------------------
def start_app():
    global root
    root = tk.Tk()
    root.title("Merge Sort Benchmarker")
    root.geometry("420x520")

    bg = setup_style()
    root.configure(bg=bg)

    container = ttk.Frame(root, padding=20)
    container.pack(fill="both", expand=True)

    title = ttk.Label(container, text="Merge Sort Benchmarker", font=("Segoe UI", 16, "bold"))
    title.pack(pady=(0, 20))

    vars_dict = {
        'N Start': tk.StringVar(value="100000"),
        'N End': tk.StringVar(value="1000000"),
        'N Delta': tk.StringVar(value="100000"),
        'T Start': tk.StringVar(value="2"),
        'T End': tk.StringVar(value="99"),
        'T Step': tk.StringVar(value="1"),
        'Iterations': tk.StringVar(value="10"),
    }

    form = ttk.Frame(container)
    form.pack()

    for i, (label, var) in enumerate(vars_dict.items()):
        ttk.Label(form, text=label).grid(row=i, column=0, sticky="e", padx=10, pady=8)
        ttk.Entry(form, textvariable=var, width=18).grid(row=i, column=1, padx=10, pady=8)

    def on_run():
        try:
            config = {k.lower().replace(" ", "_"): int(v.get()) for k, v in vars_dict.items()}
        except:
            messagebox.showerror("오류", "정수 입력")
            return

        btn_run.config(state=tk.DISABLED)
        threading.Thread(target=run_benchmark_thread, args=(config, btn_run), daemon=True).start()

    btn_run = ttk.Button(container, text="Run Benchmark", style="Accent.TButton", command=on_run)
    btn_run.pack(fill="x", pady=(25, 10))

    btn_graph = ttk.Button(container, text="Show Graphs", command=show_graphs)
    btn_graph.pack(fill="x")

    root.mainloop()

if __name__ == "__main__":
    start_app()
