import random

# 1부터 1,000,000 사이의 무작위 숫자 100만 개 생성
numbers = [str(random.randint(1, 1000000)) for _ in range(1000000)]

# 파일에 저장 (한 줄에 공백으로 구분하여 저장)
with open("input.txt", "w") as f:
    f.write(" ".join(numbers))

print("input.txt 생성 완료! (1,000,000개)")
