#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int i;

  if (argc < 2)
  {
    fprintf(2, "Usage: mkdir files...\n");
    exit(1);
  }
  // printf("hello1\n");
  for (i = 1; i < argc; i++)
  {
    if (mkdir(argv[i]) < 0)
    {
      fprintf(2, "mkdir: %s failed to create\n", argv[i]);
      break;
    }
  }
  // printf("hello2\n");
  exit(0);
}
