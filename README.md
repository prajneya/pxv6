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

A system call, trace, and an accompanying user program strace.

Usage ```strace mask command [args]```

1. We first add a ```sys_trace()``` function in ```systproc.c```:

```c
uint64
sys_trace(void){
  if (argint(0, &myproc()->tracemask) < 0)
    return -1;

  return 0;
}
```

where we add 

```c
int tracemask;               // trace mask
```

in ```struct proc```

2. We modify ```proc.c``` where we copy the tracemask to children while forking:

```c
// copy trace mask
  np->tracemask = p->tracemask;
```

3. We add new system call in respective header files (```syscall.h```) and modify ```syscall.c``` as:

```c

// trace  //
  if (p->tracemask >> num) {
    printf("%d: syscall %s (", p->pid, syscall_names[num]);
    int syscall_arg;

    for(int syscall_iterator = 0; syscall_iterator<syscall_argc[num]; syscall_iterator++)
    {
      argint(syscall_iterator, &syscall_arg);
      printf("%d ", syscall_arg);
    }

    printf(") -> %d\n", p->trapframe->a0);
  }

```
with respective declarations of functions and following new arrays:

```c
static char *syscall_names[25] = {
  "none",  "fork",  "exit",   "wait",   "pipe",  "read",  "kill",   "exec",
  "fstat", "chdir", "dup",    "getpid", "sbrk",  "sleep", "uptime", "open",
  "write", "mknod", "unlink", "link",   "mkdir", "close", "trace", "waitx",
  "set_priority"
};

static int syscall_argc[25] = {
  0, 0, 1, 1, 1, 3, 1, 2, 
  2, 1, 1, 0, 1, 1, 0, 2,
  3, 3, 1, 2, 1, 1, 0, 0, 
  0
};
```

4. We finally expose the kernel to the user to allow the usage of ```sys_trace()``` by a userprogram ```strace()``` by defining the function in ```user.h`` and create a ```strace.c``` and modify the makefile.

Contents of ```strace.c```:

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[]){
  int i;
  char *nargv[MAXARG];

  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    printf("WRONG FORMAT\n");
    exit(1);
  }

  if (trace(atoi(argv[1])) < 0) {
    printf("TRACE FAILED\n");
    exit(1);
  }
  
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }
  exec(nargv[0], nargv);
  exit(0);
}
```

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

### PBS Scheduler

A non-preemptive priority-based scheduler that selects the process with the highest priority for execution. In case two or more processes have the same priority, we use the number of times the process has been scheduled to break the tie. If the tie remains, we use the start-time of the process to break the tie.

The priority is calculated based upon two types: A static Priority, which is stored in the ```struct proc``` and a dynamic priority which is calculated based on the static priority and niceness (a factor depending upon the running and waiting times of the process).

1. We implement the ```set_priority``` function as follows:

```c
int 
set_priority(int new_priority, int pid)
{
  int old_priority = -1;
  struct proc* p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if (p->pid == pid)
    {
      old_priority = p->priority;
      p->priority = new_priority;
    }
    release(&p->lock);
  }

  return old_priority;
}
```

```struct proc``` is modified as follows:

```c

uint rtime;                  // How long the process ran for 
uint ctime;                  // When was the process created
uint etime;                  // When did the process exit 

int priority;                // Static priority of process
int nrun;                    // Number of times process is run

```

2. We iterate the process table as shown ealier and first calculate dynamic priority and calculate ties to find process to run:

```c
  int sleep_time = p->etime - p->ctime - p->rtime;
  if(sleep_time<0){
    sleep_time = 0;
  }
  
  int denom = p->etime - p->ctime;

  int niceness = 0;

  if(denom<=0){
    niceness = 5;
  }
  else{
    niceness = (sleep_time/denom)*10;
  }

  int dynamic_priority = (p->priority - niceness + 5) > 100 ? 100: (p->priority-niceness + 5);

  if(dynamic_priority<=0){
    dynamic_priority = 0;
  }

  if (p->state != RUNNABLE){
    release(&p->lock);
    continue;
  }

  if (!highest_priority_process)
  {
    highest_priority_process = p;
    highest_priority = dynamic_priority;
  }
  else
  {
    if (dynamic_priority < highest_priority)
    {
      highest_priority_process = p;
      highest_priority = dynamic_priority;
    }
    else if (dynamic_priority == highest_priority){
      if(p->nrun > highest_priority_process->nrun){
        highest_priority_process = p;
      }
      else if(p->nrun > highest_priority_process->nrun){
        if(p->ctime < highest_priority_process->ctime){
          highest_priority_process = p;
        }
      }
    }
  }
```

3. We run the process:

```c
for(p = proc; p < &proc[NPROC]; p++){
  acquire(&p->lock);
  if(p->state == RUNNABLE && p==highest_priority_process){
    p->state = RUNNING;
    c->proc = p;
    p->nrun++;
    swtch(&c->context, &p->context);
    c->proc = 0;
  }
  release(&p->lock);
}
```

### MLFQ Scheduler

A simplified preemptive MLFQ scheduler that allows processes to move between different priority queues based on their behavior and CPU bursts.

1. We create 5 queues, and their respective time slices.

