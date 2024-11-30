
#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"



#define BLOCK_SIZE 512

int
main(void)
{
    mkdir("/stuff");
    mount("/disk2","/stuff");
    exit();
}