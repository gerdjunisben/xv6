
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"



int
main(void)
{
    int fd = open("disk1", O_RDWR);
    char buf[512];

    buf[0] = 0;


    uint b = 0;
    b = read(fd, buf, 512);
    struct superblock {
        uint size; // Size of file system image (blocks)
        uint nblocks; // Number of data blocks
        uint ninodes; // Number of inodes.
        uint nlog; // Number of log blocks
        uint logstart; // Block number of first log block
        uint inodestart; // Block number of first inode block
        uint bmapstart; // Block number of first free map block
    };

    struct superblock* sb = malloc(512);
    b = read(fd, sb, 512);


    
    printf(0,"Read %d bytes super block probably size %d\n", b,sb->size);

    close(fd);
    exit();
}