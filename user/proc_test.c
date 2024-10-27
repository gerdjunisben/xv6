#include "kernel/types.h"
#include "user.h"

int compute(int num_iter, int sleep_time) {
    int final = 0;
    for (int i = 0; i < num_iter; i++) {
        for(int j = 0; j < 14600000; j++) {
            final += j * i;
        }
        sleep(sleep_time);
    }
    return final;
}

void mode_router(int mode, int pid, int num_iter) {
    int final = 0;

    //cpu burst mode
    if (mode == 0) {
        final = compute(num_iter, 0);
    }

    //I/O burst mode
    else if (mode == 1) {
        final = compute(num_iter, 3);
    }

    // 50/50 mode
    else {
        final = compute(num_iter, 1);
    }

    // parent process
    if (pid == -9) {
        printf(0,"> parent process finished\n> final = %d\n", final);
        exit();
    }
    // child process
    else {
        printf(0, "> process %d exited\n> final = %d\n", pid, final);
        exit();
    }
}

int validargs(int argc, char *argv[]) {
    if (argc < 4)
        return -1;

    int num_child = atoi(argv[1]);
    int num_iter = atoi(argv[2]);
    int mode = atoi(argv[3]);

    if (num_child < 0 || num_child > 20 || num_iter < 0 || num_iter > 50000 || mode < 0 || mode > 2)
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
    printf(0, "> invalid args\n> ussage:\n  1st arg = num of children to fork [0-20]\n  2nd arg = num iterations [0-50000]\n  3rd arg = mode [0: cpu burst mode, 1: i/o burst mode, 2: 50/50 mode]\n");
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