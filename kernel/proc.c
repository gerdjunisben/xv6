#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"


#ifndef CPU_SCHEDULER
#define CPU_SCHEDULER
#endif

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;



static struct proc *initproc;

static float loadAverage = 0;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);
static void sched(void);
static struct proc *roundrobin(void);
static struct proc *lowestUtil(void);
static struct proc *highestWait(void);

char *getState(enum procstate state) {
  switch(state) {
    case RUNNABLE: return "runnable";
    case RUNNING: return "run";
    case SLEEPING: return "sleep";
    case EMBRYO: return "embryo";
    case ZOMBIE: return "zombie";
    default: return "";
  }
}


uint procInDisk(uint diskNum)
{
  acquire(&ptable.lock);
  for(int i = 0; i < NPROC; i++) {
    struct proc *curr_proc = &ptable.proc[i];
    if(curr_proc->state == RUNNING ||  curr_proc->state == RUNNABLE || curr_proc->state == SLEEPING)
    {
      if(curr_proc->cwd->dev && curr_proc->cwd->dev == diskNum)
      {
        cprintf("A proc is currently in this dir %d\n",curr_proc->cwd->size);
        release(&ptable.lock);
        return -1;
      }
      for(int j = 0;j<NOFILE;j++)
      {
        if(curr_proc->ofile[j] && curr_proc->ofile[j]->ip && curr_proc->ofile[j]->ip->dev == diskNum)
        {
          cprintf("A proc has an open file to this dir\n");
          release(&ptable.lock);
          return -1;
        }
      }
    }
  }
  release(&ptable.lock);
  return 0;
}



float getLoadAvg(void) {
  return loadAverage;
}

void updateLatency(struct proc *curr_proc)
{
  //no locking of ptable since only called in method that already locks it

  if(curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current - 1)%100] == SLEEPING && curr_proc->state == RUNNABLE)
  {
    curr_proc->isLatency = 1;
    if(curr_proc->tickBuffer.latency[(curr_proc->tickBuffer.current)%100]==SLEEPING)
    {
      curr_proc->isOldLatency = 0;
      curr_proc->tickBuffer.latency[(curr_proc->tickBuffer.current)%100] = 1;
    }
    else
    {
      if(curr_proc->isOldLatency == 0)
      {
        curr_proc->isOldLatency = 1;
      }
    }
  }
  else if(curr_proc->isLatency && curr_proc->state == RUNNABLE)
  {
    if(curr_proc->tickBuffer.latency[(curr_proc->tickBuffer.current)%100]==0)
    {
      curr_proc->isOldLatency = 0;
      curr_proc->tickBuffer.latency[(curr_proc->tickBuffer.current)%100] = 1;
    }
    else
    {
      if(curr_proc->isOldLatency == 0)
      {
        curr_proc->isOldLatency = 1;
      }
    }
  }
  else
  {
    if(curr_proc->isLatency)
    {
      curr_proc->isLatency = 0;
    }
    if(curr_proc->tickBuffer.latency[(curr_proc->tickBuffer.current)%100]==1)
    {
      if(curr_proc->isOldLatency == 0)
      {
        curr_proc->isOldLatency = 1;
      }
      curr_proc->tickBuffer.latency[(curr_proc->tickBuffer.current)%100] = 0;
    }
    else
    {
      curr_proc->isOldLatency = 0;
    }
  }
  
}


