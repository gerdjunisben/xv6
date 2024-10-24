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


void compute(uint j)
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
   for(int j = 0;j<=4000;j++)
    {
       compute(1);


        sleep(1);
    }
    printf(0,"%d\n",sum);
    exit();
}