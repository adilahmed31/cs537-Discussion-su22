# COMP SCI 537 Discussion Week 3

## DISCLAIMER: Any code samples provided in the discussion are simply for illustration and do not meet project specifications or linting requirements. You may reuse this code freely but you are responsible to make the modifications required for it to meet the specs.
## xv6 and GDB
To run xv6 with gdb: in one window

```bash
$ make qemu-nox-gdb
```

then in another window:

```bash
$ gdb kernel
```
You might get the error message below:

```bash
$ gdb
GNU gdb (Ubuntu 9.2-0ubuntu1~20.04) 9.2
Copyright (C) 2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word".
warning: File "/dir/xv6/.gdbinit" auto-loading has been declined by your `auto-load safe-path' set to "$debugdir:$datadir/auto-load".
To enable execution of this file add
        add-auto-load-safe-path /dir/xv6/.gdbinit
line to your configuration file "/u/c/h/chenhaoy/.gdbinit".
To completely disable this security protection add
        set auto-load safe-path /
line to your configuration file "/u/c/h/chenhaoy/.gdbinit".
For more information about this security protection see the
--Type <RET> for more, q to quit, c to continue without paging--
```

Recent gdb versions will not automatically load `.gdbinit` for security purposes. You could either:

- `echo "add-auto-load-safe-path $(pwd)/.gdbinit" >> ~/.gdbinit`. This enables the autoloading of `.gdbinit` in the current working directory.
- `echo "set auto-load safe-path /" >> ~/.gdbinit`. This enables the autoloading of every `.gdbinit`

After either operation, you should be able to launch gdb. Specify you want to attach to the kernel

```bash
$ gdb kernel
GNU gdb (Ubuntu 9.2-0ubuntu1~20.04) 9.2
Copyright (C) 2020 Free Software Foundation, Inc.
...
Type "apropos word" to search for commands related to "word"...
Reading symbols from kernel...
+ target remote localhost:29475
The target architecture is assumed to be i8086
[f000:fff0]    0xffff0:	ljmp   $0x3630,$0xf000e05b
0x0000fff0 in ?? ()
+ symbol-file kernel
```

 Once GDB has connected successfully to QEMU's remote debugging stub, it retrieves and displays information about where the remote program has stopped: 

 ```bash
 The target architecture is assumed to be i8086
[f000:fff0]    0xffff0:	ljmp   $0x3630,$0xf000e05b
0x0000fff0 in ?? ()
+ symbol-file kernel
 ```
 QEMU's remote debugging stub stops the virtual machine before it executes the first instruction: i.e., at the very first instruction a real x86 PC would start executing after a power on or reset, even before any BIOS code has started executing.

 Type the following in GDB's window:

 ```bash
 (gdb) b exec
Breakpoint 1 at 0x80100a80: file exec.c, line 12.
(gdb) c
Continuing.
 ```
These commands set a breakpoint at the entrypoint to the exec function in the xv6 kernel, and then continue the virtual machine's execution until it hits that breakpoint. You should now see QEMU's BIOS go through its startup process, after which GDB will stop again with output like this:

```bash
The target architecture is assumed to be i386
0x100800 :	push   %ebp

