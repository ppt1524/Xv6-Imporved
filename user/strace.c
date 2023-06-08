#include "kernel/param.h"
#include "kernel/stat.h"
#include "user/user.h"
// #include "kernel/types.h"

int main(int argc, char *argv[])
{

  if (argc < 3)
  {
    printf("Usage: %s mask command\n", argv[0]);
    exit(1);
  }
  // printf("hello1\n");

  trace(atoi(argv[1]));
  // printf("hello2\n");

  // printf("hello3\n");
  exec(argv[2], &argv[2]);

  exit(0);
}
