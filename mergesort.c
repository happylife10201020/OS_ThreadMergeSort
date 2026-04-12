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

#define THRESHOLD 32 //32개 이하 삽입정렬 
#define MAX_TASKS 65536
#define PARALLEL_MERGE_THRESHOLD 100000 //이 크기 이하 구간은 순차 

//스레드 정렬을 위한 구조체 
typedef struct{ 
    int* arr ;
    int* temp ;
    int left ;
    int right ;
}ThreadData ;

//스레드 병합을 위한 구조체(작업 큐의 노드)
typedef struct{
    int left, mid, right ;
    // struct MergeTask* next ;
}MergeTask ;

//thread pool
typedef struct {
    pthread_t*      threads ;
    MergeTask*      tasks ;
    int             head ;
    int             tail ;
    int             task_count ;
    int             active ;
    // MergeTask*      queue_head ;
    // MergeTask*      queue_tail ;
    pthread_mutex_t mutex ;
    pthread_cond_t cond_task ; // 작업 생겼을 때 깨움 
    pthread_cond_t cond_done ; // 작업 다 끝났을 때 메인에 확인
    // int             active_tasks ; // 현재 진행 중인 작업 수 
    int             num_threads ;
    int             shutdown ;
    int*            arr ;
    int*            temp ;
} ThreadPool ;




void merge(int*, int, int, int, int*) ;
void mergeSort(int*, int, int, int*) ;
void* threadMergeSort(void* ) ; 
void insertionSort(int[], int, int) ;

void* fast_itoa(int, char*) ; // int -> string
                              

void* worker(void*) ; // 스레드 병합 병렬 




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
        if(argc == 2 && strcmp(argv[1], "--help") == 0){ // --help 구현 
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


    //================================= open files
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

    //문자로 파싱 
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
    
    //merge도 병렬로 
    ThreadPool pool ;
    pool.arr            = arr ;
    pool.temp           = global_temp ;
    pool.num_threads    = N ;
    pool.shutdown       = 0 ;
    //pool.active_tasks   = 0 ;
    //pool.queue_head     = NULL ;
    //pool.queue_tail     = NULL ;
    pool.active         = 0 ;
    pool.task_count     = 0 ;
    pool.head           = 0 ;
    pool.tail           = 0 ;
    pool.tasks          = malloc(sizeof(MergeTask) * MAX_TASKS) ;

    pthread_mutex_init(&pool.mutex, NULL) ;
    pthread_cond_init(&pool.cond_task, NULL) ;
    pthread_cond_init(&pool.cond_done, NULL) ;

    //make worker thread
    pool.threads = malloc(sizeof(pthread_t) * N) ;
    for(int i = 0; i < N; i++)
         pthread_create(&pool.threads[i], NULL, worker, &pool) ;


    int step = chunk ;
    while(step < count) {
        /*for(int i = 0; i<count; i+= 2* step) {
            // int left = i ;
            int mid = i + step - 1 ;
            int right = i + 2 * step - 1 ;
            if(mid >= count) continue ;
            if(right >= count) right = count - 1 ;
            merge(arr, i, mid, right, global_temp) ;
        }
        step *= 2 ;*/
        
        // 작으면 순차 처리
        if(step * 2 <= PARALLEL_MERGE_THRESHOLD) {
            for(int i = 0; i<count; i+= 2* step) {
                // int left = i ;
                int mid = i + step - 1 ;
                int right = i + 2 * step - 1 ;
                if(mid >= count) continue ;
                if(right >= count) right = count - 1 ;
                merge(arr, i, mid, right, global_temp) ;
            }
            step *= 2 ;
            continue ;
        }

        
        pthread_mutex_lock(&pool.mutex) ;
        for(int i = 0 ; i < count ; i += 2 * step ) {
            int mid = i + step - 1 ;
            int right = i + 2 * step - 1 ;
            if(mid >= count) continue ;
            if(right >= count) right = count - 1 ;

            //MergeTask* task = malloc(sizeof(MergeTask)) ;
            //task->left = i ;
            //task->mid = mid ;
            //task->right = right ;
            //task->next = NULL ;

            //add to queue
            /*
            if(pool.queue_tail) 
                pool.queue_tail->next = task ;
            else 
                pool.queue_head = task ;
            pool.queue_tail = task ;
            pool.active_tasks++ ;
            */
            pool.tasks[pool.tail % MAX_TASKS] = (MergeTask){i, mid, right} ;
            pool.tail++ ;
            pool.task_count++ ;
        }

        pthread_cond_broadcast(&pool.cond_task) ; //워커 깨우기
        while(pool.active > 0 || pool.task_count > 0) 
            pthread_cond_wait(&pool.cond_done, &pool.mutex) ;
        pthread_mutex_unlock(&pool.mutex) ;

        step *= 2 ;
    }

    // 스레드 풀 종료
    pthread_mutex_lock(&pool.mutex) ;
    pool.shutdown = 1 ;
    pthread_cond_broadcast(&pool.cond_task) ; // 자고 있는 워커 깨워서 종료
    pthread_mutex_unlock(&pool.mutex) ;

    for(int i = 0 ; i < N; i++ ) 
        pthread_join(pool.threads[i], NULL) ;

    free(pool.tasks) ;
    free(pool.threads) ;
    pthread_mutex_destroy(&pool.mutex) ;
    pthread_cond_destroy(&pool.cond_task) ;
    pthread_cond_destroy(&pool.cond_done) ;


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


void* worker(void* arg) {
    ThreadPool* pool = (ThreadPool*) arg ;
    while(1) {
        pthread_mutex_lock(&pool->mutex) ;
        //작업 없으면 대기
        while(pool->task_count == 0 && !pool->shutdown)
            pthread_cond_wait(&pool->cond_task, &pool->mutex) ;

        if(pool->shutdown && pool->task_count == 0) {
            pthread_mutex_unlock(&pool->mutex) ;
            return NULL ;
        }
        //작업 꺼내기
        /*
        MergeTask* task = pool->queue_head ;
        pool->queue_head = task->next ;
        if(!pool->queue_head)
            pool->queue_tail = NULL ;
        pthread_mutex_unlock(&pool->mutex) ;
        */
        MergeTask task = pool->tasks[pool->head % MAX_TASKS] ;
        pool->head++ ;
        pool->task_count-- ;
        pool->active++ ;
        pthread_mutex_unlock(&pool->mutex) ;

        //mrege
        
        merge(pool->arr, task.left, task.mid, task.right, pool->temp) ;
        // free(task) ;
        

        //완료 알림
        pthread_mutex_lock(&pool->mutex) ;
        pool->active-- ;
        if(pool->active == 0 && pool->task_count == 0) 
            pthread_cond_signal(&pool->cond_done) ;
        pthread_mutex_unlock(&pool->mutex) ;
    }
}
