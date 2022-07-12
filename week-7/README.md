# COMP SCI 537 Discussion Week 7

## `libpthread`: thread library in C

POSIX.1 specifies a set of interfaces (functions, header files)
for threaded programming commonly known as POSIX threads, or
Pthreads.  A single process can contain multiple threads, all of
which are executing the same program.  These threads share the
same global memory (data and heap segments), but each thread has
its own stack.


### Create a new thread.

The pthread_create() function starts a new thread in the calling
process.  The new thread starts execution by invoking
start_routine(); arg is passed as the sole argument of
start_routine().

```C
int pthread_create(pthread_t *restrict thread,
    const pthread_attr_t *restrict attr,
    void *(*start_routine)(void *),
    void *restrict arg);
```

Example Code:
This is how one might call pthread_create function but it doesn't do much error handling. does it?
```C
pthread_create(&p1, NULL, NULL, "A")
```
Thus in our code, it will look like this so that macro can do error handling.
 ```C  
  Pthread_create(&p1, NULL, NULL, "A"); 
``` 

Error handling macro
```C 
#define Pthread_create(thread, attr, start_routine, arg) assert(pthread_create(thread, attr, start_routine, arg) == 0);
 ```

What about attributes which we have conveniently set as NULL? Mostly, we set them as NULL and let libpthread decide appropriate values for us.
                                                             
Thread attributes:                                           

| Attribute | Example Value |
|-----------|---------------|              
| Detach state     | PTHREAD_CREATE_JOINABLE                 |
| Scope      | PTHREAD_SCOPE_SYSTEM            |
| Inherit scheduler     | PTHREAD_INHERIT_SCHED            |
| Scheduling policy     |SCHED_OTHER                 |
| Scheduling priority    |  0              |
| Guard size   | 4096 bytes                  |
| Stack address     | 88            |
| Stack size     | 0x201000 bytes             |

                 
### Time to try them out.


Step 1 : Compile program for t0 using the makefile. If you view the makefile, you can observe that we need to add an extra flag `-pthread` when compiling a program that uses the threading library, as shown in the example below.

`gcc -o t0 t0.c -Wall -pthread`

In our case, since we have already created a makefile:

`make t0`

Step 2 : Run it.

`./t0`

### One possible output.  

```
main: begin
B
A
main: end
```   

### Another possible output.  
                       
```                     
main: begin             
A                       
B                       
main: end               
```        

Why do these different variations exist? Think about their parallel nature.


## Simple Pthreads doing useful work

Time to count our work using counters....

```C

void *mythread(void *arg) 
{
    char *letter = arg;
    int i; // stack (private per thread) 
    printf("%s: begin [addr of i: %p]\n", letter, &i);
    for (i = 0; i < max; i++) {
    counter = counter + 1; // shared: only one
    }
    printf("%s: done\n", letter);
    return NULL;
}
   
```

Step 1 : Compile program for t1  

`make t1`

Step 2 : Run it.

`./t1 <loop count (int)>`

## Mutex
(see `multi-threading.c`)

Below are some APIs that you need to know for pthread mutex.

```C
#include <pthread.h>

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_init(pthread_mutexattr_t *attr);

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
```

To create a mutex you need to declare a `pthread_mutex_t` variable and initialize it. Recall in C, you must explicitly initialize the variable otherwise it will just contain some random bits.

```C
pthread_mutex_t mutex;
pthread_mutexattr_t mutex_attr;

// Initialize the attr first
pthread_mutexattr_init(&mutex_attr);
// If more fancy settings needed, you may need to call `pthread_mutexattr_setXXX(&mutex_attr, ...)` to further configure the attr

// Finally, use the attr to initialize the mutex
pthread_mutex_init(&mutex, &mutex_attr);

// Now that you are done with the attr, you should destroy it
pthread_mutexattr_destroy(&mutex_attr);
```

Alternatively, if you only want the default settings, you could just use the macro `PTHREAD_MUTEX_INITIALIZER` and assignment to initialize it.

```C
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
```

To lock/unlock the mutex

```C
pthread_mutex_lock(&mutex);
pthread_mutex_unlock(&mutex);
```

Finally, when you are done with the mutex, you should destroy it.

```C
pthread_mutex_destroy(&mutex);
```

After destroying the mutex, all calls to this mutex variable will fail. Sometimes it won't cause any problems if you don't explicitly destroy it, but it is always a good practice to do so. See this [link](https://stackoverflow.com/questions/14721229/is-it-necessary-to-call-pthread-mutex-destroy-on-a-mutex) for more details.


## Condition Variable
[Why is CV required?](https://stackoverflow.com/questions/12551341/when-is-a-condition-variable-needed-isnt-a-mutex-enough)

Below are some functions (from the man page) that you may need to know for the next project.

```C
#include <pthread.h>

int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);

int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *cond_attr);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_destroy(pthread_cond_t *cond);
```

To create a condition variable, you need to declare a `pthread_cond_t` variable and initialize it. pthread supports more complicated settings, which can be set by `pthread_condattr_t`. For this project, you don't need these settings, so just use the default one.

```C
pthread_cond_t cond_var;
pthread_condattr_t cond_var_attr;

pthread_condattr_init(cond_var_attr);
pthread_cond_init(&cond_var, &cond_var_attr);
```

Alternatively, if you only want the default settings, you could use the macro `PTHREAD_COND_INITIALIZER`.

```C
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;
```

To wait on the condition variable, recall you must acquire the mutex first

```C
pthread_mutex_lock(&mutex);
// access/modify some shared variables

while (some_check())
    pthread_cond_wait(&cond_var, &mutex);

pthread_mutex_unlock(&mutex);
```
[QUIZ](https://docs.oracle.com/cd/E19455-01/806-5257/6je9h032r/index.html): `Why while-loop here?`

To signal the waiting thread:

```C
pthread_cond_signal(&cond_var);
```

Depending on your implementation, you may or may not acquire the mutex when signaling the condition variable.

Again, you should destroy the condition variable after you're done with it.

```C
pthread_cond_destroy(&cond_var);
```

You should read the man page to fully understand the details.
