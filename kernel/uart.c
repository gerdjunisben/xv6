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

static int uart;    // is there a uart?

static struct {
  struct spinlock lock;
} com;





#define BACKSPACE 0x100

void
uartputc(int c)
{
  int i;

  if(!uart)
    return;
  // On my test system, Bochs (running at 1M instr/sec)
  // requires between 500 and 1000 iterations to avoid
  // output overrun, as indicated by log messages in the
  // Bochs log file.  This is with microdelay() having an
  // empty body and ignoring its argument.
  for(i = 0; i < 1000 && !(inb(COM1+5) & 0x20); i++)
    microdelay(10);
  if(c == BACKSPACE){
    outb(COM1+0, '\b');
    for(i = 0; i < 1000 && !(inb(COM1+5) & 0x20); i++)
      microdelay(10);
    outb(COM1+0, ' ');
    for(i = 0; i < 1000 && !(inb(COM1+5) & 0x20); i++)
      microdelay(10);
    outb(COM1+0, '\b');
  } else
  {
     outb(COM1+0, c);
  }
}


static int
uartgetc(void)
{
  if(!uart)
    return -1;
  if(!(inb(COM1+5) & 0x01))
    return -1;
  return inb(COM1+0);
}

#define INPUT_BUF 128
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} comInput;

#define C(x)  ((x)-'@')  // Control-x

void
uartintr(void)
{
  //cprintf("uartintr\n");
  int c, doprocdump = 0;

  acquire(&com.lock);
  while((c = uartgetc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'):  // Kill line.
      while(comInput.e != comInput.w &&
            comInput.buf[(comInput.e-1) % INPUT_BUF] != '\n'){
        comInput.e--;
        uartputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if(comInput.e != comInput.w){
        comInput.e--;
        uartputc(BACKSPACE);
      }
      break;
    default:
      if(c != 0 && comInput.e-comInput.r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        comInput.buf[comInput.e++ % INPUT_BUF] = c;
        uartputc(c);
        if(c == '\n' || c == C('D') || comInput.e == comInput.r+INPUT_BUF){
          comInput.w = comInput.e;
          wakeup(&comInput.r);
        }
      }
      break;
    }
  }
  release(&com.lock);
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
  target = n;
  acquire(&com.lock);
  while(n > 0){
    while(comInput.r == comInput.w){
      if(myproc()->killed){
        release(&com.lock);
        ilock(ip);
        return -1;
      }
      sleep(&comInput.r, &com.lock);
    }
    c = comInput.buf[comInput.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        comInput.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&com.lock);
  ilock(ip);

  return target - n;
}

int
uartwrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&com.lock);
  for(i = 0; i < n; i++)
    uartputc(buf[i] & 0xff);
  release(&com.lock);
  ilock(ip);

  return n;
}

void
uartinit(void)
{
  char *p;
  initlock(&com.lock,"COM1");

  

  // Turn off the FIFO
  outb(COM1+2, 0);

  // 9600 baud, 8 data bits, 1 stop bit, parity off.
  outb(COM1+3, 0x80);    // Unlock divisor
  outb(COM1+0, 115200/9600);
  outb(COM1+1, 0);
  outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
  outb(COM1+4, 0);
  outb(COM1+1, 0x01);    // Enable receive interrupts.

  // If status is 0xFF, no serial port.
  if(inb(COM1+5) == 0xFF)
    return;
  uart = 1;

  // Acknowledge pre-existing interrupt conditions;
  // enable interrupts.
  inb(COM1+2);
  inb(COM1+0);
  ioapicenable(IRQ_COM1, 0);

  cprintf("setting write and read for uart\n");
  devsw[COM].write = uartwrite;
  devsw[COM].read = uartread;

  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p);
}
