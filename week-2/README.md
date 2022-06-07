# COMP SCI 537 Discussion Week 2

# DISCLAIMER: Any code samples provided in the discussion are simply for illustration and do not meet project specifications or linting requirements. You may reuse this code freely but you are responsible to make the modifications required for it to meet the specs.

## Parsing arguments

In P1, you were asked to use some functions you may have been unfamiliar with such as `fgets` and `strncasecmp` in addition to new useful functons you discovered from the internet. 

The most useful resoruce to learn more from a new function is the man pages. As an example, lets consider the `getopt` function for parsing arguments. 

We can run `man getopt` to learn more about this function, and see the below output

```console
GETOPT(1)                                         User Commands                                         GETOPT(1)

NAME
       getopt - parse command options (enhanced)

SYNOPSIS
       getopt optstring parameters
       getopt [options] [--] optstring parameters
       getopt [options] -o|--options optstring [options] [--] parameters
...
```

As you may have noticed, the above function returns information about the `getopt` command and not the inbuilt function. We need to consult Section 3 (C Library functions) of the manual pages instead, by running `man 3 getopt`. To learn more about each section, check the manual of the man pages itself by running `man man`

The following output is returned on viewing the man page for the `getopt` C library function:

```console
GETOPT(3)                                   Linux Programmer's Manual                                   GETOPT(3)

NAME
       getopt, getopt_long, getopt_long_only, optarg, optind, opterr, optopt - Parse command-line options

SYNOPSIS
       #include <unistd.h>

       int getopt(int argc, char * const argv[],
                  const char *optstring);

       extern char *optarg;
       extern int optind, opterr, optopt;

       #include <getopt.h>

       int getopt_long(int argc, char * const argv[],
                  const char *optstring,
                  const struct option *longopts, int *longindex);

       int getopt_long_only(int argc, char * const argv[],
                  const char *optstring,
                  const struct option *longopts, int *longindex);

   Feature Test Macro Requirements for glibc (see feature_test_macros(7)):

       getopt(): _POSIX_C_SOURCE >= 2 || _XOPEN_SOURCE
       getopt_long(), getopt_long_only(): _GNU_SOURCE

...
```
We can that the function returns an integer value and also sets the value of global variables `optind`, `opterr`, `optopt` and `optagr`. Reading the description (omitted in the sample output above) tells us that the return value of the function is the character entered as an option, while `optind` returns the argument index and `optarg` returns the value of the argument when provided. Each option needs to be included in the `optstring` variable, with a trailing `:` for any flag that accepts as an argument. Using this, we can write a function to parse options as below:


```c
void parse_args(int argc, char* argv[]){
        int opt;
        while ((opt = getopt(argc, argv, "Vhf:")) != -1){
                switch(opt){
                        case 'V':
                                printf("V argument provided!\n");
                                exit(0);
                        case 'h':
                                printf("h argument provided!\n");
                                exit(0);
                        case 'f':
                                printf("f argument provided with value %s\n", optarg);
                                break;
                        default:
                                printf("Invalid arguments!\n");
                                exit(1);
                }
        }
}
```

As seen in the man page, the `optind` option specifies the index of the option currently being parsed. If we need to parse standard arguments after the options, we can use the `optind` variable to parse these arguments as below:

```c
standard_arg = argv[optind];
```

The code provided in the repo provides an example based on project P1 (but does not meet the exact spec or lint requirements).

## Intro to Xv6

To get the source code, clone the git repo `https://github.com/mit-pdos/xv6-public`:

```bash
$ git clone https://github.com/mit-pdos/xv6-public
$ cd xv6-public/
```

To compile the xv6:

```bash
$ make
```

Recall an operating system is special software that manages all the hardware, so to run an OS, you need hardware resources like CPUs, memory, storage, etc. To do this, you could either have a real physical machine, or a virtualization software that "fakes" these hardwares. QEMU is an emulator that virtualizes some hardware to run xv6.

To compile the xv6 and run it in QEMU:

```bash
$ make qemu-nox
```

`-nox` here means no graphical interface, so it won't pop another window for xv6 console but directly show everything in your current terminal. The xv6 doesn't have a `shutdown` command; to quit from xv6, you need to kill the QEMU: press `ctrl-a` then `x`.

After compiling and running xv6 in QEMU, you could play walk around inside xv6 by `ls`.

```bash
$ make qemu-nox

iPXE (http://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1FF8CA10+1FECCA10 CA00
                                                                            


Booting from Hard Disk..xv6...
cpu1: starting 1
cpu0: starting 0
sb: size 1000 nblocks 941 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap8
init: starting sh
$ ls
.              1 1 512
..             1 1 512
README         2 2 2286
cat            2 3 16272
echo           2 4 15128
forktest       2 5 9432
grep           2 6 18492
init           2 7 15712
kill           2 8 15156
ln             2 9 15012
ls             2 10 17640
mkdir          2 11 15256
rm             2 12 15232
sh             2 13 27868
stressfs       2 14 16148
usertests      2 15 67252
wc             2 16 17008
zombie         2 17 14824
console        3 18 0
stressfs0      2 19 10240
stressfs1      2 20 10240
$ echo hello world
hello world
```

## Xv6 Syscalls
We will explore main files required to do p1A. 

