#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void int_handler(int signum) {
  printf("Yeah!\n");
}

void hup_handler(int signum) {
  printf("Ouch!\n");
}


int main(int argc, char *argv[]) {
  
  signal(SIGINT, int_handler);
  signal(SIGHUP, hup_handler);

  int n;
  if (argc > 1) {
    n = atoi(argv[1]);
  }
  
  int j = 0, i = 0;
  while (j<n) {
    if (i%2 == 0) {
      printf("%d\n", i);
      j++;
      i+=2;
      sleep(5);
    }
  }

  return 0;
}