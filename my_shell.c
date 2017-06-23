#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

char * internalCommands[] = {
  "exit",
  "pwd",
  "history",
  "cd",
  "EOF"
};

// Signal handling
// Handler for SIGINT (CTRL+C)
void sigintHandler(int sigNum) {
  signal(SIGINT, sigintHandler);
  fflush(stdout);
}


// BT(built in) commands
// Exit
void BT_exit(char ** params) {

  return exit(EXIT_SUCCESS);
}

// PWD
// Gets variable environment and prints
void BT_pwd(const char* name) {

  char namePath[1024];
  // printf("BT_pwd name=%s\n", name );
  // namePath = getenv("name");
  //printf("namePath=%s\n",namePath );
  if ( getcwd(namePath, sizeof(namePath)) != NULL ) {
    fprintf(stderr, "%s\n", namePath);
  }
  return;
}

// History
// Start history in main and stifle history to 10 entries
// Get history list and print each line
#define HISTORY_SIZE 10
void BT_history( char * history[], int histPos ) {

  int current = histPos;
  int histNum = 1;
  int histSize = HISTORY_SIZE;

  do {
    if (history[current]) {
      printf( "%4d\t%s\n", histNum, history[current] );
      histNum++;
    }
    current = (current + 1) % histSize;
  } while(current != histPos);
}

// CD
void BT_cd(char ** params) {

  // Expecting cd
  if ( params[1] == NULL ) {
    printf("expected 'cd'\n");
  } else {
    // chdir returns 0 on success so
    if ( params[1] == ".." ) {
      if ( chdir("params[1]") != 0 ) {
        // printf("error in chdir\n");
        perror("cd");
      }
    } else {
      if ( chdir(params[1]) != 0 ) {
      // printf("error in chdir\n");
      perror("cd");
      }
    }
  }
}


// Reads from stdin and returns a buffer
#define RCL_BUFFERSIZE 1024
char * read_command_line(void) {

  int buffersize = RCL_BUFFERSIZE;
  int pos = 0;
  char * buffer = malloc( (buffersize * sizeof(char)) + 1 ); // +1 for '\0'
  int c;

  if( buffer == NULL ) {
    printf( "Memory allocation failed for buffer\n" );
    exit(EXIT_FAILURE);
  }

  while (1) {

    c = getchar();

    // If end of string or new line character, return buffer
    // Else continue to read from stdin
    if( c == EOF || c == '\n' ) {

      buffer[pos] = '\0';
      // printf("buffer=%s\n", buffer);
      // printf("r_c_l buffer return seg fault\n");
      return buffer;
    } else {

      buffer[pos] = c;
      pos++;
    }

    // If buffer is not large enough, realloc with double the buffer size
    if ( pos >= buffersize ) {

      buffersize += RCL_BUFFERSIZE;
      buffer = realloc( buffer, (buffersize * sizeof(char)) );
      if( buffer == NULL ) {
        printf( "Memory allocation failed for buffer\n" );
        exit(EXIT_FAILURE);
      }
    }
  }
}

// Parse command and store into a token
// Store these tokens into tokenArray to pass onto exec
#define TOKENARRAY_BUFFERSIZE 256
// Delim for strtok_r, watch for tab, whitespace and new line
#define TOKENARRAY_DELIM " \t\n"
char ** parse_RCL( char * RCL ) {

  int buffersize = TOKENARRAY_BUFFERSIZE;
  int pos = 0;
  // Don't need +1 here as '\0' isn't needed
  char ** tokenArray = malloc( buffersize * sizeof(char) );
  char * token;
  char * rest = RCL;

  if ( tokenArray == NULL ) {
    printf( "Memory allocation failed for tokenArray\n" );
    exit(EXIT_FAILURE);
  }

  // Need to call strtok_r outside of loop for its first unique call
  // printf("seg fault on strtok_r preloop\n");
  token = strtok_r( rest, TOKENARRAY_DELIM, &rest );
  // Loop until no more tokens to get pulled
  while( token != NULL ) {

    // Store each token to return
    tokenArray[pos] = token;
    // printf("tokenArray[%d]=%s\n", pos, tokenArray[pos]);
    pos++;

    // Reallocate memory if current buffersize for tokenArray is not large enough
    if ( pos >= buffersize ) {

      buffersize += TOKENARRAY_BUFFERSIZE;
      tokenArray = realloc( tokenArray, (buffersize * sizeof(char)) );
      if( tokenArray == NULL ) {
        printf( "Memory allocation failed for tokenArray\n" );
        exit(EXIT_FAILURE);
      }
    }

    token = strtok_r( NULL, TOKENARRAY_DELIM, &rest );
  }

  // Null terminate the array for exec in execute_RCL()
  tokenArray[pos] = NULL;
  unsigned int p = 0;
  // while (tokenArray[p]) {
  //   printf("tokenArray[%d]=%s\n", p, tokenArray[p]);
  //   p++;
  // }
  // printf("seg fault on return tokenArray\n");
  return tokenArray;
}

