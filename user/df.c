
#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"



#define BLOCK_SIZE 512

int
main(void)
{
    int fd = open("/disk1", O_RDWR);
    char buf[BLOCK_SIZE];
    struct superblock sb;
    struct dinode di;

    lseek(fd,BLOCK_SIZE);
    
    if(read(fd, buf, BLOCK_SIZE)<0)
    {
        close(fd);
        exit();
    }


    memmove(&sb, buf, sizeof(sb));





    uint free = 0;
    uint numInodes = 0;
    uint freeBlocks = 0;
    uint totalBlocks = 0;
    for(int y = 0;y<25;y++)
    {
        lseek(fd,(sb.inodestart + y)*BLOCK_SIZE);
        if(read(fd, buf, BLOCK_SIZE)<0)
        {
            close(fd);
            printf(2,"Blew up while reading inodes that should exist, recreate file system?\n");
            exit();
        }
        for(int i =0;i<8;i++)
        {
            memmove(&di, buf + i*sizeof(di), sizeof(di));
            if(di.type ==0)
            {
                free++;

            }
            numInodes++;
        }  
        
    }      
    totalBlocks = sb.size;
    uint numBitmapBlocks = (sb.size + BPB - 1) / BPB;
    uint bitsProcessed = 0; 

    for (uint b = 0; b < numBitmapBlocks; b++) {
        lseek(fd, (sb.bmapstart + b) * BLOCK_SIZE);
        if(read(fd, buf, BLOCK_SIZE)<0)
        {
            close(fd);
            printf(2,"Blew up while reading blocks that should exist, recreate file system?\n");
            exit();
        }

        for (int i = 0; i < BLOCK_SIZE && bitsProcessed < totalBlocks; i++) {
            for (uint j = 0; j < 8 && bitsProcessed < totalBlocks; j++) {
                if ((buf[i] & (1 << j)) == 0) {
                    freeBlocks++;
                }
                bitsProcessed++;
            }
        }
    }

    printf(0,"free blocks / blocks: %d / %d\nfree inodes / inodes: %d / %d\n",freeBlocks,totalBlocks,free,numInodes);

    


    close(fd);
    exit();
}