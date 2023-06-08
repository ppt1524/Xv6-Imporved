xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6). xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Takahiro Aoyagi, Silas Boyd-Wickizer, Anton Burtsev, carlclone, Ian
Chen, Dan Cross, Cody Cutler, Mike CAT, Tej Chajed, Asami Doi,
eyalz800, Nelson Elhage, Saar Ettinger, Alice Ferrazzi, Nathaniel
Filardo, flespark, Peter Froehlich, Yakir Goaron, Shivam Handa, Matt
Harvey, Bryan Henry, jaichenhengjie, Jim Huang, Matúš Jókay, John
Jolly, Alexander Kapshuk, Anders Kaseorg, kehao95, Wolfgang Keller,
Jungwoo Kim, Jonathan Kimmitt, Eddie Kohler, Vadim Kolontsov, Austin
Liew, l0stman, Pavan Maddamsetti, Imbar Marinescu, Yandong Mao, Matan
Shabtay, Hitoshi Mitake, Carmi Merimovich, Mark Morrissey, mtasm, Joel
Nider, Hayato Ohhashi, OptimisticSide, Harry Porter, Greg Price, Jude
Rich, segfault, Ayan Shafqat, Eldar Sehayek, Yongming Shen, Fumiya
Shigemitsu, Cam Tenny, tyfkda, Warren Toomey, Stephen Tu, Rafael Ubal,
Amane Uehara, Pablo Ventura, Xi Wang, WaheedHafez, Keiichi Watanabe,
Nicolas Wolovick, wxdao, Grant Wu, Jindong Zhang, Icenowy Zheng,
ZhUyU1997, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2022 Frans Kaashoek, Robert Morris, and Russ Cox.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
operating system for MIT's 6.1810, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu. Once they are installed, and in your shell
search path, you can run "make qemu".

## System calls and their Implementation:

---

### System call 1 : trace

Added the system call trace and user command strace
will be executed as follows :

strace mask command [args]

strace runs the specified command until it exits.

It tracks and records the system calls which are called by a process during its
execution and print the following details regarding system call:

1. The process id
2. The name of the system call
3. The decimal value of the arguments
4. The return value of the syscall.

**Implementation:**

1. Added a new variable trace_mask in struct proc in kernel/proc.h and initialised it to 0 in allocproc() in kernel/proc.c and making sure that child process inherits the mask value from parent process.

```
// proc.h
// strace
  int trace_mask; // Mask for strace
```

```
// proc.c
// strace
  p->trace_mask = 0;
```

2. Added a user function strace.c in user and added entry(”trace”) in user/usys.pl

```
entry(”trace”)
```

3. Made necessary changes kernel/syscall.c, kernel/syscall.h and changed certain part of syscall() to add new syscall.

4. Implemented a function sys_trace() in kernel/sysproc.c to set trace_mask value given by user

```
uint64
sys_trace()
{
  int mask;
  argint(0, &mask);
  myproc()
      ->trace_mask = mask;
  return 0;
}
```

### System Call 2 : sigalarm and sigreturn :

In this specification,, added a feature to xv6 that periodically alerts a process as it
uses CPU time. This might be useful for compute-bound processes that want to limit
how much CPU time they chew up, or for processes that want to compute but also want to take some periodic action.
Added a new sigalarm(interval, handler) system call. If an application
calls alarm(n, fn) , then after every n "ticks" of CPU time that the program consumes,
the kernel will cause application function
application will resume where it left off.
fn to be called. When fn returns, the Add another system call sigreturn() , to reset the process state to before the handler
was called. This system call needs to be made at the end of the handler so the process can resume where it left off.

**Implementation:**

1. Added new variables in struct proc in kernel/proc.h.

```
  // alarm init
  int interval;
  int till_tick;
  int bool_sigalarm;
  uint64 handler;
  struct trapframe *new_trapframe;
```

2. Implemented a function sys_sigreturn() and sys_sigalarm() in kernel/sysproc.c

```
uint64 sys_sigalarm(void)
{
  uint64 handler;
  argaddr(1, &handler);
  if (handler < 0)
    return -1;
  int ticks;
  argint(0, &ticks);
  if (ticks < 0)
    return -1;
  myproc()->interval = ticks;
  myproc()->till_tick = 0;
  myproc()->bool_sigalarm = 0;
  myproc()->handler = handler;
  return 0;
}
```

