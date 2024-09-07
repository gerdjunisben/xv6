
#include "kernel/types.h"
#include "user.h"

int main(void)
{
    char pkt[3];

    for (int i = 0; i < 100; i++) {
        readmouse(pkt);
        printf(0, "Consumed Packet Successfully\n");
        printf(0, "First Byte: 0x%x\n", pkt[0]);
        printf(0, "Second Byte: 0x%x\n", pkt[1]);
        printf(0, "Third Byte: 0x%x\n", pkt[2]);
        printf(0, "Third Byte: 0x%x\n", pkt[2]);
        printf(0, "LMB: %d\n", pkt[0] & 0x1);
        printf(0, "RMB: %d\n", pkt[0] & 0x2);
        printf(0, "MMB: %d\n", pkt[0] & 0x4);
        printf(0, "Delta-X sign: %d\n", pkt[0] & 0x10);
        printf(0, "Delta-X value: %d\n", (uint) pkt[1]);
        printf(0, "Delta-Y sign: %d\n", pkt[0] & 0x20);
        printf(0, "Delta-Y value: %d\n", (uint) pkt[2]);
    }

    return 0;
}