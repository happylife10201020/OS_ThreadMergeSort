# OS Assignment 1: Multi-threaded Merge Sort

본 프로젝트는 운영체제 과제 제출용으로 작성된 멀티스레드 기반 병합 정렬(Merge Sort) 프로그램과, 하드웨어별 성능 분석을 위한 파이썬 시각화 도구를 포함하고 있습니다.

## Assignment 01
- 과목명: 운영체제 (01분반)
- 지도교수: 조희승 교수님
- 학번: 2022041069
- 이름: 이인수

## 개발 및 검증 환경
- 개발 환경: macOS (Apple Silicon M4)
- 교차 검증 환경: Ubuntu 64-bit (VM)
- 컴파일러: GCC (Apple clang / gcc)

---

## C 프로그램 단독 실행 방법

메모리 매핑(mmap)과 pthread를 활용하여 구현된 정렬 프로그램입니다.

### 컴파일
```bash
gcc -O3 -o mergesort mergesort.c -pthread
```

### 실행
입력 파일 경로와 생성할 스레드 개수를 인자로 전달합니다.
```bash
./mergesort <input_file> <thread_count>
```

실행 예시:
```bash
./mergesort input.txt 8
```

---

## 성능 시각화 도구 실행 방법

GUI 환경에서 데이터 크기와 스레드 개수를 조절하며 성능을 측정할 수 있는 벤치마킹 앱입니다. 파이썬 패키지 관리는 속도 향상과 격리를 위해 `uv`를 사용합니다.

### 1. 사전 요구 사항 (Tkinter)
GUI 출력을 위해 파이썬 내장 라이브러리인 Tkinter가 시스템에 설치되어 있어야 합니다.
- macOS: `brew install python-tk`
- Linux (Ubuntu): `sudo apt-get install python3-tk`

### 2. 가상 환경 생성 및 패키지 설치
프로젝트 디렉토리 내부에서 다음 명령어를 순서대로 실행합니다.

```bash
# 가상 환경 생성
uv venv

# 가상 환경 활성화
source .venv/bin/activate

# 의존성 패키지 설치 (numpy, matplotlib 등)
uv pip install -r requirements.txt
```

### 3. 프로그램 실행
```bash
python benchmark_app.py
```

---

## 실험 결과 분석

하드웨어 아키텍처에 따른 스레드 할당 효율성 및 암달의 법칙(Amdahl's Law) 적용 양상을 교차 테스트한 결과입니다.

### 1. macOS (Apple Silicon M4)
P-core가 다수 존재하는 M4 칩의 특성상 일정 스레드(약 8개)까지는 유의미한 성능 향상(Speedup)이 관찰됩니다. 그 이상 스레드를 할당할 경우, 병렬화되지 않는 최종 병합(Final Merge) 구간의 병목과 컨텍스트 스위칭 오버헤드로 인해 성능이 하락하는 양상을 보입니다.

- 3D 성능 그래프: ![macOS 3D Graph](./img/3D_Mac01.png)
- 구동 화면 기록: ![macOS Execution](./gif/2D_Mac.gif)

### 2. Linux (Ubuntu VM)
물리 코어가 제한적으로 할당된 가상 머신 환경의 특성상, 스레드가 2개를 초과하여 증가할 때 병렬 처리로 얻는 연산 이득보다 스레드 생성 및 컨텍스트 스위칭 비용, 자원 경합(Resource Contention)이 더 크게 발생하여 성능이 점진적으로 저하되는 경향을 보입니다.

- 3D 성능 그래프: ![Linux 3D Graph](./img/3D_Linux.png)
- 구동 화면 기록: ![Linux Execution](./gif/2D_Linux.gif)
