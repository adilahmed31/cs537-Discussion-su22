# COMP SCI 537 Discussion Week 4

## Compensated Round-Robin Scheduler
Processes are assigned slices. The scheduler schedules the next process in the pool of READY processes in a round-robin manner. The process runs for the number of ticks defined by it's slice.

For e.g. We have 3 processes with the corresponding slice lenghts:
- Process A : 3 ticks
- Process B : 4 ticks
- Process C : 1 tick

Now consider the following scheduler scenario:


| Time      | Process Scheduled | 
| ----------- | ----------- |
| t = 1       | A|
| t = 2       | A|
| t = 3       | A|
| t = 4       | B|
| t = 5       | B|
| t = 6       | B|
| t = 7       | B|
| t = 8       | C|
| t = 9       | A|

All the processes run for the number of ticks that they were alotted. Now, consider the scenario that a process does not run for its allotted time slice and gives up the CPU when it is sleeping (i.e. blocked on some other activity such as I/O). To incentivize processes giving up the CPU when they don't require it, we will compensate the number of slices for which the process is actively sleeping. Consider an extension of the previous scenario:

| Time      | Process Scheduled | Sleeping |
| ----------- | ----------- | -----|
| t = 1       | A|  - | 
| t = 2       | A|  - |
| t = 3       | A|  - |
| t = 4       | B|  - |
| t = 5       | B|  - |
| t = 6       | C|  B |
| t = 7       | A|  - |
| t = 8       | A|  - |
| t = 9       | A|  - |
| t = 10      | B|  - |
| t = 11      | B|  - |
| t = 12      | B|  - |
| t = 13      | B|  - |  
| t = 13      | B|  - |  

The process B was alotted 4 ticks but gave up the CPU after running for 2 ticks, and then slept for 1 tick. When it is scheduled again , it runs for 4 regular ticks plus 1 *compensatory* tick for a total of 5 ticks. Bear in mind that even though B ran for 2 ticks lesser than expected, it is only given 1 compensatory tick as it only spent 1 tick sleeping.

## How xv6 starts (Recap)

## How xv6 starts

All the C program starts with `main`, including an operating system:

In `main.c`:

```C
// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int
main(void)
{
  kinit1(end, P2V(4*1024*1024)); // phys page allocator
  kvmalloc();      // kernel page table
  mpinit();        // detect other processors
  lapicinit();     // interrupt controller
  seginit();       // segment descriptors
  picinit();       // disable pic
  ioapicinit();    // another interrupt controller
  consoleinit();   // console hardware
  uartinit();      // serial port
  pinit();         // process table
  tvinit();        // trap vectors
  binit();         // buffer cache
  fileinit();      // file table
  ideinit();       // disk 
  startothers();   // start other processors
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
  userinit();      // first user process
  mpmain();        // finish this processor's setup
}
```

It basically does some initialization work, including setting up data structures for page table, trap, and files, detecting other CPUs, creating the first process (init), etc. 

But what happens before `main()` gets called? Checkout `bootasm.S` and `bootmain.c` if interested.

Eventually, `main()` calls `mpmain()`, which then calls `scheduler()`:

```C
// Common CPU setup code.
static void
mpmain(void)
{
  cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
  idtinit();       // load idt register
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
  scheduler();     // start running processes
}
```

The `scheduler()` is a forever-loop that keeps picking the next process to run (defined in `proc.c`):

```C
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}
```

<!-- `Note`: Your P3 will mainly revolve around scheduler code.  -->
## Scheduler Logic in xv6

The most interesting piece is what inside the while-loop:

```C
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
```

What it does is scanning through the ptable and find a `RUNNABLE` process `p`, then

1. set current CPU's running process to `p`: `c->proc = p`
2. switch userspace page table to `p`: `switchuvm(p)`
3. set `p` to `RUNNING`: `p->state = RUNNING`
4. `swtch(&(c->scheduler), p->context)`: This is the most tricky and magic piece. What it does is save the current registers into `c->scheduler`, and load the saved registers from `p->context`. After executing this line, the CPU is actually starting to execute `p`'s code (strictly speaking, it's in `p`'s kernel space, not jumping back to `p`'s userspace yet), and the next line `switchkvm()` will not be executed until later this process traps back to kernel again.

