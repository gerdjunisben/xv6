// Simple PIO-based (non-DMA) IDE driver code.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "file.h"

#define SECTOR_SIZE   512
#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct spinlock sidelock;
static struct buf *idequeue;

static int havedisk0;
static int havedisk1;
static int havedisk2;
static int havedisk3;
static void idestart(struct buf*);

// Wait for IDE disk to become ready.
static int
idewait(int checkerr,uint isSecondary)
{
  int r;
  uint byte =0x1f7;
  if(isSecondary)
  {
    byte = 0x177;
  }


  while(((r = inb(byte)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
    ;
  if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
    return -1;
  return 0;
}

int
ideread(struct inode *ip,  char *dst, int n){
  iunlock(ip);
  int comNumber = ip->minor;
  struct spinlock* lock;
  if(comNumber == 0 || comNumber == 1)
  {
    lock = &(idelock);
  }
  else if(comNumber == 2 || comNumber == 3)
  {
    lock = &(sidelock);
  }
  else
  {
    return -1;
  }

  acquire(&(*lock));



  release(&(*lock));
  ilock(ip);
}

int
idewrite(struct inode *ip, char *buf, int n)
{
  iunlock(ip);



  ilock(ip);
}

void
ideinit(void)
{
  int i;

  initlock(&idelock, "ide");
  initlock(&sidelock, "side");
  ioapicenable(IRQ_IDE, ncpu - 1);
  ioapicenable(IRQ_SIDE, ncpu-1);
  idewait(0,0);

  // Check if disk 1 is present
  outb(0x1f6, 0xe0 | (1<<4));
  for(i=0; i<1000; i++){
    if(inb(0x1f7) != 0){
      havedisk1 = 1;
      break;
    }
  }

  // Switch back to disk 0.
  outb(0x1f6, 0xe0 | (0<<4));
  for(i=0; i<1000; i++){
    if(inb(0x1f7) != 0){
      havedisk0 = 1;
      break;
    }
  }


  idewait(0,1);


  //check if disk 3 is present
  outb(0x176, 0xe0 | (1<<4));
  for(i=0; i<1000; i++){
    if(inb(0x177) != 0){
      havedisk3 = 1;
      break;
    }
  }
  //switch back to disk 2 and make sure it's present
  outb(0x176, 0xe0 | (0<<4));
  for (i = 0; i < 1000; i++) {
  if (inb(0x177) != 0) {
    havedisk2 = 1; 
    break;
  }


  devsw[IDE].write = idewrite;
  devsw[IDE].read = ideread;
  }
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
  uint isSecondary = 0;
  uint base1 = 0x1f0;
  uint base2 = 0x3f6;
  if(b->dev == 2 || b->dev == 3)
  {
    base1 = 0x170;
    base2 = 0x376;
    isSecondary = 1;
    if(b->dev == 2)
    {
      outb(base1 + 6, 0xe0 | (0 << 4));
    }
    else
    {
      outb(base1 + 6, 0xe0 | (1 << 4));
    }
  }
  else if(b->dev == 0 || b->dev ==1)
  {
    if(b->dev == 0)
    {
      outb(base1 + 6, 0xe0 | (0 << 4));
    }
    else
    {
      outb(base1 + 6, 0xe0 | (1 << 4));
    }
  }
  else
  {
    panic("Invalid disk");
  }
  if(b == 0)
    panic("idestart");
  if(b->blockno >= FSSIZE)
    panic("incorrect blockno");
  int sector_per_block =  BSIZE/SECTOR_SIZE;
  int sector = b->blockno * sector_per_block;
  int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
  int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

  if (sector_per_block > 7) panic("idestart");

  idewait(0,isSecondary);
  outb(base2, 0);  // generate interrupt
  outb(base1 + 2, sector_per_block);  // number of sectors
  outb(base1 + 3, sector & 0xff);
  outb(base1 + 4, (sector >> 8) & 0xff);
  outb(base1 + 5, (sector >> 16) & 0xff);
  outb(base1 + 6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
  if(b->flags & B_DIRTY){
    outb(base1 + 7, write_cmd);
    outsl(base1, b->data, BSIZE/4);
  } else {
    outb(base1 + 7, read_cmd);
  }
}

// Interrupt handler.
void
ideintr()
{
  uint isSecondary = 0;
  uint base1 = 0x1f0;
  struct spinlock* curLock = &idelock;
  struct buf *b;

  // First queued buffer is the active request.
  acquire(curLock);

  if((b = idequeue) == 0){
    release(curLock);
    return;
  }
  cprintf("The dev num %d\n",b->dev);

  if(b->dev == 2 || b->dev == 3)
  {
    release(curLock);
    isSecondary = 1;
    base1 = 0x170;
    curLock = &sidelock;
    acquire(curLock);
  }
  idequeue = b->qnext;

  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1,isSecondary) >= 0)
    insl(base1, b->data, BSIZE/4);

  // Wake process waiting for this buf.
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
  wakeup(b);

  // Start disk on next buf in queue.
  if(idequeue != 0)
    idestart(idequeue);

  release(curLock);
}

//PAGEBREAK!
// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
  struct spinlock* curlock = &idelock;
  if(b->dev == 2 || b->dev == 3)
  {
    curlock = &sidelock;
  }

  struct buf **pp;
  
  if(!holdingsleep(&b->lock))
    panic("iderw: buf not locked");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("iderw: nothing to do");
  if(b->dev ==1 && !havedisk1)
    panic("iderw: ide disk 1 not present");
  if(b->dev ==2 && !havedisk2)
    panic("iderw: ide disk 2 not present");
  if(b->dev ==3 && !havedisk3)
    panic("iderw: ide disk 3 not present");

  acquire(curlock);  //DOC:acquire-lock

  // Append b to idequeue.
  b->qnext = 0;
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
    ;
  *pp = b;

  // Start disk if necessary.
  if(idequeue == b)
    idestart(b);

  // Wait for request to finish.
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
    sleep(b, &idelock);
  }


  release(curlock);
}


