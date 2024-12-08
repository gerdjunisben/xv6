#include "kernel/types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char diskName[20];
  diskName[0] = '/';
  if(argc>1)
  {
    uint i =0;
    while(argv[1][i] !='\0')
    {
      diskName[i+1] = argv[1][i];
      i++;
    }
    diskName[i+1] = '\0';
  }
  else
  {
    printf(2,"Invalid disk\n");
    exit();
  }
  printf(0,"Formatting disk %s, this may take a few moments\n",diskName);
  if(mkfs(diskName)<0)
  {
    printf(0,"Formatting failed\n");
  }
  exit();
}