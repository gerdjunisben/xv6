
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"



int
main(void)
{
    int fd = open("disk2", O_RDWR);
    char buf[1000];
    char* word = "balls";

    write(fd,word,6);

    close(fd);
    fd = open("disk2", O_RDWR);

    read(fd, buf, 6);

    printf(0,"Read %s bytes from disk\n", buf);

    close(fd);
    exit();
}