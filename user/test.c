#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  uint64 res = 0;
  dump2(-1, 2, &res);
  printf("%d\n", res);
  exit(0);
}