- `Makefile`:
    -  `CPUS`: # CPUS for QEMU to virtualize (make it 1)
    -  `UPROGS`: what user-space program to build with the kernel. Note that xv6 is a very simple kernel that doesn't have a compiler, so every executable binary replies on the host Linux machine to build it along with the kernel.
    - Exercise: add a new user-space application called `hello` which prints `"hello world\n"` to the stdout.

- `usys.S`: where syscalls interfaces are defined in the assembly. This is the entry point to syscalls. 
    - Code for `usys.S` 
        ```assembly
        #include "syscall.h"
        #include "traps.h"

        #define SYSCALL(name) \
        .globl name; \
        name: \
            movl $SYS_ ## name, %eax; \
            int $T_SYSCALL; \
            ret

        SYSCALL(fork)
        SYSCALL(exit)
        SYSCALL(wait)
        # ...
        ```
    - It first declares a macro `SYSCALL`, which takes an argument `name` and expands it.

        ```assembly
        #define SYSCALL(name) \
        .globl name; \
        name: \
            movl $SYS_ ## name, %eax; \
            int $T_SYSCALL; \
            ret
        ```    
    - For e.g. let `name` be `getpid`, the macro above will be expanded into

        ```assembly
        .globl getpid
        getpid:
            movl $SYS_getpid, %eax
            int $T_SYSCALL
            ret
        ```
    - Where is `SYS_getpid` and `T_SYSCALL` located?
    - so it will be further expanded into
        ```assembly
        .globl getpid
        getpid:
            movl $11, %eax
            int $64
            ret
        ```
    - Run `gcc -S usys.S` to see all syscalls. 
        ```console
        $ gcc -S usys.S
        // ...
        .globl dup; dup: movl $10, %eax; int $64; ret
        .globl getpid; getpid: movl $11, %eax; int $64; ret
        .globl sbrk; sbrk: movl $12, %eax; int $64; ret
        // ...
        ```
    - `.globl getpid` declares a label `getpid` for compiler/linker; when someone in the user-space executes `call getpid`, the linker knows which line it should jump to. It then moves 11 into register `%eax` and issues an interrupt with an operand 64.
- `trapasm.S`: The user-space just issues an interrupt with an operand `64`. The CPU then starts to run the kernel's interrupt handler.
    - It builds what we call a trap frame, which basically saves some registers into the stack and makes them into a data structure `trapframe`. 

    - `trapframe` is defined in `x86.h`:

    ```C
    struct trapframe {
    // registers as pushed by pusha
    uint edi;
    uint esi;
    uint ebp;
    uint oesp;      // useless & ignored
    uint ebx;
    uint edx;
    uint ecx;
    uint eax;

    // rest of trap frame
    ushort gs;
    ushort padding1;
    ushort fs;
    ushort padding2;
    ushort es;
    ushort padding3;
    ushort ds;
    ushort padding4;
    uint trapno;

    // below here defined by x86 hardware
    uint err;
    uint eip;
    ushort cs;
    ushort padding5;
    uint eflags;

    // below here only when crossing rings, such as from user to kernel
    uint esp;
    ushort ss;
    ushort padding6;
    };
    ```
    - `alltraps` eventually executes `call trap`. The function `trap()` is defined in `trap.c`:


- `trap.c`: recall a syscall is implemented as an interrupt (sometimes also called "trap")
    - Trap code:
        ```C
        void
        trap(struct trapframe *tf)
        {
        if(tf->trapno == T_SYSCALL){
            if(myproc()->killed)
            exit();
            myproc()->tf = tf;
            syscall();
            if(myproc()->killed)
            exit();
            return;
        }
        // ...
        }
        ```
    - The 64 we just saw is `tf->trapno` here. `trap()` checks whether `trapno` is `T_SYSCALL`, if yes, it calls `syscall()` to handle it.

- `syscall.c` and `syscall.h`:

    - `void syscall(void)`: 
        ```C
        void
        syscall(void)
        {
        int num;
        struct proc *curproc = myproc();

        num = curproc->tf->eax;
        if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
            curproc->tf->eax = syscalls[num]();
        } else {
            cprintf("%d %s: unknown sys call %d\n",
                    curproc->pid, curproc->name, num);
            curproc->tf->eax = -1;
        }
        }
        ```
    - `Note:` See how to identify which syscall to call and where is the return value for completed syscalls. This is the heart of P2.


- `sysproc.c`:

    -  Kill syscall: Kill takes `PID` of the process to kill as an arguement, but this function has no arguement. How does this happen? 

        ```C
        int
        sys_kill(void)
        {
        int pid;

        if(argint(0, &pid) < 0)
            return -1;
        return kill(pid);
        }
        ```
        `Hint`: See `argint` is defined in `syscall.c`

- `proc.c` and `proc.h`:

    - `struct proc`: per-process state
    - `struct context`: saved registers
    - `ptable.proc`: a pool of `struct proc`
    - `static struct proc* allocproc(void)`: where a `struct proc` gets allocated from `ptable.proc` and **initialized**

- `user.h` and `defs.h`: what the difference compared to `user.h`? 

    `Hint:` [link](https://www.cs.virginia.edu/~cr4bd/4414/S2020/xv6.html)