void updateLastHundred()
{

  for(int i = 0; i < NPROC; i++) {
    struct proc *curr_proc = &ptable.proc[i];
    if(curr_proc->state == UNUSED)
    {
      continue;
    }
    else
    {
      if(curr_proc->tickBuffer.current > 0)
      {
        updateLatency(curr_proc);
      }
      if((curr_proc->run_time + curr_proc->wait_time + curr_proc->sleep_time) >= 100)
      {
        //handle our lastHundredWait
        if(curr_proc->state != RUNNABLE && curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] == RUNNABLE)
        {
          curr_proc->lastHundredWait-=1;
        }
        else if(curr_proc->state == RUNNABLE && curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] != RUNNABLE)
        {
          curr_proc->lastHundredWait+=1;
        } 


        //handle our lastHundredRun
        if(curr_proc->state != RUNNING && curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] == RUNNING)
        {
          curr_proc->lastHundredRun-=1;
        }
        else if(curr_proc->state == RUNNING && curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] != RUNNING)
        {
          curr_proc->lastHundredRun+=1;
        } 

        //set current tick to respective number
        if(curr_proc->state == SLEEPING)
        {
          curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] = SLEEPING;
        }
        else if (curr_proc->state == RUNNABLE)
        {
          curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] = RUNNABLE;
        }
        else if(curr_proc->state == RUNNING)
        {
          curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] = RUNNING;
        }

      }
      else
      {
        //handle before buffer full
        if(curr_proc->state == RUNNING)
        {
          curr_proc->lastHundredRun+= 1;
          curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] = RUNNING;
        }
        else if(curr_proc->state == RUNNABLE)
        {
          curr_proc->lastHundredWait+=1;
          curr_proc->tickBuffer.ticks[(curr_proc->tickBuffer.current)%100] = RUNNABLE;
        }
      }
      curr_proc->tickBuffer.current++;
    }
  }
  
  
}

void updateUtil()
{

  for(int i = 0; i < NPROC; i++) {
    struct proc *curr_proc = &ptable.proc[i];
    curr_proc->cpuUtil = ((0.999232766* curr_proc->cpuUtil) + ((1.0 - 0.999232766) * curr_proc->lastHundredRun));
  }

}


void updateWait()
{

  for(int i = 0; i < NPROC; i++) {
    struct proc *curr_proc = &ptable.proc[i];
    curr_proc->waitPercent = ((0.999232766* curr_proc->waitPercent) + ((1.0 - 0.999232766) * curr_proc->lastHundredWait));
  }

}

void updateLatencyAvg()
{

  for(int i = 0; i < NPROC; i++) {
    struct proc *curr_proc = &ptable.proc[i];
    int max = 0;
    int cur = 0;
    for(int i =0;i<100;i++)
    {
      if(curr_proc->tickBuffer.latency[i] == 1)
      {
        cur +=1;
      }
      else
      {
        if(cur >max)
        {
          max = cur;
        }
        cur = 0;
      }
    }
    curr_proc->avgLatency = ((0.999232766* curr_proc->avgLatency) + ((1.0 - 0.999232766) * max));
  }

}



void updateLoadAvg(void)
{

  float num_processes = 0.0;
  for(int i = 0; i < NPROC; i++) {
    struct proc *curr_proc = &ptable.proc[i];
    if(curr_proc->state == RUNNING || curr_proc->state == RUNNABLE)
      num_processes++;
  }
  loadAverage =  (0.999616 * loadAverage) + ((1 - 0.999616) * num_processes);

}

void printProcs(void)
{

  for(int i = 0; i < NPROC; i++) {
    struct proc *curr_proc = &ptable.proc[i];
    if(curr_proc->state == UNUSED)
    {
      continue;
    }
    else
    {
      char *state = getState(curr_proc->state);
      cprintf("%d %s %s run:%d wait:%d sleep:%d cpu%:%d wait%:%d latency: %d\n",curr_proc->pid, state,curr_proc->name,curr_proc->run_time,curr_proc->wait_time,curr_proc->sleep_time,(uint)(curr_proc->cpuUtil),(uint)(curr_proc->waitPercent),(uint)(curr_proc->avgLatency));
      //cprintf("latency ticks: %d, latency count: %d, isLatency: %d, isOldLatency: %d\n", curr_proc->latencyTicks, curr_proc->latencyCount, curr_proc->isLatency, curr_proc->isOldLatency);
      /*
      cprintf("Last 100 %d\n",curr_proc->lastHundredRun);
      uint sum =0;
      for(int i =0;i<100;i++)
      {
        if(tickBuffer.ticks[i]==curr_proc->pid)
          sum+=1;
      }
      cprintf("Last 100 %d\n",sum);*/
    }
  }

}


