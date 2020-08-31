/***************************************
*
*  Author: Caden Johnsen
*  Date: 2/29/20
*  File: smallShell.c
*  Description: This program creates a
*  shell similar to bash using C
*
****************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// global variable declarations
int foregroundGlobal = 0; // global for foreground, yes I'm sorry
int statusGlobal = 1; // global for status, yes I'm sorry

// function to show arguments broken up into their individual parts
void showArguments(char **args) {
  int i = 0;

  for (i = 0; args[i] != NULL; i++) {  // loop through args arguments
    printf("args[%d]: %s\n", i, args[i]); // print out the args
  }
}

// function to set up the SIGTSTP signal
void functionSIGTSTP(int signo) {
  if(foregroundGlobal == 0) {
    char *tempString = " Foreground-only mode, & is disabled\n";
    write(0, tempString, 37); // write out the foreground mode begun command
    fflush(stdout);
    write(0, "  : ", 4); // write out terminal line
    fflush(stdout);
    foregroundGlobal = 1; // switch to foreground mode
  }
  else {
    char *tempString = " Foreground-only mode & is enabled\n";
    write(0, tempString, 35);  // write out the background mode begun command
    write(0, "  : ", 4); //write out terminal line
    fflush(stdout);
    foregroundGlobal = 0; // do not switch to foreground mode
  }
}

// function to read in information from user input
char *readLine() {
  char *commandLine = NULL;
  int size = 0;
  ssize_t bufferSize = 0; // declare buffersize
  size = getline(&commandLine, &bufferSize, stdin); // take in from stdin and find the size

  if(size == 1) { // check if it only contains one character
    if(commandLine[0] == 10) {  // check if the input is the ASCII value of "\n"
    strcpy(commandLine, "@"); // add in a known value to print later on
    }
  }
  else {
    commandLine[size - 1] = '\0'; // set last character to NULL
  }

  if(strstr(commandLine, "$$")) { // if line contains "$$" replace it with current PID
    commandLine[size - 3] = '\0'; // remove last two characters (they will be "$$")
    sprintf(commandLine, "%s%d", commandLine, getpid());  // add on PID
  }

  return commandLine; // return entire line from stdin
}

// function to seperate arguments in command line input
char **argumentSeperator(char *userInput, int *foreground, int *background) {
  int argLimit = 512; //set argument limit to 512 arguments
  int counter = 0;
  char *temp;  // temporary string to hold each word
  char **arguments = malloc(argLimit * sizeof(char *)); // allocate memory for size of arguments

  temp = strtok(userInput, " \t\r\n\a\0");   // if " \t\r\n\a\0" then split and create new array
  while(temp != NULL) {  // loop while arguments are still available
    if(strcmp(temp, "&") == 0 && strcmp(arguments[0], "echo")) {
       if(foregroundGlobal == 0){  // check if not in foreground mode
          (*background) = 1;  // switch to background mode
       }
        arguments[counter] = NULL;  // set all arguments to NULL and do nothing
    }
    else {
      arguments[counter] = temp; // add current argument to arguments array
    }
    temp = strtok(NULL, " \t\r\n\a\0");  // reset argument seperators
    counter++;
  }
  arguments[counter] = NULL;  // set last argument to NULL

  return arguments; // return array of arguments
}

// contains the if statements for built in commands as well as all other commands
void ifStatements(char **args, int *foreground, int *background, int *exitFlag, struct sigaction SIGINT_action, struct sigaction SIGTSTP_actions) {
  int status, input, output, result, i, j;  // declaring variables to be used
  int temp = 0;
  int counter = 0;
  char *tempString;
  char **tempArgument = malloc(512 * sizeof(char *)); // allocate memory for size of tempArguments
  pid_t pid;  // get PID from desired program

  if(strncmp(args[0], "exit", 4) == 0) {  // if first 4 characters are "exit"
    (*exitFlag) = 0;  // exit the while loop in main
  }
  else if (strncmp(args[0], "status", 6) == 0) { // compare first 6 characters for "status"
    if(WIFEXITED(statusGlobal) != 0) { // check if exited signal was sent
      printf("exit value: %d\n", WEXITSTATUS(statusGlobal));  // print the exited signal value
      fflush(stdout); // clear stdout
    }
    if(WIFSIGNALED(statusGlobal) != 0) { // check if terminated signal was sent
      printf("terminated by signal %d\n", WTERMSIG(statusGlobal));  // print the terminated signal value
      fflush(stdout); // clear stdout
    }
  }
  else if (strncmp(args[0], "cd", 2) == 0) {  // compare first 2 characters for "cd"
    if(args[1] == NULL) {  // if nothing after cd then go HOME
      if(chdir(getenv("HOME")) == 0) {}  // move to HOME directory
      else {
        perror("smallsh");  // if fail, print error with name of run file
        fflush(stdout);
      }
    }
    else {
      if(chdir(args[1]) == 0) {}  // if something after cd, go to that folder
      else {
        perror("smallsh");  // if folder does not exist, print error with name of run file
        fflush(stdout);
      }
    }
  }
  else if(args[0][0] == 64) {}  // if ASCII value is "@" (assigned in readLine function)
  else if(args[0][0] == 35) {}  // if ASCII value is "#" (if comment do nothing)
  else {  // using commands already available
    sigset_t signalSet;  // set up signalSet
    sigemptyset(&signalSet); // empty the set of signals
    sigaddset(&signalSet, SIGTSTP);  // add newly created signal into the set of signals
    sigprocmask(SIG_BLOCK, &signalSet, NULL);  // sigtstp can't be handled until it has been unblocked

    pid = fork(); // create child and get pid

    if(pid < 0) { // impossible value so print error
      perror("fork error\n"); // print error
      exit(2);  // exit with bad value
    }
    else if(pid == 0) { // if inside child process
      if((*background) == 0) {  // if inside background
        SIGINT_action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &SIGINT_action, NULL);
      }
    if(args[1] != NULL) { // if the second argument is not NULL
       if(strstr(args[1], "<") || strstr(args[1], ">")) { //  look for < and > operands
         if(args[3] != NULL) { // if the fourth argument is not NULL
           tempArgument[0] = args[0]; // make a new args holder without operand
           tempArgument[1] = args[2];
           tempArgument[2] = NULL;
           int fileDescriptor = open(args[4], O_CREAT|O_TRUNC|O_WRONLY, 0644);  // makes file in directory
           dup2(fileDescriptor, 1); // make next stdout go into newly made file
           fflush(stdout);
           execvp(tempArgument[0], tempArgument); // execute new command and post results into file
           fflush(stdout);
           close(fileDescriptor); // close the file
         }
         else if(strstr(args[1], ">")) { // look for > operand
           tempArgument[0] = args[0]; // change args without redirection
           tempArgument[1] = NULL;
           int fileDescriptor = open(args[2], O_CREAT|O_TRUNC|O_WRONLY, 0644);  // makes file in directory
           dup2(fileDescriptor, 1); // send stdout to file
           fflush(stdout);
           execvp(tempArgument[0], tempArgument); // execute function
           fflush(stdout);
           close(fileDescriptor); // close the file
         }
         else if(strstr(args[1], "<")) { // look for < operand
           tempArgument[0] = args[0]; // make a new args holder without operand
           tempArgument[1] = args[2];
           tempArgument[2] = NULL;
           execvp(args[0], tempArgument); // execute the given function
         }
       }
    }
    if(execvp(args[0], args) < 0) { // execute first command in args
        perror(args[0]); // if it fails print error
        fflush(stdout);
        exit(2);  // exit with given value
      }
    }
    else { //if parent process
      if(foregroundGlobal == 0 && (*background) != 0) { // if background process
        printf("Background PID: %d\n", pid);  // display background PID
        waitpid(pid, &status, WNOHANG); // wait for child process to end
        fflush(stdout);
        (*background) = 0;  // set it to not be in background process
      }
      else { //if not a background process
        do {
          waitpid(pid, &status, WUNTRACED); // wait for child process to end
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));  // do this while it has not exited and not signaled

        if(WIFSIGNALED(status)) { // if signal is received
          printf("Terminated by signal %d\n", WTERMSIG(status));  // show what signal was sent
          fflush(stdout);
        }
      }
      statusGlobal = status;  // set statusGlobal to local status
    }
    sigprocmask(SIG_UNBLOCK, &signalSet, NULL);  // unblock signal
  }
  free(tempArgument);  // free memory of tempArgument
}

int main (int argc, char const *argv[]) {
  int i = 0, j = 0, counter = 0, temp = 0;
  int foreground = 0; // does not get used since foreground had to be changed to global due to functionSIGTSTP
  int background = 0; // flag if in background mode
  int exitFlag = 1; // flag to exit while loop
  char *userInput = NULL; // string to take in stdin
  char **args;  // array of arguments
  struct sigaction SIGINT_action = {0}; //declare SIGINT_action to allow "control C" to appear
  struct sigaction SIGTSTP_action = {0}; // declare SIGTSTP_action to allow "control Z" to appear

  SIGINT_action.sa_handler = SIG_IGN; // assign handler the default SIG_IGN value
  sigfillset(&SIGTSTP_action.sa_mask);  // fill signal set with address of sa_mask
  SIGINT_action.sa_flags = 0; // set flags to zero
  sigaction(SIGINT, &SIGINT_action, NULL);  // sigaction function called to change action taken by a process when getting specific signal

  SIGTSTP_action.sa_handler = functionSIGTSTP;  // assign handler the functionSIGTSTP value
  sigfillset(&SIGTSTP_action.sa_mask);  // fill signal set with address of sa_mask
  SIGTSTP_action.sa_flags = SA_RESTART; // set flags to restart
  sigaction(SIGTSTP, &SIGTSTP_action, NULL);  // sigaction function called to change action taken by a process when getting specific signal

  while (exitFlag) {  // while exit signal not sent
      printf("  : "); // print out command line
      fflush(stdout); // clear stdout
    userInput = readLine(); // call readLine function and store into userInput
    args = argumentSeperator(userInput, &foreground, &background);  // seperate userInput into individual args
    // showArguments(args);  // show arguments from args
    ifStatements(args, &foreground, &background, &exitFlag, SIGINT_action, SIGTSTP_action);  // checks built in functions and calls others

    free(userInput);  // free memory from userInput
    free(args);  // free memory from args
  }

  return 0; // exit the program
}
