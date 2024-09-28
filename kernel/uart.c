// Intel 8250 serial port (UART).

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#define COM1    0x3f8
#define COM2    0x2f8

const uint comPorts[] = {COM1,COM2};

static int uart;    // is there a uart?

static struct {
  struct spinlock lock1;
  struct spinlock lock2;
} com;





#define BACKSPACE 0x100

void
uartputc(int c,int comNumber)
{
  int i;
  uint port =comPorts[comNumber];

  if(!uart)
    return;
  // On my test system, Bochs (running at 1M instr/sec)
  // requires between 500 and 1000 iterations to avoid
  // output overrun, as indicated by log messages in the
  // Bochs log file.  This is with microdelay() having an
  // empty body and ignoring its argument.
  for(i = 0; i < 1000 && !(inb(port+5) & 0x20); i++)
    microdelay(10);
  if(c == BACKSPACE){
    outb(port+0, '\b');
    for(i = 0; i < 1000 && !(inb(port+5) & 0x20); i++)
      microdelay(10);
    outb(port+0, ' ');
    for(i = 0; i < 1000 && !(inb(port+5) & 0x20); i++)
      microdelay(10);
    outb(port+0, '\b');
  } 
  else if(c=='\n') {
    outb(port+0, '\r');
    outb(port+0, '\n');
  }
  else
  {
     outb(port+0, c);
  }
}


static int
uartgetc(int comNumber)
{
  uint port =comPorts[comNumber];
  if(!uart)
    return -1;
  if(!(inb(port+5) & 0x01))
    return -1;
  return inb(port+0);
}

#define INPUT_BUF 128
struct comBuffer{
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
};

struct comBuffer comBuffers[2];

#define C(x)  ((x)-'@')  // Control-x

void
uartintr(int comNumber)
{
  //cprintf("uartintr\n");
  int c, doprocdump = 0;
  struct spinlock* lock;
  if(comNumber == 1)
  {
    lock = &(com.lock1);
  }
  else
  {
    lock = &(com.lock2);
  }

  acquire(&(*lock));
  comNumber-=1;
  while((c = uartgetc(comNumber)) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'):  // Kill line.
      while(comBuffers[comNumber].e != comBuffers[comNumber].w &&
            comBuffers[comNumber].buf[(comBuffers[comNumber].e-1) % INPUT_BUF] != '\n'){
        comBuffers[comNumber].e--;
        uartputc(BACKSPACE,comNumber);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if(comBuffers[comNumber].e != comBuffers[comNumber].w){
        comBuffers[comNumber].e--;
        uartputc(BACKSPACE,comNumber);
      }
      break;
    default:
      if(c != 0 && comBuffers[comNumber].e-comBuffers[comNumber].r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        comBuffers[comNumber].buf[comBuffers[comNumber].e++ % INPUT_BUF] = c;
        uartputc(c,comNumber);
        if(c == '\n' || c == C('D') || comBuffers[comNumber].e == comBuffers[comNumber].r+INPUT_BUF){
          comBuffers[comNumber].w = comBuffers[comNumber].e;
          wakeup(&comBuffers[comNumber].r);
        }
      }
      break;
    }
  }
  release(&(*lock));
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }
}

int
uartread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  int comNumber = ip->minor;
  struct spinlock* lock;
  if(comNumber == 1)
  {
    lock = &(com.lock1);
  }
  else 
  {
    lock = &(com.lock2);
  }
  comNumber-=1;
  target = n;
  acquire(&(*lock));
  while(n > 0){
    while(comBuffers[comNumber].r == comBuffers[comNumber].w){
      if(myproc()->killed){
        release(&(*lock));
        ilock(ip);
        return -1;
      }
      sleep(&comBuffers[comNumber].r, &(*lock));
    }
    c = comBuffers[comNumber].buf[comBuffers[comNumber].r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        comBuffers[comNumber].r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&(*lock));
  ilock(ip);

  return target - n;
}

int
uartwrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  struct spinlock* lock;
  if(ip->minor == 1)
  {
    lock = &(com.lock1);
  }
  else 
  {
    lock = &(com.lock2);
  }
  acquire(&(*lock));
  for(i = 0; i < n; i++)
    uartputc(buf[i] & 0xff, (ip->minor - 1));
  release(&(*lock));
  ilock(ip);

  return n;
}

void
uartinit(int comNumber)
{
  char *p;
  if(comNumber == 1)
  {
    cprintf("Starting up COM1\n");
    initlock(&com.lock1,"COM1");
  }
  else
  {
    cprintf("Starting up COM2\n");
    initlock(&com.lock2,"COM2");
  }

  int port = comPorts[comNumber-1];

  // Turn off the FIFO
  outb(port+2, 0);

  // 9600 baud, 8 data bits, 1 stop bit, parity off.
  outb(port+3, 0x80);    // Unlock divisor
  outb(port+0, 115200/9600);
  outb(port+1, 0);
  outb(port+3, 0x03);    // Lock divisor, 8 data bits.
  outb(port+4, 0);
  outb(port+1, 0x01);    // Enable receive interrupts.

  // If status is 0xFF, no serial port.
  if(inb(port+5) == 0xFF)
    return;
  uart = 1;

  // Acknowledge pre-existing interrupt conditions;
  // enable interrupts.
  inb(port+2);
  inb(port+0);
  if(comNumber == 1)
  {
    ioapicenable(IRQ_COM1, 0);
  }
  else if(comNumber == 2)
  {
    ioapicenable(IRQ_COM2, 0);
  }

  devsw[COM].write = uartwrite;
  devsw[COM].read = uartread;

  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p,(comNumber-1));
}
