#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define CHANSIZE 10
#define PIPESIZE 128

struct chan {
  struct spinlock lock;

  int closed;
  char buf[CHANSIZE][PIPESIZE];
  uint qcount;   // number elements in the buffer
  uint dataqsiz; // number of maximum elements in the buffer
  uint sendx;    // position in the buffer for the next element to be received
  uint recvx;    // position in the buffer for the next element to be returned
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

int
chanalloc(struct file **f0, struct file **f1, int count)
{
  struct chan *ch;

  ch = 0;
  *f0 = *f1 = 0;
  if (count > CHANSIZE)
    goto bad;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((ch = (struct chan*)kalloc()) == 0)
    goto bad;
  ch->closed = 0;
  ch->qcount = 0;
  ch->dataqsiz = count;
  ch->recvx = ch->sendx = 0;
  initlock(&ch->lock, "pipe");
  (*f0)->type = FD_CHAN;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->chan = ch;
  (*f1)->type = FD_CHAN;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->chan = ch;
  return 0;

 bad:
  if(ch)
    kfree((char*)ch);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
chanclose(struct chan *ch, int writable)
{
  acquire(&ch->lock);
  if(writable){
    ch->writeopen = 0;
    wakeup(&ch->recvx);
  } else {
    ch->readopen = 0;
    wakeup(&ch->sendx);
  }
  if(ch->readopen == 0 && ch->writeopen == 0){
    release(&ch->lock);
    kfree((char*)ch);
  } else
    release(&ch->lock);
}

int
chanwrite(struct chan *ch, uint64 addr, int n)
{
  int i = 0;
  struct proc *pr = myproc();

  acquire(&ch->lock);
  if(ch->readopen == 0 || killed(pr)){
    release(&ch->lock);
    return -1;
  }
  if(ch->qcount == ch->dataqsiz){ //DOC: chanwrite-full
    wakeup(&ch->recvx);
    sleep(&ch->sendx, &ch->lock);
  } else {
    if(copyin(pr->pagetable, ch->buf[ch->sendx++], addr + i, n) == 0)
    {
      i = n;
      ch->sendx = (ch->sendx + 1) % ch->dataqsiz;
      ++ch->qcount;
    }
  }

  if(ch->qcount == ch->dataqsiz){ //DOC: chanewrite-full
    wakeup(&ch->recvx);
    sleep(&ch->sendx, &ch->lock);
  }
  wakeup(&ch->recvx);
  release(&ch->lock);

  return i;
}

int
chanread(struct chan *ch, uint64 addr, int n)
{
  int i = 0;
  struct proc *pr = myproc();

  acquire(&ch->lock);
  while(ch->qcount == 0){  //DOC: pipe-empty
    if(killed(pr) ){
      release(&ch->lock);
      return -1;
    }
    sleep(&ch->recvx, &ch->lock); //DOC: piperead-sleep
  }
  if(ch->qcount != 0)
  {
    if(copyout(pr->pagetable, addr, ch->buf[ch->recvx++], n) == 0)
    {
      i = n;
      ch->recvx = (ch->recvx + 1) % ch->dataqsiz;
      --ch->qcount;
    }
  }
  wakeup(&ch->sendx);  //DOC: piperead-wakeup
  release(&ch->lock);
  return i;
}
