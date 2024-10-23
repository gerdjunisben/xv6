#include "kernel/types.h"
#include "user.h"




static uint sum = 1;


void setSum(uint i)
{
    sum = i;
}

void mulSum(uint i)
{
    sum*=i;
}


int main(int argc, char *argv[])
{
    /*
    int pid;
    if((pid = fork()) == 0 )
    {
        for(int j = 0;j<1000000;j++)
        {
            float i;
            for(i = .25;i<214748364;i*=3.2)
            {
                printf(0,"child: %d\n",i);
            }
            for(;i>=1;i/=1.27);
        }
        exit();
    }
    else
    {
        for(int j = 0; j < 1000000; j++) {
            float i;
            for(i = .25; i < 214748364; i*=1.27) {
                printf(0,"parent:%d\n",i);
                
                if (i % 1000 == 0) {
                    sleep(1000);
                }
            }
            for(;i >= 1; i/=1.27);
        }
        wait();c
        exit();
    }*/
   uint sum = 1;
   for(int j = 0;j<=3000;j++)
    {
        for (int i = 1; i <= 53060 ; i++) {
            //the methods stop optimization from occuring cause gcc has no clue what they do
            mulSum(i);
        }

        sleep(1);
        setSum(1);
    }
    printf(0,"%d\n",sum);
    exit();
}