void incProcs(void)
{

  for(int i = 0; i < NPROC; i++) {
    struct proc *curr_proc = &ptable.proc[i];

    if (curr_proc->state == RUNNABLE)
    {
      curr_proc->wait_time++;
    }
    else if(curr_proc->state == RUNNING)
    {
      curr_proc->run_time++;
    }
    else if(curr_proc->state == SLEEPING)
    {
      curr_proc->sleep_time++;
    }
  }


}

void procStats(uint ticks)
{
  acquire(&ptable.lock);
   //cprintf("%d\n", CPU_SCHEDULER);

    // increment processes stats
    incProcs();

    //update load average
    updateLoadAvg();

    //update last hundred ticks
    updateLastHundred();

    //update cpu util percent
    updateUtil();


    //update wait percent
    updateWait();

    //update latency
    updateLatencyAvg();
      

    // printout process statistics
    if (ticks % 1000 == 0) {
      cprintf("\ncpus: %d, uptime: %d, load(x100): %d, scheduler: %s\n", ncpu, ticks, (uint)(100*getLoadAvg()), getCPUSchedName());
      printProcs();
    }
    release(&ptable.lock);
}

char *getCPUSchedName() {
  if (CPU_SCHEDULER == 0) {
    return "round robin";
  }
  else if (CPU_SCHEDULER == 1) {
    return "lowest CPU % first";
  }
  else if (CPU_SCHEDULER == 2) {
    return "highest wait % first";
  }

  else {
    return "round robin";
  }
}


void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);


  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  // initialize time measures
  p->run_time = 0;
  p->wait_time = 0;
  p->sleep_time = 0;
  p->cpuUtil = 0.0;
  p->lastHundredRun = 0;
  p->waitPercent = 0;
  p->lastHundredWait = 0;
  
  for(int i = 0;i<100;i++)
  {
    p->tickBuffer.ticks[i] = 0;
    p->tickBuffer.latency[i] = 0;
  }
  p->tickBuffer.current = 0;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);

  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;

  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
    
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  cprintf("Alloced proc fine\n");

  // Copy process state from proc.
  //cprintf("The fucking stack size %d\n",curproc->stackSize);
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz, curproc->stackSize)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  cprintf("Memory fine\n");
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);
  cprintf("Fork completed\n");
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU idle loop.
// Each CPU calls idle() after setting itself up.
// Idle never returns.  It loops, executing a HLT instruction in each
// iteration.  The HLT instruction waits for an interrupt (such as a
// timer interrupt) to occur.  Actual work gets done by the CPU when
// the scheduler is invoked to switch the CPU from the idle loop to
// a process context.
void
idle(void)
{
  sti(); // Enable interrupts on this processor
  for(;;) {
    if(!(readeflags()&FL_IF))
      panic("idle non-interruptible");
    hlt(); // Wait for an interrupt
  }
}

// The process scheduler.
//
// Assumes ptable.lock is held, and no other locks.
// Assumes interrupts are disabled on this CPU.
// Assumes proc->state != RUNNING (a process must have changed its
// state before calling the scheduler).
// Saves and restores intena because the original xv6 code did.
// (Original comment:  Saves and restores intena because intena is
// a property of this kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would break in the few
// places where a lock is held but there's no process.)
//
// When invoked, does the following:
//  - choose a process to run
//  - swtch to start running that process (or idle, if none)
//  - eventually that process transfers control
//      via swtch back to the scheduler.

