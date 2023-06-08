#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace()
{
  // printf("hello11\n");
  int mask;
  argint(0, &mask);
  // printf("hello12\n");
  myproc()
      ->trace_mask = mask;
  return 0;
}

uint64 sys_sigreturn(void)
{
  struct proc *p = myproc();
  p->new_trapframe->kernel_sp = p->trapframe->kernel_sp;
  p->new_trapframe->kernel_trap = p->trapframe->kernel_trap;
  p->new_trapframe->kernel_satp = p->trapframe->kernel_satp;
  p->new_trapframe->kernel_hartid = p->trapframe->kernel_hartid;
  *(p->trapframe) = *(p->new_trapframe);
  p->bool_sigalarm = 0;
  return p->trapframe->a0;
}

uint64
sys_set_priority()
{
  int priority, pid, oldpriority = 110;
  argint(0, &priority);
  argint(1, &pid);

  if (pid < 0 || priority < 0)
    return -1;

  for (struct proc *p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);

    if (p->pid == pid)
    {
      if (priority >= 0 && priority <= 100)
      {
        oldpriority = p->static_priority;
        p->static_priority = priority;
        p->running_time = 0;
        p->sleeping_time = 0;
      }
    }

    release(&p->lock);
  }
  if (priority < oldpriority)
    yield();
  return oldpriority;
}
