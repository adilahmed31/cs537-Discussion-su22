# COMP SCI 537 Discussion Week 8

## Using GDB to debug concurrent code
When running a concurrent program, it might be useful to observe what each individual thread is doing to debug a race condition or concurrency bug.

Consider the producer consumer program below. Sleep and printf statements have been added to slow the program down and to make debugging easier.

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






