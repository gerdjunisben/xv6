// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"
#include "init.h"

char *argv[] = { "sh", "", 0 };

int
main(void)
{
  int pid, wpid;
  int pids[3];

  uint i;
  for( i = 0;i<2;i++)
  {
    int fd = open(devices[i], O_RDWR);
    if(fd < 0){
      mknod(devices[i], majorNums[i], minorNums[i]);
      fd = open(devices[i], O_RDWR);
    }

    //added an arg to shell so that we check that the fd for the current device
    //exists rather than just checking console fd existance
    argv[1] = devices[i];

    pid = fork();
    pids[i] = pid;
    if(pid < 0){
      close(fd);
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0)
    {
      printf(fd, "init: starting sh\n");

      close(0);
      dup(fd);
      close(1);
      dup(fd);
      close(2);
      dup(fd);

      close(fd);

      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
  }

  for(;i<deviceCount;i++)
  {
    int fd = open(devices[i], O_RDWR);
    if(fd < 0){
      mknod(devices[i], majorNums[i], minorNums[i]);
    }

  }

  for(;;){
    while((wpid=wait()) >= 0 && wpid != pid)
    {
        for(int i =1;i<deviceCount;i++)
        {
          if(wpid == pids[i])
          {
            int fd = open(devices[i], O_RDWR);
            if(fd < 0){
              mknod(devices[i], majorNums[i], minorNums[i]);
              fd = open(devices[i], O_RDWR);
            }

            argv[1] = devices[i];

            pid = fork();
            pids[i] = pid;
            if(pid < 0){
              close(fd);
              printf(1, "init: fork failed\n");
              exit();
            }
            if(pid == 0)
            {
              printf(fd, "init: starting sh\n");

              close(0);
              dup(fd);
              close(1);
              dup(fd);
              close(2);
              dup(fd);

              close(fd);

              exec("sh", argv);
              printf(1, "init: exec sh failed\n");
              exit();
            }
          }
        }
    }
  }
}
