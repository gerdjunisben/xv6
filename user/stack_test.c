#include "kernel/types.h"
#include "user.h"

void rec(int i) {
  printf(1, "%d(0x%x)\n", i, &i);
  rec(i+1);
  printf(0,"One ahead");
}

int
main(int argc, char *argv[])
{
  rec(0);
  printf(0,"End");
}