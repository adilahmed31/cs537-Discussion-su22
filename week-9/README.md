# COMP SCI 537 Discussion Week 8

## Using GDB to debug concurrent code
When running a concurrent program, it might be useful to observe what each individual thread is doing to debug a race condition or concurrency bug.

Consider the producer consumer program below. `sleep` and `printf` statements have been added to slow the program down and to make debugging easier.

```c
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"

int max;
int loops;
int *buffer; 

int use_ptr  = 0;
int fill_ptr = 0;
int num_full = 0;

pthread_cond_t empty  = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m     = PTHREAD_MUTEX_INITIALIZER;

int consumers = 1;
int verbose = 1;


void do_fill(int value) {
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % max;
    num_full++;
}

int do_get() {
    int tmp = buffer[use_ptr];
    use_ptr = (use_ptr + 1) % max;
    num_full--;
    return tmp;
}

void *producer(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
	Mutex_lock(&m);            
	while (num_full == max){
        printf("Producer: Buffer full. Waiting for consumer to consume buffer\n");                       
	    Cond_wait(&empty, &m); 
    }
    printf("Producer: Filling buffer\n");
    sleep(2);
	do_fill(i);               
	Cond_signal(&fill);        
	Mutex_unlock(&m);          
    }

    // end case: put an end-of-production marker (-1) 
    // into shared buffer, one per consumer
    for (i = 0; i < consumers; i++) {
	Mutex_lock(&m);
	while (num_full == max) 
	    Cond_wait(&empty, &m);
	do_fill(-1);
	Cond_signal(&fill);
	Mutex_unlock(&m);
    }

    return NULL;
}
                                                                               
void *consumer(void *arg) {
    int tmp = 0;
    // consumer: keep pulling data out of shared buffer
    // until you receive a -1 (end-of-production marker)
    while (tmp != -1) { 
	Mutex_lock(&m);           
	while (num_full == 0){
        printf("Consumer: Waiting for buffer to be filled\n");      
	    Cond_wait(&fill, &m); 
    }
    printf("Consumer: fetching data from buffer\n");
    sleep(2);
	tmp = do_get();           
	Cond_signal(&empty);      
	Mutex_unlock(&m);         
    }
    return NULL;
}

int
main(int argc, char *argv[])
{
    if (argc != 4) {
	fprintf(stderr, "usage: %s <buffersize> <loops> <consumers>\n", argv[0]);
	exit(1);
    }
    max = atoi(argv[1]);
    loops = atoi(argv[2]);
    consumers = atoi(argv[3]);

    buffer = (int *) malloc(max * sizeof(int));
    assert(buffer != NULL);

    int i;
    for (i = 0; i < max; i++) {
	buffer[i] = 0;
    }

    pthread_t pid, cid[consumers];
    Pthread_create(&pid, NULL, producer, NULL); 
    for (i = 0; i < consumers; i++) {
	Pthread_create(&cid[i], NULL, consumer, (void *) (long long int) i); 
    }
    Pthread_join(pid, NULL); 
    for (i = 0; i < consumers; i++) {
	Pthread_join(cid[i], NULL); 
    }
    return 0;
}
```
The program is compiled and run as below:
```bash
$ gcc -o pc pc.c -pthread -g
$ pc
usage: pc <buffersize> <loops> <consumers>
$ ./pc 2 5 3
```

When running this program in gdb, we may wish to trace through the execution of a particular thread. Consider the gdb session paused after all threads have been created. We can run the command `info threads` to see the current threads active in the program.
```bash
(gdb) r 2 5 3
Starting program: /afs/cs.wisc.edu/u/o/b/obaidullahadil/private/adil/cs537-summer/Discussions/week-9/pc 2 5 3
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
[New Thread 0x7ffff7d90700 (LWP 248711)]
[New Thread 0x7ffff758f700 (LWP 248712)]
Consumer: Waiting for buffer to be filled
Producer: Filling buffer
[New Thread 0x7ffff6d8e700 (LWP 248713)]
[New Thread 0x7ffff658d700 (LWP 248714)]
^C
Thread 1 "pc" received signal SIGINT, Interrupt.
__pthread_clockjoin_ex (threadid=140737351583488, thread_return=0x0, clockid=<optimized out>, abstime=<optimized out>, block=<optimized out>)
    at pthread_join_common.c:145
145     pthread_join_common.c: No such file or directory.
(gdb) info threads
  Id   Target Id                               Frame 
* 1    Thread 0x7ffff7d91740 (LWP 248706) "pc" __pthread_clockjoin_ex (threadid=140737351583488, thread_return=0x0, clockid=<optimized out>, 
    abstime=<optimized out>, block=<optimized out>) at pthread_join_common.c:145
  2    Thread 0x7ffff7d90700 (LWP 248711) "pc" 0x00007ffff7e7123f in __GI___clock_nanosleep (clock_id=clock_id@entry=0, flags=flags@entry=0, 
    req=req@entry=0x7ffff7d8fe90, rem=rem@entry=0x7ffff7d8fe90) at ../sysdeps/unix/sysv/linux/clock_nanosleep.c:78
  3    Thread 0x7ffff758f700 (LWP 248712) "pc" futex_wait_cancelable (private=<optimized out>, expected=0, futex_word=0x5555555580c8 <fill+40>)
    at ../sysdeps/nptl/futex-internal.h:183
  4    Thread 0x7ffff6d8e700 (LWP 248713) "pc" __lll_lock_wait (futex=futex@entry=0x5555555580e0 <m>, private=0) at lowlevellock.c:52
  5    Thread 0x7ffff658d700 (LWP 248714) "pc" __lll_lock_wait (futex=futex@entry=0x5555555580e0 <m>, private=0) at lowlevellock.c:52
```
From the above info, we can observe that thread 2 is the producer while threads 3,4 and 5 are consumers. If not apparent, we can also refer to the order of thread creation in our code or put a breakpoint on the line where `Pthread_create` is called for the producer.

