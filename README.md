# OS Assignment 1: Multi-threaded Merge Sort

본 프로젝트는 운영체제 과제 제출용으로 작성된 멀티스레드 기반 병합 정렬(Merge Sort) 프로그램과, 하드웨어별 성능 분석을 위한 파이썬 시각화 도구를 포함한다.

## Assignment 01
- 과목명: 운영체제 (01분반)
- 지도교수: 조희승 교수님
- 학번: 2022041069
- 이름: 이인수

## 개발 및 검증 환경
- 통합 개발 환경(IDE): Neovim (nvim)
- 주 개발 환경: macOS (Apple Silicon M4)
- 교차 검증 환경: Ubuntu 64-bit (가상 머신, 할당 자원: CPU 4 Cores, Memory 12GB)
- 컴파일러: GCC (Apple clang / gcc)

---

## C 프로그램 단독 실행 방법

메모리 매핑(mmap)과 pthread를 활용하여 구현된 정렬 프로그램.

### 컴파일
```bash
gcc -O3 -o mergesort mergesort.c -pthread
```

### 실행
입력 파일 경로와 생성할 스레드 개수를 인자로 전달. 도움말을 확인하려면 `--help` 옵션을 사용.

```bash
# 도움말 출력
./mergesort --help

# 실행 예시 (input.txt 데이터를 8개의 스레드로 정렬)
./mergesort input.txt 8
```

### 실행 결과
단일 스레드와 다중 스레드 구동 시의 터미널 실행 결과 화면.

- 실행 결과 1: ![Execution Result 1](./img/Result_01.png)
- 실행 결과 2: ![Execution Result 2](./img/Result_02.png)
- 실행 결과 3: ![Execution Result 3](./img/Result_03.png)

---

## 성능 시각화 도구 실행 방법

GUI 환경에서 데이터 크기와 스레드 개수를 조절하며 성능을 측정할 수 있는 벤치마킹 앱. 파이썬 패키지 관리는 속도 향상과 격리를 위해 `uv`를 사용한다.

### 1. 사전 요구 사항 (Tkinter)
GUI 출력을 위해 파이썬 내장 라이브러리인 Tkinter가 시스템에 설치되어 있어야 한다.
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
P-core가 다수 존재하는 M4 칩의 특성상 멀티스레딩의 이점이 뚜렷하게 나타나며, 특히 **6~8개의 스레드를 할당했을 때 최적의 성능(Sweet Spot)**을 보dlsek. 

흥미로운 점은 스레드 개수가 $2^n + 1$ 개(예: 3, 5, 9, 17개...)가 될 때마다 데이터 분할 및 병합 과정의 불균형으로 인해 성능이 눈에 띄게 하락하는 패턴이 관찰된다는 것입니다. 또한, 스레드 개수를 99개까지 극단적으로 늘리더라도 I/O 병목 및 순차 처리 구간의 한계로 인해 전체 CPU 점유율은 약 40% 미만에서 더 이상 크게 증가하지 않는 현상도 확인된다.

- 3D 성능 그래프: ![macOS 3D Graph](./img/3D_Mac01.jpg)
- N의 개수별 2D 성능 그래프: ![macOS Execution](./gif/2D_Mac.gif)

### 2. Linux (Ubuntu VM)
물리 코어가 4개로 제한 할당된 가상 머신 환경의 특성상, 스레드가 가용 코어 수를 초과하여 증가할 때 병렬 처리로 얻는 연산 이득보다 스레드 생성 및 컨텍스트 스위칭 비용, 자원 경합(Resource Contention)이 더 크게 발생하여 성능이 점진적으로 저하되는 경향을 보인다.

- 3D 성능 그래프: ![Linux 3D Graph](./img/3D_Linux.png)
- N의 개수별 2D 성능 그래프: ![Linux Execution](./gif/2D_Linux.gif)
