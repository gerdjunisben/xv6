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

  for(int i = 0;i<deviceCount;i++)
  {
    int fd = open(devices[i], O_RDWR);
    if(fd < 0){
      mknod(devices[i], majorNums[i], minorNums[i]);
      fd = open(devices[i], O_RDWR);
    }
    close(0);
    dup(fd);
    close(1);
    dup(fd);
    close(2);
    dup(fd);

    argv[1] = devices[i];

    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0)
    {
      printf(1, "init: starting sh\n");
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
  }

  for(;;){
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}
