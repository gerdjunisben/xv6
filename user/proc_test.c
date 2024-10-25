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


void addSum(uint i)
{
    sum+=i;
}


void compute(float j)
{
    for(int i = 0;i<10000 * j;i++)
    {
        addSum(i);
        mulSum(i);
        addSum(i);
        mulSum(i);
        mulSum(i);
        mulSum(i);
        
    }
}


int main(int argc, char *argv[])
{
   uint sum = 1;
   uint pid;
   int i;
   for(i = 1;i<5;i++)
   {
     pid = fork();
     sleep(5);
     if(pid ==0)
     {
        break;
     }
   }
   for(int j = 0;j<=4000;j++)
    {
       compute(i);


        sleep(1);
    }
    if(pid!=0)
    {
        while(wait()!=-1);
    }
    printf(0,"%d\n",sum);
    exit();
}