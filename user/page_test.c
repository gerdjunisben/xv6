#include "kernel/types.h"
#include "user.h"


int compute(int num_iter) {
    int final = 0;
    for (int i = 0; i < num_iter; i++) {
        for(int j = 0; j < 14600000; j++) {
            final += j * i;
        }
        sleep(1);
    }
    return final;
}

void rec(int i) {
  printf(1, "%d(0x%x)\n", i, &i);
  rec(i+1);
  printf(0,"One ahead");
}

int
main(int argc, char *argv[])
{
  uint pid;
  for(uint i =0;i<10;i++)
  {
    if ((pid = fork()) == 0) {
        uint final = compute(500);
        printf(0, "process %d exited with final %d\n", pid, final);
        exit();
    }
  }
  if((pid = fork()) ==0)
  {
    rec(0);
  }
  else
  {
    while(wait()!=-1){}
  }
    
  exit();
}