static void
sched(void)
{
  int intena;
  struct proc *p;
  struct context **oldcontext;
  struct cpu *c = mycpu();
  
  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(c->ncli != 1)
    panic("sched locks");
  if(readeflags()&FL_IF)
    panic("sched interruptible");

  // Determine the current context, which is what we are switching from.
  if(c->proc) {
    if(c->proc->state == RUNNING)
      panic("sched running");
    oldcontext = &c->proc->context;
  } else {
    oldcontext = &(c->scheduler);
  }

  // Choose next process to run.
  if (CPU_SCHEDULER == 0) {
    p = roundrobin();
  }
  else if (CPU_SCHEDULER == 1) {
    p = lowestUtil();
  }
  else if (CPU_SCHEDULER == 2) {
    p = highestWait();
  }

  else {
    p = roundrobin();
  }
      
    if((p) != 0) {
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      p->state = RUNNING;
      switchuvm(p);
      if(c->proc != p) {
        c->proc = p;
        intena = c->intena;
        swtch(oldcontext, p->context);
        mycpu()->intena = intena;  // We might return on a different CPU.
      }
    } else {
      // No process to run -- switch to the idle loop.
      switchkvm();
      if(oldcontext != &(c->scheduler)) {
        c->proc = 0;
        intena = c->intena;
       swtch(oldcontext, c->scheduler);
        mycpu()->intena = intena;
      }
    }
  
  
}

// Round-robin scheduler.
// The same variable is used by all CPUs to determine the starting index.
// It is protected by the process table lock, so no additional lock is
// required.
static int rrindex;

static struct proc *
roundrobin()
{
  // Loop over process table looking for process to run.
  //cprintf("ROUND ROBIN\n");
  for(int i = 0; i < NPROC; i++) {
    struct proc *p = &ptable.proc[(i + rrindex + 1) % NPROC];
    if(p->state != RUNNABLE)
      continue;
    rrindex = p - ptable.proc;
    return p;
  }
  return 0;
}

static struct proc *
lowestUtil()
{
  //cprintf("LOWEST CPU\n");
  struct proc *lowest = 0;
  int i;
  for(i = 0; i < NPROC; i++) {
    struct proc *p = &ptable.proc[i];
    if(p->state == RUNNABLE)
    {
      lowest = p;
      break;
    }
  }
  if(lowest ==0)
  {
    return lowest;
  }
  i+=1;
  for(; i < NPROC; i++) {
    if(ptable.proc[i].state != RUNNABLE)
      continue;
    else
    {
      if(ptable.proc[i].cpuUtil < lowest->cpuUtil)
      {
        lowest = &ptable.proc[i];
      }
    }
  }
  return lowest;
}

static struct proc *
highestWait()
{
  //cprintf("HIGHEST WAIT TIME\n");
  struct proc *highest = 0;
  int i;
  for(i = 0; i < NPROC; i++) {
    struct proc *p = &ptable.proc[i];
    if(p->state == RUNNABLE)
    {
      highest = p;
      break;
    }
  }
  if(highest ==0)
  {
    return highest;
  }
  i+=1;
  for(; i < NPROC; i++) {
    if(ptable.proc[i].state != RUNNABLE)
      continue;
    else
    {
      if(ptable.proc[i].waitPercent > highest->waitPercent)
      {
        highest = &ptable.proc[i];
      }
    }
  }
  return highest;
}

// Called from timer interrupt to reschedule the CPU.
void
reschedule(void)
{
  struct cpu *c = mycpu();

  acquire(&ptable.lock);
  if(c->proc) {
    if(c->proc->state != RUNNING)
      panic("current process not in running state");
    c->proc->state = RUNNABLE;
  }
  sched();
  // NOTE: there is a race here.  We need to release the process
  // table lock before idling the CPU, but as soon as we do, it
  // is possible that an an event on another CPU could cause a process
  // to become ready to run.  The undesirable (but non-catastrophic)
  // consequence of such an occurrence is that this CPU will idle until
  // the next timer interrupt, when in fact it could have been doing
  // useful work.  To do better than this, we would need to arrange
  // for a CPU releasing the process table lock to interrupt all other
  // CPUs if there could be any runnable processes.
  release(&ptable.lock);
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

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

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

//PAGEBREAK!
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

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  cprintf("\nMemory pages: %d, used: %d, free: %d\n", getTotalPages(), getUsedPages(), getFreePages());
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    uint counts[2];
    countProcPages(p->pgdir,counts);
    
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s [%d/%d] %s", p->pid, state,counts[0],counts[1], p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}