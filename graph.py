import subprocess
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Button, Slider
import os
import random
import re
import tkinter as tk
from tkinter import messagebox
import threading

# --- 기본 설정 ---
EXECUTABLE = "./mergesort"
TEMP_INPUT = "temp_input.txt"
DATA_FILE = "benchmark_data.npz" # 그래프를 그리기 위해 측정 데이터를 저장할 파일
LOG_FILE = "output.txt"          # 1번 모드: 실행 로그 저장 파일

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
    except Exception as e:
        print(f"[ERROR] T={t} Execution failed: {e}")
        return 0.0

# ==========================================
# 1. Benchmark Logic (Mode 1)
# ==========================================
def run_benchmark_thread(config, btn_run):
    """UI가 멈추지 않도록 별도의 쓰레드에서 벤치마킹을 실행합니다."""
    try:
        n_range = np.arange(config['n_start'], config['n_end'] + 1, config['n_delta'])
        t_range = np.arange(config['t_start'], config['t_end'] + 1, config['t_step'])
        N, T = np.meshgrid(n_range, t_range)
        Z = np.zeros(N.shape)
        
        with open(LOG_FILE, 'w') as log:
            log.write("--- Merge Sort Benchmark Log ---\n")
            log.write(f"N Range: {config['n_start']} to {config['n_end']} (Step: {config['n_delta']})\n")
            log.write(f"T Range: {config['t_start']} to {config['t_end']} (Step: {config['t_step']})\n")
            log.write(f"Iterations per case: {config['iterations']}\n\n")
            
            for j, n in enumerate(n_range):
                print(f"[Target N = {n}] Generating data...")
                generate_random_file(n, TEMP_INPUT)
                log.write(f"\n[ Target N = {n} ]\n")
                
                for i, t in enumerate(t_range):
                    total_time = 0.0
                    for r in range(config['iterations']):
                        total_time += get_execution_time(TEMP_INPUT, t)
                    
                    Z[i, j] = total_time / config['iterations']
                    
                    msg = f"  > Thread {t:2d}: Avg Time = {Z[i, j]:.4f}s"
                    print(msg)
                    log.write(msg + "\n")
                    
        # 측정한 데이터를 파일로 저장 (그래프 그릴 때 사용)
        np.savez(DATA_FILE, N=N, T=T, Z=Z)
        
        if os.path.exists(TEMP_INPUT):
            os.remove(TEMP_INPUT)
            
        print("Benchmarking Process Finished Successfully.")
        # UI 업데이트는 안전하게 메인 쓰레드에서 실행
        root.after(0, lambda: messagebox.showinfo("완료", f"벤치마킹이 완료되었습니다!\n로그가 '{LOG_FILE}'에 저장되었습니다."))
        
    except Exception as e:
        root.after(0, lambda: messagebox.showerror("에러", str(e)))
    finally:
        root.after(0, lambda: btn_run.config(state=tk.NORMAL, text="1. Run Benchmark & Save Log"))

