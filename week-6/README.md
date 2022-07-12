# COMP SCI 537 Discussion Week 6

- [Recap](https://github.com/adilahmed31/cs537-Discussion-su22/tree/main/week-4#how-xv6-starts): How xv6 starts
- [Recap](https://github.com/adilahmed31/cs537-Discussion-su22/tree/main/week-4#scheduler-logic-in-xv6): Scheduler Logic in xv6, old discussion video by [Remzi](https://www.youtube.com/watch?v=eYfeOT1QYmg)
- How to approach P3

## FAQs:

- Should we use the pstat struct to make scheduling discussions or implement the logic?  
Ans: That isn't necessary. The pstat struct is used for accounting and is meant to be cumulative over the lifetime of the process. You may add your own fields to other data structures such as ptable, cpu or proc. However, the fields in pstat must be kept up-to-date and accurate results must be returned in the getpinfo call.

- When should we update the fields in the pstat struct?  
Ans: You can update it as you go along or only when getpinfo is called. The "correct" approach heavily depends on your logic / approach. Either method may be more convenient based on your approach.

- Will we need to acquire locks for whichever data structure we create?  
Ans: Yes. It's best to modify existing data structures and simply re-use the acquire and release routines in the existing code. 

## How to approach P3?
- Implement easy syscalls first
- - For e.g. Implement `setslice`, `getslice` and `fork2` first. 
- Test each part independently. 
- - Create custom test user applications. For e.g. when you implement the first 3 syscalls, create a C file, spawn a child process with `fork2`, set it's slice and try to fetch the same with `getslice` syscall. 
- - Debug any issue with GDB (Can't stress this more!). 
- Start Early! (Biggest Hint!)

## More tips:
- Way to pass struct from user program to syscall - https://github.com/himanshusagar/xv6/commit/1b08e48760f2fb5e64fa95aed237eb37641f2988 

