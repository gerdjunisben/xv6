#include "types.h"
#include "defs.h"
#include "x86.h"
#include "mouse.h"
#include "spinlock.h"
#include "sleeplock.h"

static struct {
    uint buffer[9];
    int producerIndex;
    int consumerIndex;
    uint n;
    uint size;
    struct sleeplock sleepLock;
    struct spinlock spinLock;
} msBuffer;

void consume(uint* packet,uint size)
{
    acquire(&msBuffer.spinLock);

    if(size<3)
    {
        return;
    }

    if(msBuffer.size<3)
    {
        acquiresleep(&msBuffer.sleepLock);
    }

    packet[0] = msBuffer.buffer[(msBuffer.consumerIndex++) % msBuffer.n];
    packet[1] = msBuffer.buffer[(msBuffer.consumerIndex++) % msBuffer.n];
    packet[2] = msBuffer.buffer[(msBuffer.consumerIndex++) % msBuffer.n];
    
    msBuffer.size -= 3;

    release(&msBuffer.spinLock);
}

void produce(uint msg)
{
    acquire(&msBuffer.spinLock);
    
    if(msBuffer.size != 9)
    {
        msBuffer.buffer[(msBuffer.producerIndex++) % msBuffer.n] = msg;
        msBuffer.size++;

        if(msBuffer.size>=3)
            releasesleep(&msBuffer.sleepLock);
    }

    release(&msBuffer.spinLock);
}

void mouseinit(void)
{
    //this is where we init buffer :)
    initsleeplock(&msBuffer.sleepLock, "ms_sleeplock");
    initlock(&msBuffer.spinLock, "ms_sleeplock");
    msBuffer.producerIndex = 0;
    msBuffer.consumerIndex = 0;
    msBuffer.size = 0;
    msBuffer.n = 9;
    
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
        produce(data);

        if(msBuffer.size>=3)
        {
            uint res[3];
            consume(res,3);
            for(int i =0;i<3;i++)
            {
                cprintf("Packet %d is %x ",i,res[i]);
            }
            cprintf("\n");
        }
    }
}