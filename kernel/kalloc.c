// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

// page_ref
struct spinlock ref_lock;
int ref_count[PGROUNDUP(PHYSTOP) / PGSIZE];

void dec_page_ref(void *phy_ad)
{
  acquire(&ref_lock);
  if (ref_count[((uint64)phy_ad) / PGSIZE] <= 0)
    panic("dec_page_ref");
  ref_count[((uint64)phy_ad) / PGSIZE] -= 1;
  release(&ref_lock);
}

void inc_page_ref(void *phy_ad)
{
  acquire(&ref_lock);
  if (ref_count[((uint64)phy_ad) / PGSIZE] < 0)
    panic("inc_page_ref");
  ref_count[((uint64)phy_ad) / PGSIZE] += 1;
  release(&ref_lock);
}

void kinit()
{
  initlock(&ref_lock, "page_ref");
  acquire(&ref_lock);
  for (int i = 0; i < (PGROUNDUP(PHYSTOP) / PGSIZE); i++)
    ref_count[i] = 0;
  release(&ref_lock);
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    inc_page_ref(p);
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  dec_page_ref(pa);
  acquire(&ref_lock);
  int res = ref_count[(uint64)pa / PGSIZE];
  if (ref_count[((uint64)pa) / PGSIZE] < 0)
    panic("get_page_ref");
  release(&ref_lock);

  if (res > 0)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r)
  {
    memset((char *)r, 5, PGSIZE); // fill with junk
    inc_page_ref((void *)r);
  }
  return (void *)r;
}
