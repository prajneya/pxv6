# pxv6: Prajneya's xv6

## Introduction
This is a modification of the xv6 riscv operating system in which the following features have been implemented:
- `trace` system call
- `strace` user program
- `set_priority` system call
- FCFS CPU scheduler
- PBS CPU scheduler
- MLFQ CPU scheduler
- Customized Procdump for PBS and MLFQ

## Specification 1: strace

## Specification 2: Schedulers

### FCFS Scheduler

A policy that selects the process with the lowest creation time.

With the usage of ```ticks``` we store the ```ctime``` (creation time) of process by modifying the ```struct proc```. We then implement the policy as follows:

1. We find the process with least creation time:

```c
for(p = proc; p < &proc[NPROC]; p++){
	acquire(&p->lock);
	if (p->state != RUNNABLE){
	  release(&p->lock);
	  continue;
	}

	if (!first_process)
	{
	  first_process = p;
	}
	else
	{
	  if (p->ctime < first_process->ctime)
	  {
	    first_process = p;
	  }
	}
	release(&p->lock);
	}
``` 

2. We run the process:

```c
for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if (p->state == RUNNABLE && p==first_process){
      p->state = RUNNING;
      c->proc = p;
      p->nrun++;
      swtch(&c->context, &p->context);
      c->proc = 0;
    }
    release(&p->lock);
  }
```

## Specification 3: Procdump

-------------------------------------------------------------------------

ORIGINAL README:

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.828/, which
provides pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Takahiro Aoyagi, Silas Boyd-Wickizer, Anton Burtsev, Ian Chen, Dan
Cross, Cody Cutler, Mike CAT, Tej Chajed, Asami Doi, eyalz800, Nelson
Elhage, Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, flespark,
Peter Froehlich, Yakir Goaron,Shivam Handa, Matt Harvey, Bryan Henry,
jaichenhengjie, Jim Huang, Matúš Jókay, Alexander Kapshuk, Anders
Kaseorg, kehao95, Wolfgang Keller, Jungwoo Kim, Jonathan Kimmitt,
Eddie Kohler, Vadim Kolontsov , Austin Liew, l0stman, Pavan
Maddamsetti, Imbar Marinescu, Yandong Mao, , Matan Shabtay, Hitoshi
Mitake, Carmi Merimovich, Mark Morrissey, mtasm, Joel Nider,
OptimisticSide, Greg Price, Jude Rich, Ayan Shafqat, Eldar Sehayek,
Yongming Shen, Fumiya Shigemitsu, Cam Tenny, tyfkda, Warren Toomey,
Stephen Tu, Rafael Ubal, Amane Uehara, Pablo Ventura, Xi Wang, Keiichi
Watanabe, Nicolas Wolovick, wxdao, Grant Wu, Jindong Zhang, Icenowy
Zheng, ZhUyU1997, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2020 Frans Kaashoek, Robert Morris, and Russ Cox.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
operating system for MIT's 6.S081, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu. Once they are installed, and in your shell
search path, you can run "make qemu".
