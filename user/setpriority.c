#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
#ifdef PBS
  if (argc < 3)
  {
    printf("Usage: %s command\n", argv[0]);
    exit(1);
  }

  int ret = set_priority(atoi(argv[1]), atoi(argv[2]));

  if (ret == -1)
  {
    printf("%s: set_priority error\n", argv[0]);
    exit(1);
  }
  else if (ret > 100)
  {
    printf("%s: pid %s not found\n", argv[2]);
    exit(1);
  }
#endif

  exit(0);
}
