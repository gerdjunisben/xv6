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



#define min(a, b) ((a) < (b) ? (a) : (b))

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
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
ideread(struct inode *ip,  char *dst, int n, uint offset){
  cprintf("IDEREAD %d bytes, %d offset, %d disk\n",n,offset,ip->size);
  iunlock(ip);
  
  uint m =0;
  struct buf *bp;

/*
  if(offset > ip->size || offset + n < offset)
  {
    cprintf("Returning -1\n");
    ilock(ip);
    return -1;
  }*/
  /*
  if(offset + n > ip->size)
  {
    n = ip->size - offset;
  }*/
  begin_op();
  for(uint tot=0; tot<n; tot+=m, offset+=m, dst+=m){
    //cprintf("block number %d\n",offset/BSIZE);
    bp = bread(ip->minor, offset/BSIZE);
    m = min(n - tot, BSIZE - offset%BSIZE);
    memmove(dst, bp->data + offset%BSIZE, m);
    brelse(bp);
  }
  end_op();

  ilock(ip);
   cprintf("COMPLETED IDEREAD\n");
  return n;
}

int
idewrite(struct inode *ip, char *buf, int n,uint offset)
{
  cprintf("IDEWRITE %d bytes, %d offset, %d disk\n",n,offset,ip->size);
  iunlock(ip);

  uint m =0;
  struct buf *bp;
  /*
  if(offset > ip->size || offset + n < offset)
  {

    ilock(ip);
    return -1;
  }*/
  if(offset + n > MAXFILE*BSIZE)
  {

    ilock(ip);
    return -1;
  }
  begin_op();
  for(uint tot=0; tot<n; tot+=m, offset+=m, buf+=m){
    
    bp = bread(ip->minor, offset/BSIZE);
    cprintf("Dev %d, minor num %d\n",bp->dev,ip->minor);
    m = min(n - tot, BSIZE - offset%BSIZE);
    memmove(bp->data + offset%BSIZE, buf, m);
    log_write(bp);
    brelse(bp);
  }
  end_op();

  if(n > 0 && offset > ip->size){
    ip->size = offset;
    iupdate(ip);
  }

  ilock(ip);
  cprintf("COMPLETED IDEWRITE\n");
  return n;
}

void
ideinit(void)
{
  int i;

  initlock(&idelock, "ide");
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

  }

  cprintf("All the disks are here\n");



  devsw[IDE].write = idewrite;
  devsw[IDE].read = ideread;
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
  //cprintf("Executing a request on disk %d>>>>>>>>>>>>>\n",b->dev);
  uint isSecondary = 0;
  uint base1 = 0x1f0;
  uint base2 = 0x3f6;
  uint disk = 0;
  if(b->dev == 2 || b->dev == 3)
  {
    base1 = 0x170;
    base2 = 0x376;
    isSecondary = 1;
    if(b->dev == 2)
    {
      disk = 0x0;
    }
    else
    {
      disk = 0x10;
    }
  }
  else if(b->dev == 0 || b->dev ==1)
  {
    if(b->dev == 0)
    {
      disk = 0x0;
    }
    else
    {
      disk = 0x10;
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
  memset(b->data, 0xAA, BSIZE);
  idewait(0,isSecondary);
  outb(base2, 0);  // generate interrupt
  outb(base1 + 2, sector_per_block);  // number of sectors
  outb(base1 + 3, sector & 0xff);
  outb(base1 + 4, (sector >> 8) & 0xff);
  outb(base1 + 5, (sector >> 16) & 0xff);
  outb(base1 + 6, 0xe0 | disk | ((sector>>24)&0x0f));
  if(b->flags & B_DIRTY){
    cprintf("Writing buffer to disk %d\n",b->dev);
    outb(base1 + 7, write_cmd);
    outsl(base1, b->data, BSIZE/4);
  } else {
    //cprintf("Reading into buffer from disk %d\n",b->dev);
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
  //cprintf("ide intr getting lock\n");
  // First queued buffer is the active request.
  acquire(curLock);



  if((b = idequeue) == 0){
    //cprintf("ide inter removing lock\n");
    release(curLock);
    return;
  }
  //cprintf("Interrupt to disk %d\n",b->dev);
  if(b->dev == 2 || b->dev == 3)
  {
    isSecondary = 1;
    base1 = 0x170;
  }
  if(b->dev == 1 || b->dev == 3)
  {
    outb(base1 + 6, 0xe0 | (1 << 4));
  }
  else if(b->dev == 0 || b->dev == 2)
  {
    outb(base1 + 6, 0xe0 | (0 << 4));
  }
  else
  {
    panic("Unkown drive");
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
  //cprintf("ide intr removing lock\n");
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
  //cprintf("Writing to disk %d<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n",b->dev);

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
  //cprintf("iderw getting lock\n");
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

  //cprintf("iderw removing lock\n");
  release(curlock);
}


