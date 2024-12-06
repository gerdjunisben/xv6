#include "kernel/types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char diskName[20];
  char pathName[256];
  diskName[0] = '/';
  if(argc>2)
  {
    uint i =0;
    while(argv[1][i] !='\0')
    {
      diskName[i+1] = argv[1][i];
      i++;
    }
    diskName[i+1] = '\0';

    strcpy(pathName,argv[2]);
    mkdir(pathName);
  }
  else
  {
    printf(2,"Invalid args\n");
    exit();
  }
  printf(1,"disk %s, path %s\n",diskName,pathName);
  mount(diskName,pathName);
  exit();
}