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

#define COMCount  2
#define COM1    0x3f8
#define COM2    0x2f8
#define BACKSPACE 0x100

#define C(x)  ((x)-'@')  // Control-x

#define INPUT_BUF 128
struct comBuffer{
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
};

static int uart;    // is there a uart?

//these are the locks for com 1 and 2 respectively
static struct {
  struct spinlock lock1;
  struct spinlock lock2;
} com;

//this has a buffer for each com
struct comBuffer comBuffers[COMCount];

//this is for quick access of the port number
const uint comPorts[] = {COM1,COM2};


void
uartputc(int c,int comNumber)
{
  //they do delays for bosch doesn't really matter to us but let's do it anyway
  int i;

  if(!uart)
    return;
  uint port =comPorts[comNumber];
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
    microdelay(10);
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
  if(!uart)
    return -1;
  uint port =comPorts[comNumber];
  if(!(inb(port+5) & 0x01))
    return -1;
  return inb(port+0);
}



void
uartintr(int comNumber)
{
  //cprintf("uartintr\n");
  int c, doprocdump = 0;
  struct spinlock* lock;
  if(comNumber == 1)
  {
    //cprintf("COM1 intr\n");
    lock = &(com.lock1);
  }
  else if(comNumber == 2)
  {
    //cprintf("COM2 intr\n");
    lock = &(com.lock2);
  }
  else
  {
    procdump();
    return;
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

  //normally I'd make this a loop but I'm pretty sure I save in overhead,
  //may lose it in other places but the thought is what matter
  if(comNumber == 1)
  {
    //cprintf("COM1 read\n");
    lock = &(com.lock1);
  }
  else if(comNumber == 2)
  {
    //cprintf("COM2 read\n");
    lock = &(com.lock2);
  }
  else
  {
    return -1;
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
    //cprintf("COM1 write\n");
    lock = &(com.lock1);
  }
  else if(ip->minor == 2)
  {
    //cprintf("COM2 write\n");
    lock = &(com.lock2);
  }
  else
  {
    return -1;
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
    //cprintf("Starting up COM1\n");
    initlock(&com.lock1,"COM1");
  }
  else if(comNumber == 2)
  {
    //cprintf("Starting up COM2\n");
    initlock(&com.lock2,"COM2");
  }
  else
  {
    //cprintf("Starting invalid COM\n");
    return;
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

  //at this point it's impossible for it to be any COM but a valid one so no else
  if(comNumber == 1)
  {
    ioapicenable(IRQ_COM1, 0);
  }
  else if(comNumber == 2)
  {
    ioapicenable(IRQ_COM2, 0);
  }
  

  //I see no harm in setting it multiple times but I will keep an eye on this
  devsw[COM].write = uartwrite;
  devsw[COM].read = uartread;

  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p,(comNumber-1));
}
