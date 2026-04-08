# plot_time.py

import subprocess
import re
import matplotlib.pyplot as plt

# 실행 파일 및 입력 파일
EXEC = "./mergesort"
INPUT_FILE = "test.txt"

# 테스트 범위
MAX_THREADS = 99
REPEAT = 100  # 각 N마다 실행 횟수 (평균용)

N_values = []
times = []

for N in range(2, MAX_THREADS + 1):
    total_time = 0
    success_count = 0

    for _ in range(REPEAT):
        result = subprocess.run(
            [EXEC, INPUT_FILE, str(N)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        # stderr에서 시간 추출
        match = re.search(r"Cost Time: (\d+)", result.stdout)
        if match:
            total_time += int(match.group(1))
            success_count += 1
        else:
            print(f"[경고] 시간 파싱 실패 (N={N})")

    if success_count > 0:
        avg_time = total_time / success_count
        N_values.append(N)
        times.append(avg_time)
        print(f"N={N}, avg_time={avg_time:.2f} us")

# 그래프 출력
plt.figure()
plt.plot(N_values, times, marker='o')
plt.xlabel("Thread Count (N)")
plt.ylabel("Execution Time (microseconds)")
plt.title("Thread Count vs Execution Time (Merge Sort)")
plt.grid()

plt.show()
