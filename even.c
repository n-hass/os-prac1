#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void int_handler(int signum) {
  printf("Yeah!\n");
}

void hup_handler(int signum) {
  printf("Ouch!\n");
}


int main(int argc, char *argv[]) {
  
  signal(SIGINT, int_handler); // register the SIGINT handler
  signal(SIGHUP, hup_handler); // register the SIGHUP handler

  int n;
  if (argc > 1) {
    n = atoi(argv[1]); // parse the first command line argument
  }
  
  int j = 0, i = 0;
  while (j<n) { // print i n times with a five-second gap
    
    printf("%d\n", i);
    j++;
    i+=2;
    sleep(5);
    
  }

  return 0;
}
