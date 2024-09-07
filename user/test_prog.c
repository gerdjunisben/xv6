
#include "kernel/types.h"
#include "user.h"

int main(void)
{
    char pkt[3];

    readmouse(pkt);

    for(int i =0;i<3;i++)
    {
        printf(0,"Pkt %d, data %x\n",i,pkt[i]);
    }

    return 0;
}