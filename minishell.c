/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NV 20  /* max number of command tokens */
#define NL 100 /* input buffer size */
#define MAX_DETACHED 10 /* max number of detached processes */
char line[NL]; /* command input buffer */

/*
        shell prompt
 */
void prompt(void) {
  fprintf(stdout, "\n msh> ");
  fflush(stdout);
}

static int* detached;
static int n_detached = 0;
void sigchld_handler (int signum) {
  int status, pid;
  
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    // Check if the PID is in the list of background processes
    for (int i = 0; i < n_detached; i++) {
      if (detached[i] == pid) {
        if (WIFEXITED(status)) {
          printf("\nPID %d exited with status %d\n", pid, WEXITSTATUS(status));
          fflush(stdout);
          prompt();
        }
        // Remove the PID from the list
        for (int j = i; j < n_detached - 1; j++) {
          detached[j] = detached[j + 1];
        }
        n_detached--;
        break;
      }
    }
  }
}


int main(int argk, char *argv[], char *envp[])
/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */

{
  int frkRtnVal;       /* value returned by fork sys call */
  int wpid;            /* value returned by wait */
  char *v[NV];         /* array of pointers to command line tokens */
  char *sep = " \t\n"; /* command line token separators    */
  int i;               /* parse index */
  int background;      /* indicator to run process in background */
  
  signal(SIGCHLD, sigchld_handler); /* Register the sigchld handler */
  detached = malloc(MAX_DETACHED * sizeof(int)); // Allocate memory for the detached list

  /* prompt for and process one command line at a time  */

  while (1) { /* do Forever */
    prompt();
    fgets(line, NL, stdin);
    fflush(stdin);

    // PROCESS TOKENS //
    if (feof(stdin)) { /* non-zero on EOF  */

      fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin),
              ferror(stdin));
      exit(0);
    }
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
      continue; /* to prompt */

    v[0] = strtok(line, sep);
    for (i = 1; i < NV; i++) {
      v[i] = strtok(NULL, sep);
      if (v[i] == NULL)
        break;
    }
    /* assert i is number of tokens + 1 */

    // PROCESS COMMAND //

    if (strcmp(v[0], "cd") == 0) { // check for the cd command
      if (v[1] == NULL) { // if no directory is specified
        chdir(getenv("HOME")); // if no directory is specified, change to the home directory
      } else {
        if (chdir(v[1]) == -1) { // perform the actual directory change with the specified directory
          fprintf(stderr, "cd: %s: No such file or directory\n", v[1]); // report error if the cd fails
        }
      }
    }

    background = 0; // assume the process should not run in the background
    if (v[i - 1] != NULL && strcmp(v[i - 1], "&") == 0) {
      background = 1; // If the last token is "&", enable background mode
      v[i - 1] = NULL; // remove the trailing & from the command
    }

    if (n_detached == MAX_DETACHED) { // If the detached list is full
      printf("Max detached processes reached\n");
      continue; // Skip the command
    }

    /* fork a child process to exec the command in v[0] */

    switch (frkRtnVal = fork()) {
    case -1: /* fork returns error to parent process */
    {
      break;
    }
    case 0: /* code executed only by child process */
    {
      execvp(v[0], v); // execute the command
    }
    default: /* code executed only by parent process */
    {
      if (background) { // If background mode is enabled
        detached[n_detached] = frkRtnVal; // Add the PID to the detached list
        n_detached++;
        printf("Started %s in background with PID %d\n", v[0], frkRtnVal);
      } else {
        wpid = wait(0); // wait for the child process to finish like normal (in foreground)
        printf("%s done\n", v[0]);
      }
      break;
    }
    } /* switch */
  }   /* while */

  free(detached); // Free the detached list
} /* main */
