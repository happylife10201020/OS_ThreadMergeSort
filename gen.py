import subprocess
import numpy as np
import matplotlib.pyplot as plt
import os
import random
import re

# --- 설정 ---
EXECUTABLE = "./mergesort"  # 실행 파일 이름 확인
TEMP_INPUT = "temp_input.txt" # 테스트용 임시 파일 이름

# N(데이터 개수)과 T(쓰레드 개수) 범위
N_START, N_END, N_DELTA = 1000000, 10000000, 1000000
T_START, T_END, T_STEP = 1, 16, 1
ITERATIONS = 2 # 반복 횟수 

def generate_random_file(n, filename):
    """n개의 랜덤한 숫자 인풋 파일 생성 ."""
    with open(filename, 'w') as f:
        numbers = [str(random.randint(1, n * 10)) for _ in range(n)]
        f.write("\n".join(numbers))

def get_execution_time(filename, t):
    """./mergesort <filename> <t> 실행 후 'Cost Time: 000' 에서 숫자만 추출합니다."""
    try:
        result = subprocess.run(
            [EXECUTABLE, filename, str(t)], 
            capture_output=True, text=True, timeout=20
        )
        output = result.stdout.strip()
        
        # 정규표현식으로 'Cost Time: 숫자' 패턴을 찾는다 
        match = re.search(r"Cost Time:\s*(\d+)", output)
        
        if match:
            # 찾은 숫자를 마이크로초 -> 초 단위로 변환 (/ 1,000,000)
            micro_seconds = float(match.group(1))
            return micro_seconds / 1_000_000
        else:
            # 만약 패턴을 못 찾았다면 디버깅을 위해 출력 내용을 표시 
            if output:
                print(f"\n[Parsing Error] Unexceptied output: {output[-50:]}")
            return 0.0
            
    except Exception as e:
        print(f"\n[ERROR] T={t} 실행 중 문제 발생: {e}")
        return 0.0

# --- 데이터 수집 ---
n_range = np.arange(N_START, N_END + 1, N_DELTA)
t_range = np.arange(T_START, T_END + 1, T_STEP)
N, T = np.meshgrid(n_range, t_range)
Z = np.zeros(N.shape)

print("성능 분석 벤치마킹 시작...")

for j, n in enumerate(n_range):
    # 각 N 단계마다 새로운 입력 파일 생성
    print(f"\n[N = {n}] Making Data Files...", end="")
    generate_random_file(n, TEMP_INPUT)
    print(" Done.")
    
    for i, t in enumerate(t_range):
        times = []
        for r in range(ITERATIONS):
            exec_time = get_execution_time(TEMP_INPUT, t)
            if exec_time > 0 :
                times.append(exec_time)

        if times:
            Z[i, j] = sum(times) / len(times)
        else:
            z[i, j] = 0.0


        print(f"  └ T={t:2d} -> Avg Time: {Z[i, j]:.4f}s", end='\r')

# 테스트 완료 후 임시 파일 삭제
if os.path.exists(TEMP_INPUT):
    os.remove(TEMP_INPUT)

print("\n\nDone")

# --- 3D 그래프 그리기 ---
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
