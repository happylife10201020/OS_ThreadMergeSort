 /* OS Assignment: Multi-threaded Merge Sort
  * professor: 조희승
 * std num: 2022041069
 * name: 이인수
 *
 * Development Environment
 *  -os: macOS (Apple Silicon M4)
 *  -Editor: Neovim (nvim)
 *  -Compiler: GCC (Apple clang/gcc) 
 *
 * Excution Environment
 *  - Verified to run correctly on MacOS (Apple Silicon M4)
 *  - Also tested and verified on Ubuntu 64-bit env (VM)
 *
 * Comilation & Excution
 *  1. gcc -O3 -o mergesort mergesort.c -pthread
 *  2. ./mergesort <input_file> <thread_count>
 *  (ex. ./mergesort data.arr 10
 *
 * Explanation
 *  pthread를 사용해 N개의 스레드 수 만큼 병렬 합병 정렬을 수행한다.
 * */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<sys/time.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<stdbool.h>

#define THRESHOLD 32 //32개 이하 삽입정

typedef struct{
    int* arr ;
    int* temp ;
    int left ;
    int right ;
}ThreadData ;

void merge(int*, int, int, int, int*) ;
void mergeSort(int*, int, int, int*) ;
void* threadMergeSort(void* ) ; 
void insertionSort(int[], int, int) ;

void* fast_itoa(int, char*) ;


/**
 * brief		Multi-Thread방식으로 MergeSort를 구현
 * *@param	crgv[1] : string	input file
 * @param	crgv[2] : natural int	the number of thread
 */
int main(int argc, char* argv[]) {
    //================================= start time count 
    struct timeval startTime, endTime ;
    gettimeofday(&startTime, NULL) ;


    //================================= 입력값 검증 
    if(argc != 3){
        if(argc == 2 && strcmp(argv[1], "--help") == 0){ // --help 구
            printf(
                    "\tUsage: %s <file> <N>\n"
                    "\t <file> : path to input file containing natural integers\n"
                    "\t <N> : number of threads\n"
                    "\n\tExample: %s input.txt 4\n",
                    argv[0], argv[0]
                  ) ;
        } else {
            fprintf(stderr, 
                    "\tUsage Error: %s <file> <N> \n"
                    "\tTry '%s --help' for more information. \n" ,
                    argv[0], argv[0]
                   ) ;
        }
        return 1 ;
    } else {
	if(atoi(argv[2]) <= 0) {
		fprintf(stderr, "N should bigger than 0\n") ;
		return 1 ;
	}
    }


    //================================= open files현
    /*char* fileName = argv[1];
    int N = atoi(argv[2]);

    FILE *fp ;
    fp = fopen(fileName, "r") ;

    if(fp == NULL) {
        fprintf(stderr, "Failed to open file %s\n", fileName) ;
        return 1 ;
    }
    if(N == 0) {
        fprintf(stderr, "N cannot be 0\n") ;
        return 싱1 ;
    }

    //================================= 배열 만들기 
    int capacity = 10 ; 
    int* arr = malloc(sizeof(int) * capacity) ;
    int count = 0 ;
    char* line = NULL ;
    size_t len = 0 ;

    while(getline(&line, &len, fp) != -1) {
        char* ptr = line ;
        char* end ;
        while(1) {
            long num = strtol(ptr, &end, 10) ;
            if(ptr == end) break ;
            if(count == capacity) {
                capacity *= 2 ;
                arr = realloc(arr, capacity *sizeof(int)) ;
            }
            arr[count++] = (int)num ;
            ptr = end ;
        }
    }*/
   

    //================================= mmap 사용한 고속 입출력
	
    int N = atoi(argv[2]) ;
    int fd = open(argv[1], O_RDONLY) ;

    if(fd < 0) {
        perror("Failed to open file\n");
        return 1 ;
    }
    
    struct stat sb ;
    fstat(fd, &sb) ;
    char* file_addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0) ;
    if(file_addr == MAP_FAILED) {
        perror("mmap failed") ;
        return 1 ;
    }

    // 숫자 개수 파악
    int* arr= malloc(sizeof(int) * (sb.st_size / 2)) ;
    size_t count = 0 ;
    char* p = file_addr ;
    char* end_p = file_addr + sb.st_size ;

    //문자로 파
    bool error_flag = 0 ;

    while( p< end_p) {
        if(*p == ' ' || *p == '\n' || *p == '\t') {
            p++ ;
            continue ;
        }
    	if((*p >= '0' && *p <= '9') || *p == '-') {
	    	int sign = 1 ;
	    	if(*p == '-') {
                sign = -1; 
                p++; 

                if(!(p < end_p && *p >= '0' && *p <= '9')) {
                    error_flag = 1 ;
                    break ;
                }
            }
	    	int num = 0 ;
	    	while(p < end_p && *p >= '0' && *p <= '9') {
	    		num = num * 10 + (*p - '0') ;
	    		p++ ;
	    	}
	    	arr[count++] = num * sign;
    	} else {
	        error_flag = 1 ;
            break ;
        }
    }

    if(error_flag) {
        fprintf(stderr, "Error: input file contains invalid character\n") ;
        munmap(file_addr, sb.st_size) ;
        close(fd) ;
        free(arr) ;
        return 1 ;
    }

    munmap(file_addr, sb.st_size) ;
    close(fd) ;

    if(count == 0) {
	    free(arr) ;
	    return 0 ;
    }


    //================================= make thread

    if(N > count) N = count ; // N 이 count보다 클 때
    int chunk = count / N ;
    pthread_t threads[N] ;
    ThreadData tdata[N] ;

    int* global_temp = malloc(sizeof(int) * count) ; // 전체 공유 버퍼


    for(int i = 0; i<N; i++) {
        tdata[i].arr = arr ;
    	tdata[i].temp = global_temp ;
        tdata[i].left = i * chunk ;

        if(i == N - 1) 
            tdata[i].right = count - 1 ;
        else
            tdata[i].right = (i + 1) * chunk - 1 ;
        
        //int size = tdata[i].right - tdata[i].left + 1 ;
        //tdata[i].temp = malloc(sizeof(int) * size) ;
    	pthread_create(&threads[i], NULL, threadMergeSort, &tdata[i]) ;
    }
    
    //================================= join
    for(int i = 0; i<N; i++ ) {
        pthread_join(threads[i], NULL) ;
        // free(tdata[i].temp) ; // 정렬 이후 전부 free
    }

    
    //================================= final merge
    int step = chunk ;
    while(step < count) {
        for(int i = 0; i<count; i+= 2* step) {
            // int left = i ;
            int mid = i + step - 1 ;
            int right = i + 2 * step - 1 ;
            if(mid >= count) continue ;
            if(right >= count) right = count - 1 ;
            merge(arr, i, mid, right, global_temp) ;
        }
        step *= 2 ;
    }
    //================================= output
    /*
    printf("%d", arr[0]) ;
    for(int i = 1; i < count; i++) {
        printf(" %d", arr[i]) ;
    }
    printf("\n") ;
    */
    
    //buffered write

    char* out_buf = malloc(count * 12) ; //숫자당 최대 11자 + 공백
    char* out_p = out_buf ;
    for(int i = 0; i<count; i++) {
	    out_p = fast_itoa(arr[i], out_p) ;
	    *out_p++ = (i == count - 1) ? '\n' : ' ' ;
    }
    fwrite(out_buf, 1, out_p - out_buf, stdout) ;
    
    //================================= free and print time
    //free(arr) ;
    //free(line) ;
    //fclose(fp) ;
    free(global_temp) ;
    free(out_buf) ;
    //free(temp) ;
    free(arr) ;
	

    gettimeofday(&endTime, NULL) ;
    long long diff = (endTime.tv_sec - startTime.tv_sec) * 1000000LL + (endTime.tv_usec - startTime.tv_usec) ;

    printf("Cost Time: %lld micro seconds\n", diff) ; //최종 소요시간 출력

    return 0 ;
}