# ==========================================
# 2. Graphing Logic (Mode 2)
# ==========================================
def show_graphs():
    if not os.path.exists(DATA_FILE):
        messagebox.showwarning("No Data", "No benchmark data found. Please run Mode 1 first.")
        return

    # Load data
    data = np.load(DATA_FILE)
    N, T, Z = data['N'], data['T'], data['Z']
    n_unique = N[0, :] 

    fig = plt.figure(figsize=(12, 8))
    plt.subplots_adjust(bottom=0.25)
    
    # State management using a dictionary to avoid nonlocal issues
    ctx = {
        'mode': '3D',
        'ax': fig.add_subplot(111, projection='3d')
    }
    
    # UI Elements
    ax_button = plt.axes([0.8, 0.05, 0.15, 0.075])
    btn_toggle = Button(ax_button, 'Switch to 2D')
    
    ax_slider = plt.axes([0.2, 0.1, 0.5, 0.03])
    slider_n = Slider(ax_slider, 'N Index', 0, len(n_unique)-1, valinit=0, valstep=1)
    ax_slider.set_visible(False) 
    
    def draw_3d():
        # Clear the current axes
        ctx['ax'].clear()
        fig.delaxes(ctx['ax'])
        
        # Re-create 3D axes
        ctx['ax'] = fig.add_subplot(111, projection='3d')
        plt.subplots_adjust(bottom=0.25)
        
        N_scaled = N / 1000
        surf = ctx['ax'].plot_surface(N_scaled, T, Z, cmap='viridis', edgecolor='none', alpha=0.9)
        ctx['ax'].set_xlabel('Number of Elements (K)')
        ctx['ax'].set_ylabel('Number of Threads (T)')
        ctx['ax'].set_zlabel('Execution Time (sec)')
        ctx['ax'].set_title('Merge Sort Performance (3D)')
        fig.canvas.draw_idle()

    def draw_2d(val=None):
        if ctx['mode'] == '3D': return
        
        idx = int(slider_n.val)
        actual_n = int(n_unique[idx])
        
        ctx['ax'].clear()
        ctx['ax'].plot(T[:, idx], Z[:, idx], marker='o', color='b', linewidth=2)
        ctx['ax'].set_xlabel('Number of Threads (T)')
        ctx['ax'].set_ylabel('Execution Time (sec)')
        ctx['ax'].set_title(f'Performance (2D Line) - N = {actual_n}')
        ctx['ax'].grid(True)
        fig.canvas.draw_idle()

    def toggle_mode(event):
        if ctx['mode'] == '3D':
            ctx['mode'] = '2D'
            btn_toggle.label.set_text('Switch to 3D')
            ax_slider.set_visible(True)
            
            # Switch to 2D subplot
            fig.delaxes(ctx['ax'])
            ctx['ax'] = fig.add_subplot(111)
            plt.subplots_adjust(bottom=0.25)
            draw_2d()
        else:
            ctx['mode'] = '3D'
            btn_toggle.label.set_text('Switch to 2D')
            ax_slider.set_visible(False)
            draw_3d()

    # Link events
    btn_toggle.on_clicked(toggle_mode)
    slider_n.on_changed(draw_2d)

    # Initial Draw
    draw_3d()
    plt.show()



# ==========================================
# 3. Main GUI Application
# ==========================================
def start_app():
    global root
    root = tk.Tk()
    root.title("Merge Sort Benchmarker")
    root.geometry("400x500")
    
    # 변수 설정 사전
    vars_dict = {
        'N Start': tk.StringVar(value="1000000"),
        'N End': tk.StringVar(value="10000000"),
        'N Delta': tk.StringVar(value="1000000"),
        'T Start': tk.StringVar(value="2"),
        'T End': tk.StringVar(value="16"),  # M4 칩에 맞게 기본값은 일단 16으로 조정
        'T Step': tk.StringVar(value="1"),
        'Iterations': tk.StringVar(value="10"),
    }
    
    # 설정 UI 생성
    tk.Label(root, text="벤치마크 파라미터 설정", font=("Arial", 16, "bold")).pack(pady=15)
    
    frame = tk.Frame(root)
    frame.pack(pady=10)
    
    row = 0
    for label_text, str_var in vars_dict.items():
        tk.Label(frame, text=label_text).grid(row=row, column=0, sticky='e', padx=10, pady=5)
        tk.Entry(frame, textvariable=str_var, width=15).grid(row=row, column=1, padx=10, pady=5)
        row += 1

    # 실행 버튼 콜백
    def on_run_clicked():
        try:
            config = {
                'n_start': int(vars_dict['N Start'].get()),
                'n_end': int(vars_dict['N End'].get()),
                'n_delta': int(vars_dict['N Delta'].get()),
                't_start': int(vars_dict['T Start'].get()),
                't_end': int(vars_dict['T End'].get()),
                't_step': int(vars_dict['T Step'].get()),
                'iterations': int(vars_dict['Iterations'].get())
            }
        except ValueError:
            messagebox.showerror("입력 오류", "모든 값은 정수로 입력해야 합니다.")
            return
            
        btn_run.config(state=tk.DISABLED, text="실행 중... (터미널 확인)")
        # UI가 얼지 않게 쓰레드로 실행
        threading.Thread(target=run_benchmark_thread, args=(config, btn_run), daemon=True).start()

    # 버튼들
    btn_frame = tk.Frame(root)
    btn_frame.pack(pady=20)
    
    btn_run = tk.Button(btn_frame, text="1. Run Benchmark & Save Log", command=on_run_clicked, width=25, height=2)
    btn_run.pack(pady=5)
    
    btn_graph = tk.Button(btn_frame, text="2. Show 3D / 2D Graphs", command=show_graphs, width=25, height=2)
    btn_graph.pack(pady=5)

    root.mainloop()

if __name__ == "__main__":
    start_app()
