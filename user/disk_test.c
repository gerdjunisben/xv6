
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

struct superblock {
        uint size; // Size of file system image (blocks)
        uint nblocks; // Number of data blocks
        uint ninodes; // Number of inodes.
        uint nlog; // Number of log blocks
        uint logstart; // Block number of first log block
        uint inodestart; // Block number of first inode block
        uint bmapstart; // Block number of first free map block
    };

int
main(void)
{
    int fd = open("disk1", O_RDWR);
    char buf[512];
    struct superblock sb;


    read(fd, buf, 512);
    

    read(fd, buf, 512);

    memmove(&sb, buf, sizeof(sb));
    
    printf(1, "Superblock Information:\n");
    printf(1, "  Size: %d blocks\n", sb.size);
    printf(1, "  Number of data blocks: %d\n", sb.nblocks);
    printf(1, "  Number of inodes: %d\n", sb.ninodes);
    printf(1, "  Number of log blocks: %d\n", sb.nlog);
    printf(1, "  Log starts at block: %d\n", sb.logstart);
    printf(1, "  Inodes start at block: %d\n", sb.inodestart);
    printf(1, "  Bitmap starts at block: %d\n", sb.bmapstart);

    close(fd);
    exit();
}