Breakpoint 1, exec (path=0x20b01c "/init", argv=0x20cf14) at exec.c:11
11	{
(gdb) 
```

At this point, the machine is running in 32-bit mode, the xv6 kernel has initialized itself, and it is just about to load and execute its first user-mode process, the /init program. You will learn more about exec and the init program later; for now, just continue execution: 

```bash
(gdb) c
Continuing.
0x100800 :	push   %ebp

Breakpoint 1, exec (path=0x2056c8 "sh", argv=0x207f14) at exec.c:11
11	{
(gdb) 
```
The second time the exec function gets called is when the /init program launches the first interactive shell, sh.

 Now if you continue again, you should see GDB appear to "hang": this is because xv6 is waiting for a command (you should see a '$' prompt in the virtual machine's display), and it won't hit the exec function again until you enter a command and the shell tries to run it. Do so by typing something like:

```bash
$ cat README
```
You should now see in the GDB window:

```bash
0x100800 :	push   %ebp

Breakpoint 1, exec (path=0x1f40e0 "cat", argv=0x201f14) at exec.c:11
11	{
(gdb) 
```

GDB has now trapped the exec system call the shell invoked to execute the requested command.

Now let's inspect the state of the kernel a bit at the point of this exec command. 

```bash
(gdb) p argv[0]
(gdb) p argv[1]
(gdb) p argv[2]
```

Let's continue to end the cat command.
```bash
(gdb) c
Continuing.
```


## Syscalls Recap 
We will explore the main files required to do p1b. 

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
    - `Note:` See how to identify which syscall to call and where is the return value for completed syscalls. This is the heart of P1-B.


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


## Exercise

Create a hello-world syscall and call it from a new user application. 
Hint: Check `kill`

## Shell Overview:
Shell is a program that takes input as "commands" line by line, parses them, then
- For most of the commands, the shell will create a new process and load it with an executable file to run. E.g. `gcc`, `ls`
- For some commands, the shell will react to them directly without creating a new process. These are called "built-in" commands. E.g. `alias`

If you are curious about which catalog a command falls into, try `which cmd_name`:

```console
# The output of the following commands may vary depending on what shell you are running
# Below is from zsh. If you are using bash, it may look different
$ which gcc
/usr/bin/gcc
$ which alias
alias: shell built-in command
$ which which
which: shell built-in command
```
### `fork`:

The syscall `fork()` creates a copy of the current process. We call the original process "parent" and the newly created process "child". But how are we going to tell which is which if they are the same??? The child process will get the return value 0 from `fork` while the parent will get the child's pid as the return value.

```C
pid_t pid = fork();
if (pid == 0) { // the child process will execute this
    printf("I am child with pid %d. I got return value from fork: %d\n", getpid(), pid);
    exit(0); // we exit here so the child process will not keep running
} else { // the parent process will execute this
    printf("I am parent with pid %d. I got return value from fork: %d\n", getpid(), pid);
}
```

You could find the code above in the repo as `fork_example.c`. After executing it, you will see output like this:

```console
$ ./fork_example
I am parent with pid 46565. I got return value from fork: 46566
I am child with pid 46566. I got return value from fork: 0
```
### `exec`: 

`fork` itself is not sufficient to run an operating system. It can only create a copy of the previous program, but we don't want the exact same program all the time. That's when `exec` shines. `exec` is actually a family of functions, including `execve`, `execl`, `execle`, `execlp`, `execv`, `execvp`, `execvP`... The key one is `execve` (which is the actual syscall) and the rest of them are just some wrappers in the glibc that does some work and then calls `execve`. For this project, `execv` is probably what you need. It is slightly less powerful than `execve`, but is enough for this project.

What `exec` does is: it accepts a path/filename and finds this file. If it is an executable file, it then destroys the current address space (including code, stack, heap, etc), loads in the new code, and starts executing the new code.

```C
int execv(const char *path, char *const argv[])
```

Here is how `execv` works: it takes a first argument `path` to specify where the executable file locates, and then provides the command-line arguments `argv`, which will eventually become what the target program receives in their main function `int main(int argc, char* argv[])`.

```C
pid_t pid = fork();
if (pid == 0) {
    // the child process will execute this
    char *const argv[3] = {
        "/bin/ls", // string literial is of type "const char*"
        "-l",
        NULL // it must have a NULL in the end of argv
    };
    int ret = execv(argv[0], argv);
    // if succeeds, execve should never return
    // if it returns, it must fail (e.g. cannot find the executable file)
    printf("Fails to execute %s\n", argv[0]);
    exit(1); 
}
// do parent's work
```

You could find the code above in the repo as `exec_example.c`. After executing it, you will see output exactly like executing `ls -l`.

As the last word of this section, I strongly recommend you to read the [document](https://linux.die.net/man/3/execv) yourself to understand how it works.

### `waitpid`: Wait for child to finish

Now you know the shell is the parent of all the processes it creates. The next question is, when executing a command, the shell should suspend and not asking for new inputs until the child process finishes executing.

```console
$ sleep 10 # this will create a new process executing /usr/bin/sleep
```

The command above will create a child process that does nothing other than sleeping for 10 seconds. During this period, you may notice your shell is also not printing a new prompt. This is what the shell does: it waits until the child process to terminate (no matter voluntarily or not). If you use `ctrl-c` to terminate `sleep`, you should see the shell would print the prompt immediately.

This is also be done by a syscall `waitpid`:

```C
pid_t waitpid(pid_t pid, int *status, int options);
```

This syscall will suspend the parent process and resume again when the child is done. It takes three arguments: `pid`, a pointer to an int `status`, and some flags `options`. `pid` is the pid of the child that the parent wants to waiit. `status` is a pointer pointing a piece of memory that `waitpid` could write the status to. `options` allows you to configure when `waitpid` should return. By default (`options = 0`), it only returns when the child terminates. This should be sufficient for this project.

Side note: NEVER write code like this:

```C
// assume we know pid
int* status;
waitpid(pid, status, 0); // QUIZ: what is wrong?
```

`*status` will be filled in the status that `waitpid` returns. This allows you to have more info about what happens to the child. You could use some macro defined in the library to check it, e.g. `WIFEXITED(status)` is true if the child terminated normally (i.e. not killed by signals); `WEXITSTATUS(status)` could tell you what is the exit code from the child process (e.g. if the child `return 0` from the main function, `WEXITSTATUS(status)` will give you zero).

Again, you should read the [document](https://linux.die.net/man/2/waitpid) yourself carefully.

Putting what we have discussed together, you should now have some idea of the skeleton of the shell

```C
pid_t pid = fork(); // create a new child process
if (pid == 0) { // this is child
    // do some preparation
    execv(path, argv);
    exit(1); // this means execv() fails
}
// parent
int status;
waitpid(pid, &status, 0); // wait the child to finish its work before keep going
// continue to handle the next command
```

### File Descriptor for redirection

If you have ever printed out a file descriptor, you may notice it is just an integer

```C
int fd = open("filename", O_RDONLY);
printf("fd: %d\n", fd);
// you may get output "3\n"
```

<!-- However, the file-related operation is stateful: e.g. it must keep a record what is the current read/write offset, etc. How does an integer encoding this information?

It turns out there is a level of indirection here: the kernel maintains such states and the file descriptor it returns is essentially a "handler" for later index back to these states. The kernel provides the guarantee that "the user provides the file descriptor and the operations to perform, the kernel will look up the file states that corresponding to the file descriptor and operate on it". -->

It would be helpful to understand what happens with some actual code from a kernel. For simplicity, we use ths xv6 code below to illustrate how file descriptor works, but remember, your p2b is on Linux. Linux has a very similar implementation.

For every process state (`struct proc` in xv6), it has an array of `struct file` (see the field  `proc.ofile`) to keep a record of the files this process has opened.

```C
// In proc.h
struct proc {
  // ...
  int pid;
  // ...
  struct file *ofile[NOFILE];  // Open files
};

// In file.h
struct file {
  // ...
  char readable;    // these two variables are actually boolean, but C doesn't have `bool` type,
  char writable;    // so the authors use `char`
  // ...
  struct inode *ip; // this is a pointer to another data structure called `inode`
  uint off;         // offset
};
```

The file descriptor is essentially an index for `proc.ofile`. In the example above, when opening a file named "filename", the kernel allocates a `struct file` for all the related state of this file. Then it stores the address of this `struct file` into `proc.ofile[3]`. In the future, when providing the file descript `3`, the kernel could get `struct file` by using `3` to index `proc.ofile`. This also gives you a reason why you should `close()` a file after done with it: the kernel will not free `struct file` until you `close()` it; also `proc.ofile` is a fixed-size array, so it has limit on how many files a process can open at max (`NOFILE`).

In addition, file descriptors `0`, `1`, `2` are reserved for stdin, stdout, and stderr.

### File Descriptors after `fork()`

During `fork()`, the new child process will copy `proc.ofile` (i.e. copying the pointers to `struct file`), but not `struct file` themselves. In other words, after `fork()`, both parent and child will share `struct file`. If the parent changes the offset, the change will also be visible to the child.

```
struct proc: parent {
    +---+
    | 0 | ------------+---------> [struct file: stdin]
    +---+             |
    | 1 | --------------+-------> [struct file: stdout]
    +---+             | |
    | 2 | ----------------+-----> [struct file: stderr]
    +---+             | | |
    ...               | | |
}                     | | |
                      | | |
struct proc: child {  | | |
    +---+             | | |
    | 0 | ------------+ | |
    +---+               | |
    | 1 | --------------+ |
    +---+                 |
    | 2 | ----------------+
    +---+
    ...
}
```
`High-level Ideas of Redirection: What to Do`

When a process writes to stdout, what it actually does it is writing data to the file that is associated with the file descriptor `1`. So the trick of redirection is, we could replace the `struct file` pointer at `proc.ofile[1]` with another `struct file`.

For example, when handling the shell command `ls > log.txt`, what the file descriptors eventually should look like:

```
struct proc: parent {                                      <= `mysh` process
    +---+
    | 0 | ------------+---------> [struct file: stdin]
    +---+             |
    | 1 | ----------------------> [struct file: stdout]
    +---+             | 
    | 2 | ----------------+-----> [struct file: stderr]
    +---+             |   |
    ...               |   |
}                     |   |
                      |   |
struct proc: child {  |   |                                <= `ls` process
    +---+             |   |
    | 0 | ------------+   |
    +---+                 |
    | 1 | ----------------|-----> [struct file: log.txt]   <= this is a new stdout!
    +---+                 |
    | 2 | ----------------+
    +---+
    ...
}
```

### `dup2()`: How to Do

The trick to implement the redirection is the syscall `dup2`. This syscall performs the task of "duplicating a file descriptor".

```C
int dup2(int oldfd, int newfd);
```

`dup2` takes two file descriptors as the arguments. It performs these tasks (with some pseudo-code):

1. if the file descriptor `newfd` has some associated files, close it. (`close(newfd)`)
2. copy the file associated with `oldfd` to `newfd`. (`proc.ofile[newfd] = proc.ofile[oldfd]`)

Consider the provious example with `dup2`:

```C
int pid = fork();
if (pid == 0) { // child;
    int fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644); // [1]
    dup2(fd, fileno(stdout));                                     // [2]
    // execv(...)
}
```

Here `fileno(stdout)` will give the file descriptor associated with the current stdout. After executing `[1]`, you should have:

```
struct proc: parent {                                     <= `mysh` process
    +---+
    | 0 | ------------+---------> [struct file: stdin]
    +---+             |
    | 1 | --------------+-------> [struct file: stdout]
    +---+             | |
    | 2 | ----------------+-----> [struct file: stderr]
    +---+             | | |
    ...               | | |
}                     | | |
                      | | |
struct proc: child {  | | |                               <= child process (before execv "ls")
    +---+             | | |
    | 0 | ------------+ | |
    +---+               | |
    | 1 | --------------+ |
    +---+                 |
    | 2 | ----------------+
    +---+
    | 3 | ----------------------> [struct file: log.txt]  <= open a file "log.txt"
    +---+
    ...
}
```

After executing `[2]`, you should have

```
struct proc: parent {                                     <= `mysh` process
    +---+
    | 0 | ------------+---------> [struct file: stdin]
    +---+             |
    | 1 | ----------------------> [struct file: stdout]
    +---+             | 
    | 2 | ----------------+-----> [struct file: stderr]
    +---+             |   |
    ...               |   |
}                     |   |
                      |   |
struct proc: child {  |   |                               <= child process (before execv "ls")
    +---+             |   |
    | 0 | ------------+   |
    +---+                 |
    | 1 | --------------+ |
    +---+               | |
    | 2 | ----------------+
    +---+               |
    | 3 | --------------+-------> [struct file: log.txt]
    +---+
    ...
}
```

Also, compared to the figure of what we want, it has a redundant file descriptor `3`. We should close it. The modified code should look like this:

```C
int pid = fork();
if (pid == 0) { // child;
    int fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644); // [1]
    dup2(fd, fileno(stdout));                                     // [2]
    close(fd);                                                    // [3]
    // execv(...)
}
```

After we finishing `dup2`, the previous `fd` is no longer useful. We close it at `[3]`. Then the file descriptors should look like this:

```
struct proc: parent {                                     <= `mysh` process
    +---+
    | 0 | ------------+---------> [struct file: stdin]
    +---+             |
    | 1 | ----------------------> [struct file: stdout]
    +---+             | 
    | 2 | ----------------+-----> [struct file: stderr]
    +---+             |   |
    ...               |   |
}                     |   |
                      |   |
struct proc: child {  |   |                               <= child process (before execv "ls")
    +---+             |   |
    | 0 | ------------+   |
    +---+                 |
    | 1 | --------------+ |
    +---+               | |
    | 2 | ----------------+
    +---+               |
    | X |               +-------> [struct file: log.txt]
    +---+
    ...
}
```

Despite the terrible drawing, we now have what we want! You can imagine doing it in a similar way for stdin redirection.

Again, you should read the [document](https://man7.org/linux/man-pages/man2/dup2.2.html) yourself to understand how to use it.

`Note`, piping is actually also implemented in a similar way.
