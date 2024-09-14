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

void consume(char* packet, uint size)
{
    acquire(&msBuffer.spinLock);
    //cprintf("Consuming on pointer %p of size %d\n", packet, size);

    //if there isn't enough space don't bother
    if(size < 3)
    {
        //cprintf("Unsufficient Pointer Size\n");
        release(&msBuffer.spinLock);
        return;
    }

    //if there isn't enough in the buffer sleep until then
    if(msBuffer.size < 3)
    {
        //cprintf("Not enough data on buffer\nCONSUMER SLEEPING\n");
        release(&msBuffer.spinLock);
        acquiresleep(&msBuffer.sleepLock);
    }

    else {
        //cprintf("Full 3-byte data detected. Consuming...\n");
        for (int i = 0; i < 3; i++) {
            //cprintf("Storing byte 0x%x from buffer index %d to packet index %d\n", msBuffer.buffer[msBuffer.consumerIndex % msBuffer.n], msBuffer.consumerIndex % msBuffer.n, i);
            packet[i] = msBuffer.buffer[(msBuffer.consumerIndex++) % msBuffer.n];
        }
            
        msBuffer.size -= 3;

        release(&msBuffer.spinLock);
    }

    
}

void produce(uchar msg)
{
    acquire(&msBuffer.spinLock);
    
    //cprintf("Producing mouse packet 0x%x\n", msg);
    //ACK is being 'produced' at boot. This should handle it
    if(msg == ACK) {
        //cprintf("ACK\nDISCARDED\n");
        release(&msBuffer.spinLock);
        return;
    }

    // "flush" buffer
    if (msBuffer.size >= msBuffer.n) {
        msBuffer.consumerIndex = 0;
        msBuffer.producerIndex = 0;
        msBuffer.size = 0;
    }

    //sanity check for first byte
    if (msBuffer.producerIndex % 3 == 0) {
        //cprintf("Supposed to be First Byte of a new 3-Byte Sequence\n");

        if ((msg&0x8) == 0) {

            //cprintf("Invalid First Byte\nDISCARD\n");

            release(&msBuffer.spinLock);
            return;

        }

        else {
            //cprintf("Valid First-Byte, ");
        }
    }

    //cprintf("Storing byte 0x%x at index %d\n", msg, msBuffer.producerIndex);
    msBuffer.buffer[(msBuffer.producerIndex++) % msBuffer.n] = msg;
    msBuffer.size++;

    //wake up consumer when there is a full packet
    if(msBuffer.size >= 3) {
        //cprintf("3-byte sequence detected.\nAWAKENING CONSUMER\n");
        releasesleep(&msBuffer.sleepLock);
    }
        
    release(&msBuffer.spinLock);
}

int readmouse(char *pkt) {
    consume(pkt, 3);
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

    //enable the mouse
    outb(MSSTATP,ENABLEMS);

    //set config
    uint st;
    outb(MSSTATP, GETCOMPAQ);
    st = inb(MSDATAP);
    st = (st|0x2) & (~0x20);
    outb(MSSTATP,SETCOMPAQ);
    outb(MSDATAP,st);


    //reset mouse
    int timeout = 0;
    do
    {
        timeout+=1;
        outb(MSSTATP,MSCOMMAND);
        outb(MSDATAP,RESETMS);
        st = inb(MSDATAP);
        if(st != ACK)
        {
            //cprintf("No ACK?\n");
        }
        else
        {
            //cprintf("ACK\n");
        }
        //cprintf("Ticks since start %d\n",timeout);
        if(timeout >=100)
        {
            //cprintf("failure timeout\n");
            break;
        }
    }while(st != ACK);

    timeout = 0;
    do{
        timeout+=1;
        st = inb(MSDATAP);
        if(st != TESTPASS)
        {
            //cprintf("FAILED\n");
        }
        else
        {
            //cprintf("PASSED\n");
        }   
        //cprintf("Ticks since start %d\n",timeout);
        if(timeout >=100)
        {
            //cprintf("failure timeout\n");
            break;
        }
    }while(st!=TESTPASS);

    //init mousedd
    timeout = 0;
    do
    {
        timeout+=1;
        outb(MSSTATP,MSCOMMAND);
        outb(MSDATAP,MSINIT);
        st = inb(MSDATAP);
        if(st != ACK)
        {
            //cprintf("No ACK?\n");
        }
        else
        {
            //cprintf("ACK\n");
        }
        //cprintf("Ticks since start %d\n",timeout);
        if(timeout >=100)
        {
            //cprintf("failure timeout\n");
            break;
        }
    }while(st != ACK);
}

void mouseintr(void)
{
    uint st = inb(MSSTATP);

    //check wether there is data to read and if it comes from the mouse
    if ((st&0x1)==0 || (st&0x20) == 0)
        return;

    uchar data = inb(MSDATAP);
    produce(data);
}