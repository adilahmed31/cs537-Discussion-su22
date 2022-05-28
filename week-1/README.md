# COMP SCI 537 Discussion Week 1

## CSL AFS Permission

CSL Linux machines use AFS (Andrew File System). It is a distributed filesystem (that's why you see the same content when logging into different CSL machines) with additional access control.

To see the permission of a directory

```shell
fs la /path/to/dir
```

You could also omit `/path/to/dir` to show the permission in the current working directory. You might get output like this:

```console
$ fs la /p/course/cs537-yuvraj/turnin
Access list for /p/course/cs537-yuvraj/turnin is
Normal rights:
  system:administrators rlidwka
  system:anyuser l
  asinha32 rlidwka
  yuvraj rlidwka
```

- `r` (read): allow to read the content of files
- `l` (lookup): allow to list what in a directory
- `i` (insert): allow to create files
- `d` (delete): allow to delete files
- `w` (write): allow to modify the content of a file or `chmod`
- `a` (administer): allow to change access control

Check [this](https://computing.cs.cmu.edu/help-support/afs-acls) out for more how to interpret this output.

You should test all your code in CSL.

## Compilation

For this course, we use `gcc` as the standard compiler.

```shell
gcc -o my-code my-code.c -Wall -Werror
# be careful! DO NOT type "gcc -o my-code.c ..."
```

- `-Wall` flag means `gcc` will show most of the warnings it detects.
-  `-Werror` flag means `gcc` will treat the warnings as errors and reject compiling.

## Makefiles

The *make* utility lets us automate the build process for our C programs.

We define the rules and commands for building the code in a specialized `Makefile`.
In the Makefile, you specify *targets*, the *dependencies* of each target, and the commands to run to build each target.

Format:
```bash
target : dependencies
    commands
```

After writing your makefile, save it with with the name `Makefile` or `makefile`, and run make with the name of the 'target' you wish to build:  
```
make target
```

### Simple Example
```bash
blah: blah.o
	cc blah.o -o blah # Runs third

blah.o: blah.c
	cc -c blah.c -o blah.o # Runs second

blah.c:
	echo "int main() { return 0; }" > blah.c # Runs first
```

The following Makefile has three separate rules. When you run make `blah` in the terminal, it will build a program called blah in a series of steps:
- Make is given blah as the target, so it first searches for this target
- blah requires blah.o, so make searches for the blah.o target
- blah.o requires blah.c, so make searches for the blah.c target
- blah.c has no dependencies, so the echo command is run
- The cc -c command is then run, because all of the blah.o dependencies are finished
- The top cc command is run, because all the blah dependencies are finished
- That's it: blah is a compiled c program


### The all target

Making multiple targets and you want `all` of them to run? Make an all target.

```bash
all: one two three

one:
	touch one
two:
	touch two
three:
	touch three

clean:
	rm -f one two three
```

- More examples: https://makefiletutorial.com/


- For more information, see the GNU Make official manual: [https://www.gnu.org/software/make/manual/](https://www.gnu.org/software/make/manual/)

## Coding Style

In addition to testing for correct program behavior, we will also be giving points for good programming style and for careful memory management.

As programmers, we often won't be writing all our own code from scratch, and instead will be making contributions inside of an already existing project which other people are also contributing to. In these situations, other programmers will often need to be able to read and understand the code you've written. Because of this, many companies will require its programmers to adhere to a style guideline, so that the task of reading and understanding another person's code is made easier.

The linter we will use can be found here:

`/p/course/cs537-yuvraj/public/cpplint.py`

We will be running it with the following options:

`cpplint.py --root=/p/course/cs537-yuvraj/turnin/ --extensions=c,h my-diff.c`

## Sanitizers

Sanitizers are a set of tools to inject some checking instructions during the compilation. These instructions could provide warnings at the runtime.

### Memory Leak

To detect memory leak, use `-fsanitize=address`:

```console
$ gcc -fsanitize=address -o leak leak.c
$ ./leak

=================================================================
==36716==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 4 byte(s) in 1 object(s) allocated from:
    #0 0xffff9686ca30 in __interceptor_malloc (/lib/aarch64-linux-gnu/libasan.so.5+0xeda30)
    #1 0xaaaace937868 in leak (/dir/leak+0x868)
    #2 0xaaaace937884 in main (/dir/leak+0x884)
    #3 0xffff9663208c in __libc_start_main (/lib/aarch64-linux-gnu/libc.so.6+0x2408c)
    #4 0xaaaace937780  (/dir/leak+0x780)

SUMMARY: AddressSanitizer: 4 byte(s) leaked in 1 allocation(s).
```

Note that compilation with sanitizer flags could significantly slow down your code (typically 3x to 10x); do not include it in your submission Makefile.

### Access Illegal Memory

`-fsanitize=address` could actually be helpful in many memory-related bugs, for example, illegal memory access:

```console
$ gcc -fsanitize=address -o hello_world_bug hello_world_bug.c
$ ./hello_world_bug AAAAAAAAAA
=================================================================
==615079==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7fffe35411fa at pc 0x7f486465b16d bp 0x7fffe35411b0 sp 0x7fffe3540958
WRITE of size 13 at 0x7fffe35411fa thread T0
    #0 0x7f486465b16c in __interceptor_strcpy ../../../../src/libsanitizer/asan/asan_interceptors.cc:431
    #1 0x55907bf7d319 in main /u/o/b/obaidullahadil/private/adil/cs537-summer/Discussions/week-1/hello_world_bug.c:7
    #2 0x7f48643f1082 in __libc_start_main ../csu/libc-start.c:308
    #3 0x55907bf7d18d in _start (/afs/cs.wisc.edu/u/o/b/obaidullahadil/private/adil/cs537-summer/Discussions/week-1/hello_world_bug+0x118d)

Address 0x7fffe35411fa is located in stack of thread T0 at offset 42 in frame
    #0 0x55907bf7d258 in main /u/o/b/obaidullahadil/private/adil/cs537-summer/Discussions/week-1/hello_world_bug.c:5

  This frame has 1 object(s):
    [32, 42) 'name' (line 6) <== Memory access at offset 42 overflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
      (longjmp and C++ exceptions *are* supported)
SUMMARY: AddressSanitizer: stack-buffer-overflow ../../../../src/libsanitizer/asan/asan_interceptors.cc:431 in __interceptor_strcpy
Shadow bytes around the buggy address:
  0x10007c6a01e0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x10007c6a01f0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x10007c6a0200: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x10007c6a0210: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x10007c6a0220: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x10007c6a0230: 00 00 00 00 00 00 00 00 00 00 f1 f1 f1 f1 00[02]
  0x10007c6a0240: f3 f3 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x10007c6a0250: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x10007c6a0260: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x10007c6a0270: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x10007c6a0280: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07 
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
  Shadow gap:              cc
==615079==ABORTING
```

## Valgrind

Valgrind could also be helpful for a memory leak. Different from sanitizers, valgrind does not require special compilation command.

```console
$ gcc -o leak1 leak1.c
$ valgrind ./leak1
==614513== Memcheck, a memory error detector
==614513== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==614513== Using Valgrind-3.15.0 and LibVEX; rerun with -h for copyright info
==614513== Command: ./leak1
==614513== 
==614513== 
==614513== HEAP SUMMARY:
==614513==     in use at exit: 4 bytes in 1 blocks
==614513==   total heap usage: 1 allocs, 0 frees, 4 bytes allocated
==614513== 
==614513== LEAK SUMMARY:
==614513==    definitely lost: 4 bytes in 1 blocks
==614513==    indirectly lost: 0 bytes in 0 blocks
==614513==      possibly lost: 0 bytes in 0 blocks
==614513==    still reachable: 0 bytes in 0 blocks
==614513==         suppressed: 0 bytes in 0 blocks
==614513== Rerun with --leak-check=full to see details of leaked memory
==614513== 
==614513== For lists of detected and suppressed errors, rerun with: -s
==614513== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Other possible memory leaks:

- Orphaned memory locations: When a pointer is re-defined to point to a different memory location without freeing the first memory, that memory is now unreachable and cannot be freed.

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void leak() {
        char* old_val = malloc (10);
        char* new_val = malloc (10);
        old_val = new_val;
        free(new_val);
        free(old_val);
        return;
}

int main() {
        leak();
        return 0;
}
```

- Not handling a pointer returned from a function could cause a memory leak.
```
#include <stdio.h>
#include <stdlib.h>

char* leak() {
        printf("Leak function called!");
        char* p = malloc(20);
        return p;
}

int main() {
        leak();
        return 0;
}
```

A more complete list of possible memory safety issues can be found [here][https://developer.ibm.com/articles/au-toughgame/]

## Debugging with GDB

Sometimes, we want to do more than run the code and read output.
We'd like to interact with the code as it runs, so we can narrow down the source of bugs.
This is where debuggers become useful.
You may have used one in a GUI-based IDE before.

The gdb program is used for debugging work through the command line.
First, use the -g option when compiling:

```console
$ gcc -g -o hello_world_bug hello_world_bug.c
```

We can now interact with the program as it runs, via gdb:
```console
$ gdb ./hello_world_bug
```

Useful commands to run inside the gdb environment (single-letter abbreviations in brackets):
+ list ## [l]: list lines of code, beginning with line ##.
+ break ## [b]: set a breakpoint at line number ## e.g. break 10. Alternately, break *func*, where func is the name of a function.
+ info b [i]: list breakpoints (other options than b available).
+ disable ##: disable the breakpoint with number ## (info b will show breakpoint numbers).
+ enable ##: enable the breakpoint with number ##  
___
+ run [r]: begin to execute the code.
+ step [s]: advance to next line (enter into functions).
+ next [n]: advance to next line (don't enter functions).
+ print var [p]: output the value stored in var.
+ watch var: show changes to value of var as they occur.

## The Art of Troubleshooting and Asking Questions

There are no stupid questions, but some questions are more likely to get help than the others:

- Be informative: Describe the problems with necessary information
    - Do not: "Hey TA, my code doesn't work."
    - Do: "My code is supposed to do X, but it does Y. It works as expected upto this point, but this function gives a weird result"
- Be concrete: Use gdb to at least locate which line(s) of code goes wrong; typically checking variables' value and track down uptil which point it has an expected value and you can't understand why it happens.
    - Do not: Send a 200-line source code file to TAs and ask them to debug for you.
    - Do: Use gdb to narrow down the problem.

Also, try the tools we discussed today. They could save you from a lot of hassle.

## Reference Links

- Never forget `man` in your command line :)

- [Cppreference](https://en.cppreference.com/w/c) is primary for C++ reference, but it also has pretty references for C (with examples!).
- [This git tutorial](https://git-scm.com/docs/gittutorial) could be helpful if you are not familiar with git.
