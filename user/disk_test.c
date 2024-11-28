
#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"



#define BLOCK_SIZE 512

int
main(void)
{
    int fd = open("/disk2", O_RDWR);
    char buf[BLOCK_SIZE];
    struct superblock sb;
    //struct dinode di;

    lseek(fd,BLOCK_SIZE);
    
    uint b = read(fd, buf, BLOCK_SIZE);
    if(b<0)
    {
        close(fd);
        exit();
    }


    memmove(&sb, buf, sizeof(sb));
    printf(0, "Superblock Information:\n");
    printf(0, "  Size: %d blocks\n", sb.size);
    printf(0, "  Number of data blocks: %d\n", sb.nblocks);
    printf(0, "  Number of inodes: %d\n", sb.ninodes);
    printf(0, "  Number of log blocks: %d\n", sb.nlog);
    printf(0, "  Log starts at block: %d\n", sb.logstart);
    printf(0, "  Inodes start at block: %d\n", sb.inodestart);
    printf(0, "  Bitmap starts at block: %d\n", sb.bmapstart);

    close(fd);

    fd = open("/disk1", O_RDWR);
    lseek(fd,BLOCK_SIZE);
    
    b = read(fd, buf, BLOCK_SIZE);
    if(b<0)
    {
        close(fd);
        exit();
    }


    memmove(&sb, buf, sizeof(sb));
    printf(0, "Superblock Information:\n");
    printf(0, "  Size: %d blocks\n", sb.size);
    printf(0, "  Number of data blocks: %d\n", sb.nblocks);
    printf(0, "  Number of inodes: %d\n", sb.ninodes);
    printf(0, "  Number of log blocks: %d\n", sb.nlog);
    printf(0, "  Log starts at block: %d\n", sb.logstart);
    printf(0, "  Inodes start at block: %d\n", sb.inodestart);
    printf(0, "  Bitmap starts at block: %d\n", sb.bmapstart);

    /*
    printf(0, "Inodes per block %d\n",IPB); //64 bytes for each inode, 8 per block
    printf(0,"Inode 0 is in block %d\n",IBLOCK(1,sb));

    

    lseek(fd, BLOCK_SIZE * sb.bmapstart);
    read(fd, buf, BLOCK_SIZE);
    for(int i =0;i<BLOCK_SIZE;i++)
    {
        if(buf[i]!=-1)
        {
            printf(0,"Bit map byte %d is %d\n",i,buf[i]);
        }
    }


    uint free = 0;
    uint numInodes = 0;
    //uint freeBlocks = 0;
    //uint totalBlocks = 0;
    for(int y = 0;y<25;y++)
    {
        lseek(fd,(sb.inodestart + y)*BLOCK_SIZE);
        read(fd, buf, BLOCK_SIZE);
        for(int i =0;i<8;i++)
        {
            memmove(&di, buf + i*sizeof(di), sizeof(di));
            if(di.type ==0)
            {
                free++;

            }
            numInodes++;
            
            //printf(0, "  Type: %d\n", di.type);       
            //printf(0, "  Major %d\n",di.major);        
            //printf(0, "  Minor: %d\n", di.minor);         
            //printf(0, "  Size: %d\n\n\n", di.size);   
            
        }  
        
    }      
    printf(0,"Total free %d, total %d\n",free,numInodes);
    totalBlocks = sb.size;
    uint numBitmapBlocks = (sb.size + BPB - 1) / BPB;
    uint bitsProcessed = 0; 

    for (uint b = 0; b < numBitmapBlocks; b++) {
        lseek(fd, (sb.bmapstart + b) * BLOCK_SIZE);
        read(fd, buf, BLOCK_SIZE);

        for (int i = 0; i < BLOCK_SIZE && bitsProcessed < totalBlocks; i++) {
            for (uint j = 0; j < 8 && bitsProcessed < totalBlocks; j++) {
                if ((buf[i] & (1 << j)) == 0) {
                    freeBlocks++;
                }
                bitsProcessed++;
            }
        }
    }

    printf(0,"Total free blocks %d, total blocks %d\n",freeBlocks, totalBlocks);
    */
    


    close(fd);
    exit();
}