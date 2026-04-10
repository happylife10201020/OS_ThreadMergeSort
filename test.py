import sys

def check_sorted(filename):
    with open(filename, 'r') as f:
        content = f.read()

    # "Cost Time:" 이후 제거
    content = content.split("Cost Time:")[0].strip()

    numbers = list(map(int, content.split()))

    if not numbers:
        print("숫자가 없습니다.")
        return

    for i in range(len(numbers) - 1):
        if numbers[i] > numbers[i + 1]:
            print(f"정렬 실패: index {i} → {numbers[i]} > {numbers[i+1]}")
            return

    print(f"정렬 완료: {len(numbers)}개 숫자 확인")

if __name__ == "__main__":
    filename = sys.argv[1] if len(sys.argv) > 1 else "output.txt"
    check_sorted(filename)
