#include "types.h"
#include "defs.h"
#include "x86.h"
#include "mouse.h"


void mouseinit(void)
{
    //this is where we init buffer :)
    
    //whenever we read or write add a timeout

    //empty buffer/sanity check
    inb(MSDATAP);
    inb(MSSTATP);

    //enable the port
    outb(MSSTATP,ENABLEMS);

    //set config
    uint st;
    outb(MSSTATP, GETCOMPAQ);
    st = inb(MSDATAP);
    st = (st|0x2) & (~0x20);
    outb(MSSTATP,SETCOMPAQ);
    outb(MSDATAP,st);


    //reset mouse
    outb(MSSTATP,MSCOMMAND);
    outb(MSDATAP,RESETMS);

    st = inb(MSDATAP);
    if(st != ACK)
    {
        cprintf("No ACK?");
    }
    else
    {
        cprintf("ACK");
    }

    st = inb(MSDATAP);
    if(st != TESTPASS)
    {
        cprintf("FAILED");
    }
    else
    {
        cprintf("PASSED");
    }

    //init mousedd
    while(st != ACK)
    {
        outb(MSSTATP,MSCOMMAND);
        outb(MSDATAP,MSINIT);
        st = inb(MSDATAP);
        if(st != ACK)
        {
            cprintf("No ACK?");
        }
        else
        {
            cprintf("ACK");
        }
    }
}

void mouseintr(void)
{
    uint st = inb(MSSTATP);
    if((st&0x1)==0)
    {
        return;
    }
    else
    {
        uint data = inb(MSDATAP);
        cprintf("Mouse data packet %x\n",data);
    }
}