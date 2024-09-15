
#include "kernel/types.h"
#include "user.h"

void movement_loop(char *msg, uint sign_bit_position, uint direction, uint delta_byte) {
    printf(0, msg);

    char pkt[3];
    while (1) {
        readmouse(pkt);

        if (((pkt[0] >> sign_bit_position) & 0x1) != direction)
            continue;
        
        if (pkt[delta_byte] != 0x0)
            break;
    }

    printf(0, "mouse detected\n\n");
}

int main(void)
{
    printf(0, "welcome to our test program for the mouse driver\n\nto start, we are going to ask you to move your mouse in different directions\n\n");

    movement_loop("please move your mouse to the right\n", 4, 0, 1);
    movement_loop("please move your mouse to the left\n", 4, 1, 1);
    movement_loop("please move your mouse up\n", 5, 0, 2);
    movement_loop("please move your mouse down\n", 5, 1, 2);

    printf(0, "thanks, now you will enter an infinite loop that will constantly print mouse information\n\n");
    printf(0, "to exit the loop, you need to press the mouse buttons in the following sequence: left left right left\n\n");
    printf(0, "please, click the left mouse button to start the loop\n");

    char pkt[3];

    while(1) {
        readmouse(pkt);

        if ((pkt[0] & 0x1) != 0 && (pkt[0]&0x2)==0 && (pkt[0]&0x4)==0)
        {
            for(int i = 0;i<3;i++)
            {
                printf(0,"Part %d:%x\n",i,pkt[i]);
            }
            break;
        }
    }

    //A tolerance of 20 packets for exit sequence
    int packet_timeout = 0;

    // when first left click detected, OR with 0x1 and << 1. Therefore, bit-1 is now set to 1 (0x2)
    // when second left click detected, OR with 0x1 and << 1. Therefore bits 2 and 1 are now set to (0x6)
    // when right click detected. If sequence_flags value is 0x6, then OR with 0x1 and << 1. Now bits 3, 2, and 1 are set (0xE)
    // for last, left click, if sequence_flags value is 0xE, then exit the loop. else, don't do anything
    // if packet_timeout reaches 20, sequence_flags is set to 0x0 again.
    uchar sequence_flags = 0x0;

    while (1) {
        readmouse(pkt);

        uchar intro_msg = 0;

        if (pkt[0] & 0x1) {

            if (!intro_msg) {
                printf(0,"package information: \n");
                intro_msg = 1;
            }

            printf(0, "left-button pressed\n");

            if (sequence_flags == 0xE)
                break;

            else {
                sequence_flags = (sequence_flags || 0x1) << 1;
                printf(0, "sequence_flags set to: 0x%x\n", sequence_flags);
            }    
        }

        if (pkt[0] & 0x4) {
            
            if (!intro_msg) {
                printf(0,"package information: \n");
                intro_msg = 1;
            }

            printf(0, "right-button pressed\n");

            if (sequence_flags == 0x6) {
                sequence_flags = (sequence_flags || 0x1) << 1;
                printf(0, "sequence_flags set to: 0x%x\n", sequence_flags);
            }
        }

        if (pkt[0] & 0x4) {

            if (!intro_msg) {
                printf(0,"package information: \n");
                intro_msg = 1;
            }

            printf(0, "middle-button pressed\n");
        }

        if (pkt[1]) {

            if (!intro_msg) {
                printf(0,"package information: \n");
                intro_msg = 1;
            }

            if (pkt[0] & 0x10) {
                printf(0, "mouse moved to the left\n");
            }
            
            else {
                printf(0, "mouse moved to the right\n");
            }
        }

        if (pkt[2]) {

            if (!intro_msg) {
                printf(0,"package information: \n");
                intro_msg = 1;
            }

            if (pkt[0] & 0x20) {
                printf(0, "mouse moved down\n");
            }
            
            else {
                printf(0, "mouse moved up\n");
            }
        }

        if (sequence_flags) {
            packet_timeout += 1;
            printf(0, "package_timeout: %d\n", packet_timeout);
        }

        if (packet_timeout >= 20) {
            sequence_flags = 0x0;
            packet_timeout = 0;
            printf(0, "\nsequence_flags reset to: 0x%x\n", sequence_flags);
            printf(0, "packet_timeout reset to: %d\n", packet_timeout);
        }

        printf(0, "\n");
    }

    return 0;
}