```c
struct proc *queues[5][NPROC];
int queue_iterator[5];
int max_ticks_in_queue[5] = {1, 2, 4, 8, 16};
```

```struct proc``` is modified as follows:

```c
  int queue;                  // Current queue of the process
  int ticks_in_current_slice; // Ticks in current time slice
  int ticks[5];               // Number of ticks received at each of the five queues
  int last_executed;          // Last execution time
```

2. The above variables are by default initialised to 0, unless specified otherwise. When a process is created in ```allocproc()```, we initialise accordingly (we add process to Queue0 - highest priority):

```c
queues[0][queue_iterator[0]] = p;
queue_iterator[0]++;

p->queue = 0;
p->ticks_in_current_slice = 0;

p->nrun = 0;
for (int i = 0; i < 5; i++){
	p->ticks[i] = 0;
}
```

3. Check for expiration of time slices:

```c
for (p = proc; p < &proc[NPROC]; p++){
  acquire(&p->lock);
  if (p->state != RUNNABLE){
    release(&p->lock);
    continue;
  }
  if (p->ticks_in_current_slice >= max_ticks_in_queue[p->queue])
  {
    if (p->queue != 4)
    {
      p->queue++;
      p->ticks_in_current_slice = 0;
    }
  }
  release(&p->lock);
}
```

4. To promote processes with waiting times > 1000, we implement aging:

```c
for (p = proc; p < &proc[NPROC]; p++){
  acquire(&p->lock);
  if (p->state != RUNNABLE){
    release(&p->lock);
    continue;
  }
  if(ticks - p->last_executed > 1000)
  {
    if(p->queue!=0)
    {
      p->queue--;
      p->ticks_in_current_slice = 0;
    }

  }
  release(&p->lock);
}
```

5. We finally run processes in order of their priority, in a round robin fashion:

```c
struct proc *process_to_run = 0;
for (int priority = 0; priority < 5; priority++){
  for (p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if (p->state != RUNNABLE){
      release(&p->lock);
      continue;
    }
    if (p->queue == priority)
    {
      process_to_run = p;
      release(&p->lock);
      goto run_proc;
    }
    release(&p->lock);
  }
}
run_proc:
  for (p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if (p->state == RUNNABLE && p==process_to_run){
      p->nrun++;
      c->proc = p;
      p->state = RUNNING;
      swtch(&c->context, &p->context);
      c->proc = 0;
    }
    release(&p->lock);
  }
```

### Performance Comparision

|Type                       | Default(RoundRobin)        | FCFS          | PBS          | MLFQ          |
| :---:                     | :------------------------: | :-----------: | :----------: | :-----------: |
|  Average Running Time     | 123                        | 98            | 127          | 128           |
|  Average Waiting TIme     | 23                         | 24			 | 11           | 11            |


## Specification 3: Procdump

Printing information about processes, specifically for PBS and MLFQ:

Here, ```wtime``` is calculated using ```etime```, ```ctime``` and ```rtime```: ```w_time = p->etime - p->ctime - p->rtime;```

```c

 #ifdef MLFQ
    printf("PID\tPriority\tState\t\tr_time\tw_time\tn_run\tq0\tq1\tq2\tq3\tq4\n");
    for(p = proc; p < &proc[NPROC]; p++){
      if(p->state == UNUSED)
        continue;
      if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
        state = states[p->state];
      else
        state = "???";
      int w_time = p->etime - p->ctime - p->rtime;
      if(w_time < 0)
        w_time = 0;
      printf("%d\t%d\t\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", p->pid, p->priority, state, p->rtime, w_time, p->nrun, p->ticks[0], p->ticks[1], p->ticks[2], p->ticks[3], p->ticks[4]);
      printf("\n");
    }
  #else
    #ifdef PBS
    printf("PID\tPriority\tState\t\tr_time\tw_time\tn_run\n");
    for(p = proc; p < &proc[NPROC]; p++){
      if(p->state == UNUSED)
        continue;
      if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
        state = states[p->state];
      else
        state = "???";
      int w_time = p->etime - p->ctime - p->rtime;
      if(w_time < 0)
        w_time = 0;
      printf("%d\t%d\t\t%s\t%d\t%d\t%d\n", p->pid, p->priority, state, p->rtime, w_time, p->nrun);
      printf("\n");
    }
    #else
    for(p = proc; p < &proc[NPROC]; p++){
      if(p->state == UNUSED)
        continue;
      if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
        state = states[p->state];
      else
        state = "???";
      printf("%d %s %s", p->pid, state, p->name);
      printf("\n");
    }
    #endif
  #endif

```

QUESTION: If a process voluntarily relinquishes control of the CPU(eg. For doing I/O), it leaves the queuing network, and when the process becomes ready again after the I/O, it is​ ​inserted at the tail of the same queue, from which it is relinquished earlier​ ​( Q: Explain in the README how could this be exploited by a process.

ANSWER: When a process is I/O bound, MLFQ assumes that it is also likely to be I/O bound in the future. To exploit this behavior, the scheduler can favor jobs that have used the least amount of CPU time, thus prioritising other jobs that have continuously being higher in the priority queues.

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
