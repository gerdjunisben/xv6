#include "kernel/types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char path[256];
  if(argc>1)
  {
    strcpy(path,argv[1]);
  }
  else
  {
    printf(2,"Invalid path\n");
    exit();
  }
  unmount(path);
  exit();
}