// Read command parameters, execute the process and return success
void execute_RCL( char ** params, int numOfPipes ) {

  // printf("params[0]%s\n", params[0] );
  if ( params[0] == NULL ) {
    // Empty command
    // printf("seg fault at params[0]==NULL\n");
    return;
  }

  int g = 0;
  int h = 0;
  int i = 0;
  int j = 1;
  int place;
  int commandBegin[10];
  commandBegin[0] = 0;
  const int numOfCommands = numOfPipes + 1;
  int pipefds[2*numOfPipes];

  for (i = 0; i < numOfPipes; i++) {
    if ( pipe(pipefds + 2*i) < 0 ) {
      perror("Pipe failure");
      exit(EXIT_FAILURE);
    }
  }

  // unsigned int k = 0;
  // unsigned int commandLength = 0;
  // while (params[k] != NULL) {
  //   commandLength++;
  //   // printf("seg fault at while params[k]!=NULL\n");
  //   printf("params[%d]=%s\n", k, params[k] );
  //   k++;
  // }
  // commandLength += 1;
  //printf("commandLength=%d\n", commandLength);

  pid_t childpid, w;
  int status;

  while ( params[h] != NULL ) {
    if ( !(strcmp(params[h], "|")) ) {
      params[h] = NULL;
      commandBegin[j] = h + 1;
      j++;
    }
    h++;
  }

  for (i = 0; i < numOfCommands; i++) {

    // Set each start place for chain of commands
    place = commandBegin[i];

    childpid = fork();
    if ( childpid < 0 ) {
      // Error, no child process created
      fprintf(stderr, "Fork failure, error %d\n", errno );
      exit(EXIT_FAILURE);
    } else if ( childpid == 0 ) {
      // Child process created, child code
      // If not the last command then
      if ( i < numOfPipes ) {
        if ( dup2(pipefds[g + 1], 1) < 0 ) {
          perror("dup2");
          exit(EXIT_FAILURE);
        }
      }

      // If not the last command with g != 2 * numOfPipes
      if ( g != 0 ) {
        if ( dup2(pipefds[g - 2], 0) < 0 ) {
          perror(" dup2");
          exit(EXIT_FAILURE);
        }
      }

      for(i = 0; i < 2 * numOfPipes; i++) {
        close(pipefds[i]);
      }

      // printf("before execvp\n");
      if ( execvp( params[place], params+place ) < 0 ) {
        perror("execvp");
        exit(EXIT_FAILURE);
      }
    } /* else {
        // Parent code
        // Parts referenced from waitpid Linux man page
        do {
          w = waitpid( childpid, &status, WUNTRACED | WCONTINUED );
          if ( w == -1 ) {
            perror("waitpid");
            exit(EXIT_FAILURE);
          }

          // if (WIFEXITED(status)) {
          //   printf("exited, WEXITSTATUS(status)=%d, WIFEXITED(status)=%d status=%d\n", WEXITSTATUS(status), WIFEXITED(status), status);
          // } else if (WIFSIGNALED(status)) {
          //   printf("killed by signal %d\n", WTERMSIG(status));
          // }
        } while( !WIFEXITED(status) && !WIFSIGNALED(status) ); // Execute until CTRL+C/CTRL+D
        // exit(EXIT_SUCCESS);
    } */
    g += 2;
  }

  // Parent closes pipes and waits for children
  for (i = 0; i < 2 * numOfPipes; i++) {
    close( pipefds[i] );
  }

  for (i = 0; i < numOfPipes + 1; i++) {
    wait(&status);
  }
  //return 1;
}

int main(int argc, char const *argv[]) {

  char * RCL;
  char ** params;
  // int status;
  int BT_func_used;

  // Start signal handler
  signal(SIGINT, sigintHandler);

  int histSize = HISTORY_SIZE;
  char * histList[histSize];
  int current, histPos = 0;
  for ( current = 0; current < histSize; current++) {
    histList[current] = NULL;
  }

  int paramsCounter = 0;
  int pipeCounter = 0;

  do {
    BT_func_used = 0;
    printf("> ");
    RCL = read_command_line();

    // Add line to history if it non-empty
    // printf("RCL=%s\n", RCL);
    if ( RCL != NULL ) {
      free(histList[histPos]);
      histList[histPos] = calloc( strlen(RCL) + 1, 1 );
      strcpy(histList[histPos], RCL);
      histPos = (histPos + 1) % histSize;
    }

    params = parse_RCL(RCL);

    // Iterate through params and count number of '|' as pipes
    // and send to execute_RCL
    while (params[paramsCounter]) {
      if ( !(strcmp(params[paramsCounter], "|")) ) {
        pipeCounter++;
      }
      paramsCounter++;
    }

    // printf("pipeCounter=%d\n", pipeCounter);

    // printf("seg fault before main for loop\n");
    // printf("params[0]=%s\n", params[0] );
    // Check if internal command is called only if command isn't blank
    if (params[0]) {

      for (unsigned int j = 0; j < 5; j++) {
        // printf("seg fault before main for loop if command\n");
        if ( strcmp(params[0], internalCommands[j]) == 0 ) {
          // printf("seg fault after main for loop if command\n");
          BT_func_used = 1;
          // Run the function for internalCommands[j]
          if (j == 0) {
            BT_exit(params);
            continue;
          } else if (j == 1) {
            BT_pwd(params[0]); // send path to BT_pwd()
            continue;
          } else if (j == 2) {
            BT_history(histList, histPos);
            continue;
          } else if (j == 3) {
            BT_cd(params);
            continue;
          } /* else if (params[0] == EOF) { // exit on EOF
            BT_exit(params);
            continue;
          } */
        }
      }
    }
    if (BT_func_used == 0) {
      execute_RCL(params, pipeCounter);
    }
    // printf("error before free(RCL)\n");
    free(RCL);
    // printf("error before free(params)\n");
    free(params);
  } while(1);

  return EXIT_SUCCESS;
}
