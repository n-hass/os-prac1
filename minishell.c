/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

   Edited by Nicholas Hassan for the University of Adelaide Operating Systems COMP SCI 3004, Sem 2 2023

********************************************************************/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define NV 20  /* max number of command tokens */
#define NL 100 /* input buffer size */
#define MAX_DETACHED 10 /* max number of detached processes */
char line[NL]; /* command input buffer */

/*
        shell prompt
 */
void prompt(void) {
  // I have commented out these prompts as they are not needed in the final submission of the autograder
  // fprintf(stdout, "\n msh> ");
  // fflush(stdout);
}

// a struct to hold the state of backgrounded/detached commands
struct child_proc {
  int pid; // the unix PID of the spawned forked process
  int minishell_id; // the ID given by the shell for the command (starting at 1, ie [1], [2], etc.)
  char command[NL]; // the command that is being run
};


static struct child_proc* detached; // a list of detached processes for tracking their execution
static int n_detached = 0; // number of detached processes
static int process_num = 1; // id number for backgrounding commands (starting at 1, ie [1], [2], etc.)


/**
 * This signal handler is run every time a command finishes execution.
 * The main purpose of this SIGCHLD handler is to monitor detached processes
 * executed in the background. When a detached process finishes, the handler
 * checks the processes that were launched in the background and reports completion
 * for the appropriate command.
 */
void sigchld_handler (int signum) {

  int status, pid;

  // this for loop iterates through the detached processes, checking their completion and reporting it when that occurs
  for (int i=0; i<n_detached; i++) {
    pid = waitpid(detached[i].pid, &status, WNOHANG); // wait until the detached process has finished
    if (pid == -1) {
      if (errno != ECHILD) perror("Command failed");
    } else if (pid > 0) {
      if (WIFEXITED(status)) {
        printf("[%d]+ Done                        %s\n", detached[i].minishell_id, detached[i].command);
      }
      // Remove this process from the list and move everything else up
      for (int j = i; j < n_detached - 1; j++) {
        detached[j] = detached[j + 1];
      }
      n_detached--;
      process_num--;
      break;
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
  int background;      /* indicator to run command in background */
  
  signal(SIGCHLD, sigchld_handler); /* Register the sigchld handler */
  detached = malloc(MAX_DETACHED * sizeof(struct child_proc)); // Allocate memory for the detached list
  if (detached == NULL) {
    perror("Failed to allocate memory for tracking detached processes");
    exit(EXIT_FAILURE);
  }

  /* prompt for and process one command line at a time  */

  while (1) { /* do Forever */
    prompt();
    fgets(line, NL, stdin);
    fflush(stdin);

    // PROCESS TOKENS //
    if (feof(stdin)) { /* non-zero on EOF  */

      // fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin),
      //         ferror(stdin));
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
          // fprintf(stderr, "cd: %s: No such file or directory\n", v[1]); // report error if the cd fails
        }
      }
      continue; // skip the rest of the command
    }
    
    background = 0; // assume the process should not run in the background
    if (v[i - 1] != NULL && strcmp(v[i - 1], "&") == 0) { // If the last token is "&", enable background mode
      background = 1;
      v[i - 1] = NULL; // remove the trailing & from the command
    }

    if (n_detached == MAX_DETACHED) { // If the list of detached commands is full, report an error and skip it
      printf("Max detached processes reached\n");
      continue;
    }

    /* fork a child process to exec the command in v[0] */

    switch (frkRtnVal = fork()) {
    case -1: /* fork returns error to parent process */
    {
      perror("Fork failed");
      break;
    }
    case 0: /* code executed only by child process */
    {
      execvp(v[0], v);
      fflush(stdout);
      perror(v[0]); // This will be executed if execvp fails
      exit(EXIT_FAILURE); // Terminate the child process with a failure status
    }
    default: /* code executed only by parent process */
    {
      if (background) { // If background mode is enabled
        char full_cmd[NL] = ""; // Create a charray to hold the full command
        for(int k=0; k<i-2; k++) {
          strcat(full_cmd, v[k]); // Concatenate the command tokens into a single string
          strcat(full_cmd, " ");
        }
        strcat(full_cmd, v[i-2]); // Concatenate the last token

        struct child_proc new_child = {frkRtnVal, process_num, ""}; // Create a new child process state container
        strcpy(new_child.command, full_cmd);

        detached[n_detached] = new_child; // Add the new child process to the list of backgrounded commands being executed
        printf("[%d] %d\n", process_num, frkRtnVal);
        fflush(stdout);
        
        process_num++;
        n_detached++;

        // Bring the shell back into the foreground - not needed
        // tcsetpgrp(STDIN_FILENO, shell_pgid);

      } else {
        wpid = wait(0); // wait for the child process to finish like normal (in foreground)
        fflush(stdout);
        if (wpid == -1) { // if the child process emits a signal, but the signal is not an ECHILD error
          if (errno != ECHILD) perror("Command failed");
        } else {
          // printf("%s done\n", v[0]);
        }
      }
      break;
    }
    } /* switch */
  }   /* while */

  free(detached); // Free the memory for list of detached command state
} /* main */