Both `c->scheduler` and `p->context` are of type `struct context`, while `c` is of type `struct cpu`.

What is the "context"?  
Let's consider a single CPU core. On that core, we have the notion of the current **execution context**, which basically means the current values of the following registers:

```C
// proc.h
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;     // backup of stack pointer
  uint eip;     // instruction pointer
};
```

Each CPU core has its current context. The context defines where in code this CPU core is currently running.

To jump between different processes, we "switch" the context - this is called a process **context switch**.
- We save the current running process P1's context somewhere - **To where?**  Into P1's PCB.
- We set the CPU context to be the context we previously saved for the to-be-scheduled process P2 - **From where?**  From P2's PCB.

The `cpu` struct details are below:

```C
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

// Per-process state
struct proc {
  // ...
  int pid;                     // Process ID
  //...
  struct context *context;     // swtch() here to run process
  // ...
};
```

**What is the first user process that gets scheduled on the CPU?**  The `init` process. The `init` then forks a child `sh`, which runs the xv6 shell program. The `init` then waits on `sh`, and the `sh` process will at some timepoint be scheduled -- this is when you see that active xv6 shell prompting you for some input!

[QUIZ] Before we finishing this section, consider this question: what is the current xv6's scheduling policy?

## `sched()`: From User Process to Scheduler

As you see in the scheduler, when a scheduler decision is made, `swtch(&(c->scheduler), p->context)` will switch to that user process.  
**But when will a user process swtch back to the scheduler?**  Basically, whenever the process calls `sched()`. 

```C
// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}
```

Skip some uninteresting sanity checking, the major work is really just one single line: `swtch(&p->context, mycpu()->scheduler)`.

There are three cases where a process could come into kernel mode and call `sched()`:

- The process *exits*
- The process goes to *block* voluntarily, examples:
  - It calls the sleep syscall
  - It calls the wait syscall
  - It tries to read on a pipe
- The process "*yield*"s - typically at a timer interrupt
  - At every ~10ms, the timer issues a hardware interrupt
  - This forces the CPU to trap into the kernel, see `trap()` in `trap.c`, the `IRQ_TIMER` case
  - xv6 then increments a global counter named `ticks`, then `yield()` the current running process

```C
// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  // ... (some cleanup)
  sched();
  panic("zombie exit");
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  //... (prepare to sleep)
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();
  //... (after wakeup)
}
```

It's unsurprising to see `sched()` gets called in these three cases... `exit()` is expected. `sleep()` here is a bad name... It actually means "blocked" or "wait". Any process that is waiting for some events (e.g. IO) will call this. When handling compensation ticks, we will play more with `sleep()` (in the next section). `yield()` here is another bad name... xv6 doesn't have a syscall named `yield()`. You should search in the codebase again to see who calls `yield()`.

## Timer Interrupt

Scheduling will be less useful without considering timer interrupts. As you have seen in P2, all the interrupts are handled in `trap.c: trap()`.

```C
struct spinlock tickslock;
uint ticks;

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
  // something you have already seen in P1B
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  // other cases...
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
```

There are two interesting things going on here:

1. If it is a timer interrupt `case T_IRQ0 + IRQ_TIMER`: the global variable `ticks` gets incremented
2. If it is a timer interrupt satisfying `myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER`: call `yield()`, which then relinquish CPU and gets into scheduler