3. Changing values of interval and till_tick in usertrap() in kernel/trap.c and changing bool_sigalarm variable in proc to 0 and reset it to 1 in sigreturn because incase of handler function runs for more time than ticks after which handler should be ran:

```
if (which_dev == 2)
  {
    p->till_tick++;
    if (!p->bool_sigalarm && p->interval && p->till_tick >= p->interval)
    {
      p->bool_sigalarm = 1;
      p->till_tick = 0;
      *(p->new_trapframe) = *(p->trapframe);
      p->trapframe->epc = p->handler;
    }
    yield();
  }
```

## Specification 3: Copy-on-write fork:

1.) Modify uvmcopy() to map the parent's physical pages into the child, instead of allocating new pages. Clear PTE_W in the PTEs of both child and parent.
That's to say, we need to make those pages of parents which is marked as writable unwritable and use a new bit to mark them as COW page. By doing so we can share readable pages between parents and children and when need to write on COW pages we allocate new pages (PTE_C).

```
if (flags & PTE_W)
{
    flags = (flags & (~PTE_W)) | PTE_C;
    *pte = PA2PTE(pa) | flags;
}
if (mappages(new, i, PGSIZE, pa, flags) != 0)
{
    goto err;
}
inc_page_ref((void *)pa);
```

2.) Modified usertrap() to recognize page faults. When a page-fault occurs on a COW page, allocated a new page with kalloc(), copied the old page to the new page, and installed the new page in the PTE with PTE_W set.

```
int page_fault_handler(void *va, pagetable_t pagetable)
{
  struct proc *p = myproc();
  if (((uint64)va < PGROUNDDOWN(p->trapframe->sp) - PGSIZE || (uint64)va > PGROUNDDOWN(p->trapframe->sp)) && (uint64)va < MAXVA)
  {
    uint64 page;
    uint flags;
    pte_t *pte;

    va = (void *)PGROUNDDOWN((uint64)va);
    pte = walk(pagetable, (uint64)va, 0);

    if (!pte || !(page = PTE2PA(*pte)))
      return -1;

    flags = PTE_FLAGS(*pte);
    if (flags & PTE_C)
    {
      flags = (~PTE_C) & (flags | PTE_W);
      char *mem = kalloc();
      if (mem)
      {
        memmove(mem, (void *)page, PGSIZE);
        *pte = flags | PA2PTE(mem);
        kfree((void *)page);
        return 0;
      }
      else
        return -1;
    }
  }
  else
    return 1;
  return 0;
}
```

```
// in usertrap()
else if (r_scause() == 15)
  {
    if (r_stval())
    {
      int res = page_fault_handler((void *)r_stval(), p->pagetable);
      if (res == 1 || res == -1)
        p->killed = 1;
    }
    else
      p->killed = 1;
  }
```

3.) Ensured that each physical page is freed when the last PTE reference to it goes away -- but not before. A good way to do this is to keep, for each physical page, a "reference count" of the number of user page tables that refer to that page. Set a page's reference count to one when kalloc() allocates it. Incremented a page's reference count when fork causes a child to share the page, and decremented a page's count each time any process drops the page from its page table. kfree() should only place a page back on the free list if its reference count is zero.

```
// page_ref
struct spinlock ref_lock;
int ref_count[PGROUNDUP(PHYSTOP) / PGSIZE];
```

```
void dec_page_ref(void *phy_ad)
{
  acquire(&ref_lock);
  if (ref_count[(uint64)phy_ad / PGSIZE] <= 0)
    panic("dec_page_ref");
  ref_count[(uint64)phy_ad / PGSIZE] -= 1;
  release(&ref_lock);
}

void inc_page_ref(void *phy_ad)
{
  acquire(&ref_lock);
  if (ref_count[(uint64)phy_ad / PGSIZE] < 0)
    panic("inc_page_ref");
  ref_count[(uint64)phy_ad / PGSIZE] += 1;
  release(&ref_lock);
}
```

```
// kfree()
dec_page_ref(pa);
  acquire(&ref_lock);
  int res = ref_count[(uint64)pa / PGSIZE];
  if (ref_count[(uint64)pa / PGSIZE] < 0)
    panic("get_page_ref");
  release(&ref_lock);

  if (res > 0)
    return;
```

4.) Modified copyout() to use the same scheme as page faults when it encounters a COW page.

```
pte = walk(pagetable, va0, 0);
flags = PTE_FLAGS(*pte);
if (PTE_C & flags)
{
    page_fault_handler((void *)va0, pagetable);
    pa0 = walkaddr(pagetable, va0);
}
```
