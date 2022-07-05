# COMP SCI 537 Discussion Week 6

- [Recap](https://github.com/adilahmed31/cs537-Discussion-su22/tree/main/week-4#how-xv6-starts): How xv6 starts
- [Recap](https://github.com/adilahmed31/cs537-Discussion-su22/tree/main/week-4#scheduler-logic-in-xv6): Scheduler Logic in xv6, old discussion video by [Remzi](https://www.youtube.com/watch?v=eYfeOT1QYmg)
- How to approach P3
- `sleep()`: is not really just sleeping

This discussion is meant to be a quick QnA session / office hour. This page will be updated with the most commonly asked questions and their answers.

## How to approach P3?
- Implement easy syscalls first
- - For e.g. Implement `setslice`, `getslice` and `fork2` first. 
- Test each part independently. 
- - Create custom test user applications. For e.g. when you implement the first 3 syscalls, create a C file, spawn a child process with `fork2`, set it's slice and try to fetch the same with `getslice` syscall. 
- - Debug any issue with GDB (Can't stress this more!). 
- Start Early! (Biggest Hint!)

## More tips:
- Way to pass struct from user program to syscall - https://github.com/himanshusagar/xv6/commit/1b08e48760f2fb5e64fa95aed237eb37641f2988 