## How to approach P2-A?
- Implement easy syscalls first
- - For e.g. Implement `setslice`, `getslice` and `fork2` first. 
- Test each part independently. 
- - Create custom test user applications. For e.g. when you implement the first 3 syscalls, create a C file, spawn a child process with `fork2`, set it's slice and try to fetch the same with `getslice` syscall. 
- - Debug any issue with GDB (Can't stress this enough!). 
- Start Early! (Biggest Hint!)

## `sleep()`: is not really just sleeping

There are typically three cases of *voluntary blocking* of a user process in xv6:

* It calls the `sleep(num_ticks)` syscall, which will be served by the in-kernel function `sys_sleep()`
* It calls the `wait()` syscall, trying to wait for a child process to finish
* It tries to do `read()` on a blocking pipe - a mechanism for doing inter-process communications

A slightly confusing naming here: all these three situations will eventually call an internal helper function, `proc.c: sleep(chan)`, that performs the blocking. Though named `sleep`, this internal helper function is used not only by `sys_sleep()`, but also by all other mechanisms of blocking. So `proc.c:sleep(chan)` function in xv6 is a terrible name... What it does is more like "waiting" or "blocking" instead of sleeping.

```C
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}
```

What does this do? `sleep()` works just like `pthread_cond_wait`: it requires the function caller to first acquire a lock (first argument `lk`) that guards a shared data structure `chan` (the second argument); then `sleep()` puts the current process into `SLEEPING` state and *then* release the lock. 

What does this mean (what is the semantics of this function)? "Channel" here is a waiting mechanism which could be any address. When process A updates a data structure and expects that some other processes could be waiting on a change of this data structure, A can scan and check other SLEEPING processes' `chan; `if process B's `chan` holds the address of the data structure process A just updated, then A would wake B up.

It might be helpful to read `sleep()` with its inverse function `wakeup()`:

```C
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}
```

Now let's take a closer look at `sleep()`:

```C
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  // [skip some checking]...

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // [Mark as sleeping and then call sched()]
  // [At some point this process gets waken up and resume]

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}
```

You may notice it has a conditional checking to see if `lk` refers to `ptable.lock`. Why? Pause here for a second to think about it. Hint: we are going to call `sched()` in the middle.

[Spoiler Alert!]

The reason is, `sched()` requires `ptable.lock` gets held. So here we are actually dealing with two locks: `lk` held by caller and `ptable.lock` that `sleep()` should hold. In a boundary case, these two locks are the same, which acquire special handling and that is why see the condition checking above.

```C
// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
    // ...
}
```

Remember this condition checking... It will save you from a lot of kernel panic we promise...

## `sys_sleep()`: A Use Case of `sleep()`

After understanding `sleep()`, we are now able to understand how `sys_sleep()` works.

```C
// In sysproc.c
int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}
```

Here the global variable `ticks` is used as the wait channel (recall global variable `ticks` is defined in `trap.c`), and the corresponding guard lock is `ticklock`.

```C
// In trap.c
struct spinlock tickslock;
uint ticks;

void
trap(struct trapframe *tf)
{
  // ... 
  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  // ...
  }
  // ...
}
```

So what really happens is: a process that calls **`sleep()` syscall** will be wakened up every time that the global variable `ticks` changes. It then replies on the while loop in `sys_sleep()` to ensure it has slept long enough.

```C
  while(ticks - ticks0 < n){
    // ...
    sleep(&ticks, &tickslock);
  }
```

This is not really a smart implementation of `sleep` syscall, because this process will jump between `RUNNABLE`, `RUNNING`, and `SLEEPING` back and forth until it sleeps long enough, which is inefficient and will mess up our compensation mechanism.

To make it more efficient, you need to make `wakeup1()` smarter and treat a process that waits on `ticks` differently to avoid falsely wake up.

For example, one implementation could be:

```C
// In sysproc.c
int
sys_sleep(void)
{
  // ...
  acquire(&tickslock);
  ticks0 = ticks;

  // Before sleep, mark some fields of this process fields to denote when this process should wake up
  MARK_SOME_FIELDS_OF_PROC();
  sleep(&ticks, &tickslock);
 
  release(&tickslock);
}

// In proc.c
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan) {
      // Add more checking to see are we really going to wake this process up
      if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
    }
}
```

## One More (Important) Hint

Whenever you need to access the global variable `ticks`, think twice whether the `tickslock` is already being held or not. A process who tries to acquire the lock that it has already acquired will cause a kernel panic.

For example, suppose you want to access `ticks` in `sleep()`. Instead of

```C
sleep(void *chan, struct spinlock *lk)
{
  // ...
  uint now_ticks;
  acquire(&tickslock);
  now_ticks = ticks;
  release(&tickslock);
  // ...
}
```

Make sure you don't double-acquire `tickslock`:

```C
sleep(void *chan, struct spinlock *lk)
{
  // ...
  if (lk == &tickslock) { // already hold
    now_ticks = ticks;
  } else {
    acquire(&tickslock);
    now_ticks = ticks;
    release(&tickslock);
  }
  // ...
    
  // same for release...
}
```

Keep this in mind. It will save you from a huge amount of kernel panic...