/**
 * @breif		두 개의 정렬된 부분 배열을 하나의 정렬된 배열로 합침
 * * @param	arr	왼쪽 데이터 배열
 * @param 	left	왼쪽 부분 배열의 시작 위치
 * @param 	mid	왼쪽 부분 배열의 끝 위치
 * @param 	right	오른쪽 부분 배열의 끝 위치
 * @param 	temp	합병 과정에서 사용할 임시 공유 버퍼
 * @return 	void
 */
void merge(int* arr, int left, int mid, int right, int* temp) {
    int i = left, j = mid + 1, k = left ;

    while(i <= mid && j <= right) {
        if(arr[i] < arr[j]) temp[k++] = arr[i++] ;
        else temp[k++] = arr[j++] ;
    }

    while(i<= mid) temp[k++] = arr[i++] ;
    while(j <= right) temp[k++] = arr[j++] ;

    for(int t = left; t <= right; t++) 
        arr[t] = temp[t] ;
}



/**
 * @brief 		재귀적으로 배열을 분할하고 합병 정렬을 수행
 ** @param 	arr 	정렬할 정수 배열의 포인터
 * @param 	left	정렬 부분의 시작 인덱스
 * @param 	right	정렬 부분의 끝 인덱스
 * @param 	temp 	합병 과정에서 사용할 임시 공유 버퍼 : malloc을 한 번만 하기 위해
 * @return 	void
 */
void mergeSort(int* arr, int left, int right, int* temp) {
    // if(left >= right) return ;
    
    if(right - left + 1 <= THRESHOLD) {
        insertionSort(arr, left, right) ;
        return ;
    }
    int mid = (left + right) / 2 ;

    mergeSort(arr, left, mid, temp) ;
    mergeSort(arr, mid+1, right, temp) ;
    merge(arr, left, mid, right, temp) ;
}

void* threadMergeSort(void* args) {
    ThreadData* data = (ThreadData*)args ;
    mergeSort(data->arr, data->left, data->right, data->temp) ;
    return NULL ;
}

void insertionSort(int arr[], int left, int right) {
    for(int i = left + 1; i<= right; i++) {
        int key = arr[i] ;
        int j= i - 1 ;
        while(j >= left && arr[j] > key) {
            arr[j + 1] = arr[j] ;
            j-- ;
        }   
        arr[j + 1] = key ;
    }
}

/**
 * @brief		입출력 속도 향상을 위해 정수값을 문자열로 변환
 * * @param	val	변환할 정수
 * @param	buf	문자열이 저장될 버퍼의 현재 위치 포인터
 * @return void*	다음 숫자가 써질 버퍼의 위치 포인터 변환
 */
void* fast_itoa(int val, char* buf) {
    if (val == 0) { *buf++ = '0'; return buf; }
    unsigned int uval; // INT_MIN 절대값처리하면 오버플로우남 
    if (val < 0) {
        *buf++ = '-';
        uval = (unsigned int)-(val);
    } else {
        uval = (unsigned int)val;
    }
    char temp[12];
    int i = 0;
    while (uval > 0) {
        temp[i++] = (uval % 10) + '0';
        uval /= 10;
    }
    while (i > 0) {
        *buf++ = temp[--i];
    }
    return buf;
}
