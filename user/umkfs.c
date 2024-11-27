#include "kernel/types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  mkfs("/disk2");
  exit();
}