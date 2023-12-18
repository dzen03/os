// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern char end[];  // first address after kernel.
                    // defined by kernel.ld.

struct spinlock refs_lock;
uint64 pages_count;
uint8 *refs;

#define PTR_TO_REFS(x) (uint64)(((uint64)x - PGROUNDUP((uint64)end)) / PGSIZE) 

void kinit() {
  char *p = (char *)PGROUNDUP((uint64)end);
  bd_init(p, (void *)PHYSTOP);

  refs = bd_malloc(sizeof(uint8) * (PTR_TO_REFS(PHYSTOP) + 1));
  memset(refs, 0, pages_count);
  initlock(&refs_lock, "page_refs_lock");
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa) { 
  acquire(&refs_lock);
  if (PTR_TO_REFS(pa) < 0 || PTR_TO_REFS(pa) >= (PTR_TO_REFS(PHYSTOP) + 1))
  {
    panic("freeing wrong page");
  }
  --refs[PTR_TO_REFS(pa)];
  if (refs[PTR_TO_REFS(pa)] == 0)
  {
    bd_free(pa);
  }
  release(&refs_lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  acquire(&refs_lock);
  void *p = bd_malloc(PGSIZE);
  if (p != 0)
    refs[PTR_TO_REFS(p)] = 1;
  release(&refs_lock);
  return p;
}

void add_ref(void *pa) {
  acquire(&refs_lock);
  if (PTR_TO_REFS(pa) < 0 || PTR_TO_REFS(pa) >= (PTR_TO_REFS(PHYSTOP) + 1))
  {
    panic("adding wrong page");
  }
  ++refs[PTR_TO_REFS(pa)];
  release(&refs_lock);
}

int get_ref(void *pa) {
  char refs_count;
  acquire(&refs_lock);
  if (PTR_TO_REFS(pa) < 0 || PTR_TO_REFS(pa) >= (PTR_TO_REFS(PHYSTOP) + 1))
  {
    panic("getting refs wrong page");
  }
  refs_count = refs[PTR_TO_REFS(pa)];
  release(&refs_lock);
  return refs_count;
}
