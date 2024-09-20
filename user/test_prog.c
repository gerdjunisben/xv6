#include "kernel/types.h"
#include "user.h"
#define xSens 1000
#define ySens 1000

void movement_loop(char *msg,  uint direction, int* xPos, int* yPos) {
    printf(0, msg);
    char pkt[3];
    while (1) {
        readmouse(pkt);

        if((pkt[0]&0x10))
        {
            *xPos += (pkt[1] | 0xFFFFFF00);
        }
        else
        {
            *xPos += pkt[1];
        }

        if((pkt[0]&0x20))
        {
            *yPos +=(pkt[2] | 0xFFFFFF00);
        }
        else
        {
            *yPos +=pkt[2];
        }

        if(direction == 0 && *xPos>xSens)
        {

            printf(0,"Right movement accepted, good work!\n");
            break;
        }
        else if(direction == 1 && *xPos < (xSens *-1))
        {
            printf(0,"Left movement accepted, nice job!\n");
            break;
        }
        else if(direction == 2 && *yPos>ySens)
        {
            printf(0,"Up movement accepted, keep it up!\n");
            break;
        }
        else if(direction == 3 && *yPos < (ySens * -1))
        {
            printf(0,"Down movement acceptable, that's a wrap!\n");
            break;
        }
        else if((pkt[0]&0x1)==1)
        {
            printf(0,"Mouse postition x : %d, y : %d\n",*xPos,*yPos);
        }
    }

    printf(0, "mouse detected\n\n");
}

int main(void)
{
    int xPos = 0;
    int yPos = 0;
    printf(0, "welcome to our test program for the mouse driver\n\nto start, we are going to ask you to move your mouse in different directions\nYou may press left click during the test to print your current position and you position carries over between tests\n\n");

    movement_loop("please move your mouse to the right\n", 0,&xPos, &yPos);
    movement_loop("please move your mouse to the left\n", 1,&xPos, &yPos);
    movement_loop("please move your mouse up\n",  2,&xPos, &yPos);
    movement_loop("please move your mouse down\n", 3,&xPos, &yPos);

    printf(0, "thanks, now you will enter an infinite loop that will constantly print mouse information\n\n");
    printf(0, "to exit the loop, you need to press the mouse buttons in the following sequence: left left right left\n\n");
    printf(0, "please, click the left mouse button to start the loop\n");

    char pkt[3];

    while(1) {
        readmouse(pkt);

        // extra sanity checks to avoid input pollusion by missalignment
        if ((pkt[0] & 0x1) != 0 && (pkt[0]&0x2)==0 && (pkt[0]&0x4)==0)
        {
            break;
        }
    }

    //A tolerance of 20 packets for exit sequence
    int packet_timeout = 0;

    // when first left click detected, << 1 and OR with 0x1. Therefore, bit-0 is now set to 1 (0x1)
    // when second left click detected, << 1 and OR with 0x1. Therefore bits 1 and 0 are now set to (0x3)
    // when right click detected. If sequence_flags value is 0x3, then << 1 and OR with 0x1. Now bits 2, 1, and 0 are set (0x7)
    // for last, left click, if sequence_flags value is 0x7, then exit the loop. else, don't do anything
    // if packet_timeout reaches 5, sequence_flags is set to 0x0 again.
    uchar sequence_flags = 0x0;

    printf(0, "\n");

    while (1) {
        readmouse(pkt);

        //  empty packet
        if (pkt[0] == 0x8 && !pkt[1] && !pkt[2]) {
            continue;
        }

        //printf(0,"package information: \n");
        //printf(0, "part 1: 0x%x\npart 2: 0x%x\npart 3: 0x%x\n", pkt[0], pkt[1], pkt[2]);
        printf(0,"mouse information: \n");
        if((pkt[0]&0x10))
        {
            xPos += (pkt[1] | 0xFFFFFF00);
        }
        else
        {
            xPos += pkt[1];
        }

        if((pkt[0]&0x20))
        {
            yPos +=(pkt[2] | 0xFFFFFF00);
        }
        else
        {
            yPos +=pkt[2];
        }
        printf(0,"Mouse postition x : %d, y : %d\n",xPos,yPos);
        // left mouse button
        if (pkt[0] & 0x1) {

            printf(0, "left button pressed\n");

            if (sequence_flags == 0x7)
                break;

            else if (sequence_flags == 0x0 || sequence_flags == 0x1) {
                sequence_flags = sequence_flags << 1;
                sequence_flags |= 0x1;
            }    
        }

        // right mouse button
        if (pkt[0] & 0x2) {

            printf(0, "right-button pressed\n");

            if (sequence_flags == 0x3) {
                sequence_flags =  sequence_flags << 1;
                sequence_flags |= 0x1;
            }
        }

        //middle button
        if (pkt[0] & 0x4) {
            printf(0, "middle-button pressed\n");
        }

        //movement in X
        if (pkt[1]) {

            if (pkt[0] & 0x10) {
                printf(0, "mouse moved to the left\n");
            }
            
            else {
                printf(0, "mouse moved to the right\n");
            }
        }

        //movement in Y
        if (pkt[2]) {

            if (pkt[0] & 0x20) {
                printf(0, "mouse moved down\n");
            }
            
            else {
                printf(0, "mouse moved up\n");
            }
        }

        if (sequence_flags) {
            packet_timeout += 1;
        }

        if (packet_timeout >= 10) {
            sequence_flags = 0x0;
            packet_timeout = 0;
        }
        printf(0, "\n");
    }

    printf(0, "\n\nthank you for testing our driver\n\n");
    exit();
    return 0;
}