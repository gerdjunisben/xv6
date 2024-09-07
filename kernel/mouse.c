#include "types.h"
#include "defs.h"
#include "x86.h"
#include "mouse.h"
#include "spinlock.h"
#include "sleeplock.h"

static struct {
    char buffer[9];
    int producerIndex;
    int consumerIndex;
    uint n;
    uint size;
    struct sleeplock sleepLock;
    struct spinlock spinLock;
} msBuffer;

void consume(char* packet,uint size)
{
    acquire(&msBuffer.spinLock);

    //if there isn't enough space don't bother
    if(size<3)
    {
        return;
    }

    //if there isn't enough in the buffer sleep until then
    if(msBuffer.size<3)
    {
        acquiresleep(&msBuffer.sleepLock);
    }

    //weird order align it trust me
    packet[2] = msBuffer.buffer[(msBuffer.consumerIndex++) % msBuffer.n];
    packet[0] = msBuffer.buffer[(msBuffer.consumerIndex++) % msBuffer.n];
    packet[1] = msBuffer.buffer[(msBuffer.consumerIndex++) % msBuffer.n];
    
    msBuffer.size -= 3;

    release(&msBuffer.spinLock);
}

void produce(char msg)
{
    acquire(&msBuffer.spinLock);
    
    //if full discard
    if(msBuffer.size != 9)
    {
        //check if it's a mouse packet
        msBuffer.buffer[(msBuffer.producerIndex++) % msBuffer.n] = msg;
        msBuffer.size++;

        //wake up consumer when there is a full packet
        if(msBuffer.size>=3)
            releasesleep(&msBuffer.sleepLock);
    }

    release(&msBuffer.spinLock);
}

int readmouse(char *pkt) {

    cprintf("Pointer: %p", pkt);

    consume(pkt,3);

    return 0;
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
    }
}