We can then switch to this thread using the command `thread 2` and then step through the code using `step` or `next`.
```bash
(gdb) thread 2
[Switching to thread 2 (Thread 0x7ffff7d90700 (LWP 249197))]
(gdb)
64      in ../sysdeps/posix/sleep.c
(gdb) 
producer (arg=0x0) at pc.c:49
49              do_fill(i);               
(gdb) 
50              Cond_signal(&fill);        
```

Or we can switch to the first consumer thread using `thread 3`. We might want to observe the current state of the global buffer using `print`.
```bash
(gdb) thread 3
[Switching to thread 3 (Thread 0x7ffff758f700 (LWP 249198))]
Producer: Buffer full. Waiting for consumer to consume buffer
Consumer: fetching data from buffer
(gdb) print *buffer
$2 = 0
Consumer: fetching data from buffer
51      in lowlevellock.c
(gdb) p *buffer
$8 = -1
(gdb) p num_full
$9 = 2
(gdb) n
Consumer: fetching data from buffer
52      in lowlevellock.c
(gdb) p num_full
$10 = 1
```

## Timing Code to identify performance bottlenecks

When optimizing your code for P4, it is important to identify which parts of your code are slow and which can be optimized further. This can be done by timing different parts of your code. It is important to take multiple time measurements and average them to deal with environmental variations. Of further importance is to make an apples to apples comparison, times can only be compared on machines with the same specs and environmental conditions. 

On Linux platforms, we can use the `clock_gettime()` or `gettimeofday()` to measure code. The trick is simple: measure the time before and after the code you want to time, and subtract it.

```c
t1 = clock_gettime();
//your code here
t2 = clock_gettime();
printf("Time taken: %d\n", t2 - t1);
```

Another important factor to consider is the precision of your clock, i.e. how small a time event can be measured with this timer accurately. To do this, you can start with a loop and measure the time before and after the loop. Gradually increase the number of iterations until you start to see a difference in the time samples. Be careful of compiler optimizations that may remove the code in the loop. The `clock_gettime()` function has higher precision (to the nano-second level) than `gettimeofday` (micro-seconds) and is recommended for use.


Consider the version of `sequential_mapreduce` below that prints out the time it took for each phase.

```c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "mapreduce.h"
#include "hashmap.h"
#include <time.h>

struct kv {
    char* key;
    char* value;
};

struct kv_list {
    struct kv** elements;
    size_t num_elements;
    size_t size;
};

struct kv_list kvl;
size_t kvl_counter;

void init_kv_list(size_t size) {
    kvl.elements = (struct kv**) malloc(size * sizeof(struct kv*));
    kvl.num_elements = 0;
    kvl.size = size;
}

void add_to_list(struct kv* elt) {
    if (kvl.num_elements == kvl.size) {
	kvl.size *= 2;
	kvl.elements = realloc(kvl.elements, kvl.size * sizeof(struct kv*));
    }
    kvl.elements[kvl.num_elements++] = elt;
}

char* get_func(char* key, int partition_number) {
    if (kvl_counter == kvl.num_elements) {
	return NULL;
    }
    struct kv *curr_elt = kvl.elements[kvl_counter];
    if (!strcmp(curr_elt->key, key)) {
	kvl_counter++;
	return curr_elt->value;
    }
    return NULL;
}

int cmp(const void* a, const void* b) {
    char* str1 = (*(struct kv **)a)->key;
    char* str2 = (*(struct kv **)b)->key;
    return strcmp(str1, str2);
}

void MR_Emit(char* key, char* value)
{
    struct kv *elt = (struct kv*) malloc(sizeof(struct kv));
    if (elt == NULL) {
	printf("Malloc error! %s\n", strerror(errno));
	exit(1);
    }
    elt->key = strdup(key);
    elt->value = strdup(value);
    add_to_list(elt);

    return;
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    return 0;
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers,
	    Reducer reduce, int num_reducers, Partitioner partition)
{
    struct timespec start, end;
    double cpu_time_used;
    

    init_kv_list(10);
    int i;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 1; i < argc; i++) {
	(*map)(argv[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (end.tv_sec - start.tv_sec);
    cpu_time_used += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("** Map stage took %.3f secs **\n", cpu_time_used);


    clock_gettime(CLOCK_MONOTONIC, &start);
    qsort(kvl.elements, kvl.num_elements, sizeof(struct kv*), cmp);
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (end.tv_sec - start.tv_sec);
    cpu_time_used += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("** Sort stage took %.3f secs **\n", cpu_time_used);

    // note that in the single-threaded version, we don't really have
    // partitions. We just use a global counter to keep it really simple
    kvl_counter = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (kvl_counter < kvl.num_elements) {
	(*reduce)((kvl.elements[kvl_counter])->key, get_func, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (end.tv_sec - start.tv_sec);
    cpu_time_used += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("** Reduce stage took %.3f secs **\n", cpu_time_used);
}
```





