#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"
#include "mkfs.h"



#define BLOCK_SIZE 512

int
main(int argc, char* argv[])
{
    if(argc<2)
    {
        printf(2,"You gotta give a disk arg\n");
        exit();
    }
    //probably should make sure it's a disk but for now
    int fd = open(argv[1], O_RDWR);
    if(fd<0)
    {
        printf(2,"Invalid disk\n");
        exit();
    }
    close(fd);
    exit();
}