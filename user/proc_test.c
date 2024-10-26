#include "kernel/types.h"
#include "user.h"

static int sum = 1;

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
    for(int i = 0;i < 10000 * j; i++)
    {
        addSum(i);
        mulSum(i);
        addSum(i);
        mulSum(i);
        mulSum(i);
        mulSum(i);     
    }
}

void cpu_mode(uint pid, int num_iter) {
    for(int j = 0; j < num_iter; j++) {
       compute(j);
       sleep(5);
    }
}

void io_mode(uint pid, int num_iter) {
    for(int j = 0; j < num_iter; j++) {
       compute(j);
       sleep(70);
    }
}

void fifty_fifty_mode(uint pid, int num_iter) {
    for(int j = 0; j < num_iter; j++) {
       compute(j);
       sleep(50);
    }
}

void mode_router(int mode, int pid, int num_iter) {
    if (mode == 0) {
        cpu_mode(pid, num_iter);
    }

    else if (mode == 1) {
        io_mode(pid, num_iter);
    }

    else {
        fifty_fifty_mode(pid, num_iter);
    }

    // parent process
    if (pid == -9) {
        printf(0,"> parent process finished\n> sum = %d\n", sum);
        exit();
    }
    // child process
    else {
        printf(0, "> process %d exited\n> sum = %d\n", pid, sum);
        exit();
    }
}

int validargs(int argc, char *argv[]) {
    if (argc < 4)
        return -1;

    int num_child = atoi(argv[1]);
    int num_iter = atoi(argv[2]);
    int mode = atoi(argv[3]);

    if (num_child < 0 || num_child > 20 || num_iter < 0 || num_iter > 200 || mode < 0 || mode > 2)
        return -1;
    
    return 0;
}

/*
 * 1st arg = num children to fork
 * 2nd arg = num iterations
 * 3rd arg = mode [0: cpu burst mode, 1: i/o burst mode, 2: 50/50 mode]
*/

int main(int argc, char *argv[])
{

   if (validargs(argc, argv) == -1) {
    printf(0, "> invalid args\n> ussage:\n  1st arg = num of children to fork [0-20]\n  2nd arg = num iterations [0-200]\n  3rd arg = mode [0: cpu burst mode, 1: i/o burst mode, 2: 50/50 mode]\n");
    exit();
   }

   int pid = -1;
   int num_child = atoi(argv[1]);
   int num_iter = atoi(argv[2]);
   int mode = atoi(argv[3]);

   // Fork children
   for(int i = 1; i< num_child; i++) {
     pid = fork();
     sleep(2);
     if(pid == 0)
        mode_router(mode, pid, num_iter);
   }

   //Computations for parent after forking all children
   // -9 will be used as a flag for the parent process
   mode_router(mode, -9, num_iter);
}