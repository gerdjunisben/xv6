#include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"

static struct {
  int buff[10];
  int size;
  int cur;
} keyBuffer;

int buffgetc(void)
{
  return keyBuffer.buff[keyBuffer.cur++];
}


int
kbdgetc(void)
{
  static uint shift;
  static uchar *charcode[4] = {
    normalmap, shiftmap, ctlmap, ctlmap
  };
  uint st, data, c;

  st = inb(KBSTATP);
  if((st & KBS_DIB) == 0)
    return -2;
  if((st & msOrKbd) != 0)
  {
    inb(KBDATAP);
    return -1;
  }
  data = inb(KBDATAP);

  if(data == 0xE0){
    shift |= E0ESC;
    return 0;
  } else if(data & 0x80){
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);
    return 0;
  } else if(shift & E0ESC){
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
    shift &= ~E0ESC;
  }

  shift |= shiftcode[data];
  shift ^= togglecode[data];
  c = charcode[shift & (CTL | SHIFT)][data];
  if(shift & CAPSLOCK){
    if('a' <= c && c <= 'z')
      c += 'A' - 'a';
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A';
  }
  return c;
}


void
kbdintr(void)
{
  //cprintf("kbd ISR");
  int c;
  keyBuffer.size = 0;
  keyBuffer.cur = 0;

  //cprintf("Console Lock");
  while((c = kbdgetc()) >= -1){
    if(keyBuffer.size < 9) // (c = kbggetc & msOrKbd) == 0
    {
      //cprintf("key board input. adding data to buffer");
      if (c!= -1)
      {
        keyBuffer.buff[keyBuffer.size++] = c;
      }
    }
  }
  keyBuffer.buff[keyBuffer.size] = -1;
  // cprintf("entering consoleintr");
  consoleintr(buffgetc);
}