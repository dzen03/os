#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pipe1[2]; // Parent->Child
  int pipe2[2]; // Child->Parent
  // Both pipes are: {read, write}

  pipe(pipe1);
  pipe(pipe2);
  

  int child_pid = 0;
  child_pid = fork();

  if (child_pid < 0)
  {
    fprintf(2, "Fork failed\n");
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);
    exit(-1);
  }
  // Parent process
  if (child_pid > 0)
  {
    close(pipe1[0]);
    // printf("%d %d\n", getpid(), child_pid);

    char* message_to_send = "ping";

    write(pipe1[1], message_to_send, strlen(message_to_send) + 1);
    close(pipe1[1]);


    wait(0);

    close(pipe2[1]);

    char recieved_message[100];

    read(pipe2[0], &recieved_message, 100);
    printf("%d: got %s\n", getpid(), recieved_message);

    close(pipe2[0]);
    
    exit(0);
  }
  //Child process
  else 
  {
    close(pipe1[1]);

    char recieved_message[100];

    read(pipe1[0], &recieved_message, 100);
    printf("%d: got %s\n", getpid(), recieved_message);

    close(pipe1[0]);
    close(pipe2[0]);

    char* message_to_send = "pong";
    write(pipe2[1], message_to_send, strlen(message_to_send) + 1);

    close(pipe2[1]);
    return 0;
  }

}