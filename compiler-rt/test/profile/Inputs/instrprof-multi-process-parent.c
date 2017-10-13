#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  char *cmd;
  asprintf(&cmd, "%s/child", dirname(argv[0]));
  if (fork() == 0) {
    system(cmd);
  }

  return 0;
}
