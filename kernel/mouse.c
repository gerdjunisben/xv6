#include "types.h"
#include "defs.h"
#include "x86.h"
#include "mouse.h"
#include "spinlock.h"

static struct {
    char buffer[180];
    int producerIndex;
    int consumerIndex;
    uint n;
    uint size;
} msBuffer;

static struct {
    struct spinlock spinLock;
} msLock;

void consume(char* packet, uint size)
{
    //if there isn't enough space don't bother
    if(size < 3)
    {
        return;
    }

    acquire(&msLock.spinLock);
    //cprintf("Consume got the lock\n");

    //if there isn't enough in the buffer or if we need to empty the buffer due to misalignment sleep until then 
    uint first = msBuffer.buffer[(msBuffer.consumerIndex) % msBuffer.n];
    while(msBuffer.size < 3 || ((first&0x08) == 0 || (first&0x80) || (first&0x40)))
    {
        if(msBuffer.size<3)
        {
            sleep(&msBuffer.consumerIndex,&msLock.spinLock);
        }
        else{
            msBuffer.size=0;
            msBuffer.consumerIndex=0;
            msBuffer.producerIndex=0;

            sleep(&msBuffer.consumerIndex,&msLock.spinLock);
            first = msBuffer.buffer[(msBuffer.consumerIndex) % msBuffer.n];
    
        }
    }

    // store each byte of data in order inside the pkt pointer
    for (int i = 0; i < 3; i++) {
        packet[i] = msBuffer.buffer[(msBuffer.consumerIndex++) % msBuffer.n];
    }

    msBuffer.size -= 3;
    //cprintf("Consume releases lock\n");
    release(&msLock.spinLock);

    
}

void produce(uchar msg)
{
    //ACK is being 'produced' at boot. This should handle it
    if(msg == ACK) {
        return;
    }

    //cprintf("Produce trying to get lock\n");
    acquire(&msLock.spinLock);

    // "flush" buffer if full
    if (msBuffer.size >= msBuffer.n) {
        msBuffer.consumerIndex = 0;
        msBuffer.producerIndex = 0;
        msBuffer.size = 0;
    }


    msBuffer.buffer[(msBuffer.producerIndex++) % msBuffer.n] = msg;
    msBuffer.size++;

    //wake up consumer when there is a full packet
    if(msBuffer.size >= 3) {
        //cprintf("Wake up consumer index\n");
        wakeup(&msBuffer.consumerIndex);
    }
        
    //cprintf("Produce releases lock\n");
    release(&msLock.spinLock);
}

int readmouse(char *pkt) {
    consume(pkt, 3);
    return 0;
}

void mouseinit(void)
{
    // buffer init
    initlock(&msLock.spinLock, "ms_sleeplock");
    msBuffer.producerIndex = 0;
    msBuffer.consumerIndex = 0;
    msBuffer.size = 0;
    msBuffer.n = 180;

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

        if(timeout >=100)
        {
            break;
        }
    } while(st != ACK);

    timeout = 0;

    do{
        timeout+=1;
        st = inb(MSDATAP);

        if(timeout >=100)
        {
            break;
        }
    } while(st!=TESTPASS);

    //init mousedd
    timeout = 0;
    do
    {
        timeout+=1;
        outb(MSSTATP,MSCOMMAND);
        outb(MSDATAP,MSINIT);
        st = inb(MSDATAP);

        if(timeout >=100)
        {
            break;
        }
    } while(st != ACK);
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