#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
/**
 * Executes the command "cat scores | grep Villanova".  In this quick-and-dirty
 * implementation the parent doesn't wait for the child to finish and
 * so the command prompt may reappear before the child terminates.
 *
 * @author Jim Glenn
 * @version 0.1 9/23/2004
 */
#include "llist.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define IN_HALF 0
#define OUT_HALF 1

enum command_type {
  AND_COMMAND,         // A && B
  SEQUENCE_COMMAND,    // A ; B
  OR_COMMAND,          // A || B
  PIPE_COMMAND,        // A | B
  SIMPLE_COMMAND,      // a simple command
  SUBSHELL_COMMAND,    // ( A )
};

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or null if none.
  char *input;
  char *output;

  union
  {
    // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
    struct command *command[2];

    // for SIMPLE_COMMAND:
    char **word;

    // for SUBSHELL_COMMAND:
    struct command *subshell_command;
  } u;
};

typedef struct command * command_t;
// How will we actually do this?
int
execute_simple (command_t c, int input_fd, int output_fd, node_t close_list) 
{
  int pid;
  node_t i;
  
  pid = fork();

  if (pid == 0) // in child
  {
    // replace standard input with input part of pipe
    dup2(input_fd, STDIN_FILENO);
    
    // replace standard output with output part of pipe
    dup2(output_fd, STDOUT_FILENO);

    // Close neccessary files
    for(i = close_list->next; i != close_list; i = i->next)
      close(i->val);
    
    // execute command
    execvp(c->u.word[0], c->u.word);
  }

  return pid;
}

void
execute_commands_helper (command_t c, int input_fd, int output_fd, node_t close_list, node_t pid_list)
{
  int status, pid, ignore;
  int pipefd[2];
  node_t n;
  switch (c->type)
    {
      //int left_pid, right_pid;
      case SIMPLE_COMMAND:
        pid = execute_simple (c, input_fd, output_fd, close_list);
        insert_node (pid_list, pid);
        break;

      case PIPE_COMMAND:
        // Set up pipe
        ignore = pipe(pipefd);
        
        // Start the left side of the pipe
        // Need to close the input half of the pipe for the left side
        insert_node (close_list, pipefd[IN_HALF]);
        execute_commands_helper (c->u.command[0], input_fd, pipefd[OUT_HALF], close_list, pid_list);
        // Don't want to close the input half anymore
        remove_last_element (close_list);

        // Start the right side of the pipe
        // Need to close the output_fd half of the pipe for the right side
        insert_node (close_list, pipefd[OUT_HALF]);\
        execute_commands_helper (c->u.command[1], pipefd[IN_HALF], output_fd, close_list, pid_list);
        // Don't want to close the output half anymore
        remove_last_element (close_list);

        // Close the pipe (not needed anymore)
        close(pipefd[IN_HALF]);
        close(pipefd[OUT_HALF]);
        break;

      case SUBSHELL_COMMAND:
        // TODO: Need to parallelize this
        execute_commands_helper (c->u.command[0], input_fd, output_fd, close_list, pid_list);
        break;

      case AND_COMMAND:
        // Execute left side
        execute_commands_helper (c->u.command[0], input_fd, output_fd, close_list, pid_list);
        // Wait for the left side to either exit or finish
        for (n = pid_list->next; n != pid_list; n = n->next)
          {
            waitpid (n->val, &status, 0);
            if (status)
              // TODO: Need to kill child processes
              exit (status);
          }
        // Left side exited successfully, remove their pids from the list and execute right side
        while (pid_list->next != pid_list) { remove_last_element (pid_list); }
        execute_commands_helper (c->u.command[1], input_fd, output_fd, close_list, pid_list);
        break;

      case OR_COMMAND:
        // Execute left side
        execute_commands_helper (c->u.command[0], input_fd, output_fd, close_list, pid_list);
        // Wait for the left side to either exit or finish
        for (n = pid_list->next; n != pid_list; n = n->next)
          {
            waitpid (n->val, &status, 0);
            if (status)
              {
                // Left side exited unsuccessfully, remove their pids from the list and execute right side
                // TODO: Need to kill child processes
                while (pid_list->next != pid_list) { remove_last_element (pid_list); }
                execute_commands_helper (c->u.command[1], input_fd, output_fd, close_list, pid_list);
              }
          }
        break;

      case SEQUENCE_COMMAND:
        execute_commands_helper (c->u.command[0], input_fd, output_fd, close_list, pid_list);
        execute_commands_helper (c->u.command[1], input_fd, output_fd, close_list, pid_list);
        break;
    }
}

int main(int argc, char **argv)
{
  int pipefd[2];
  int pid1, pid2, status;
  int i;

  // Make command tree
  char *cat_args[] = {"cat", "previous_examples", NULL};
  char *catfail_args[] = {"cat", "llish.h", NULL};
  char *grep_args[] = {"grep", "pid", NULL};
  char *tr_args[] = {"tr", "aeiou", "-----", NULL};
  char *cut_args[] = {"cut", "-b", "1-10", NULL};

  struct command cat;
  cat.type = SIMPLE_COMMAND;
  cat.u.word = cat_args;

  struct command catfail;
  catfail.type = SIMPLE_COMMAND;
  catfail.u.word = catfail_args;

  struct command grep;
  grep.type = SIMPLE_COMMAND;
  grep.u.word = grep_args;

  struct command tr;
  tr.type = SIMPLE_COMMAND;
  tr.u.word = tr_args;

  struct command cut;
  cut.type = SIMPLE_COMMAND;
  cut.u.word = cut_args;

  struct command and1;
  and1.type = AND_COMMAND;
  and1.u.command[0] = &cat;
  and1.u.command[1] = &catfail;

  struct command or1;
  or1.type = OR_COMMAND;
  or1.u.command[0] = &catfail;
  or1.u.command[1] = &cat;

  struct command seq1;
  seq1.type = SEQUENCE_COMMAND;
  seq1.u.command[0] = &cat;
  seq1.u.command[1] = &catfail;

  struct command subshell1;
  subshell1.type = SUBSHELL_COMMAND;
  subshell1.u.command[0] = &or1;
  subshell1.u.command[1] = 0;

  struct command pipe1;
  pipe1.type = PIPE_COMMAND;
  pipe1.u.command[0] = &subshell1;
  pipe1.u.command[1] = &grep;

  struct command pipe2;
  pipe2.type = PIPE_COMMAND;
  pipe2.u.command[0] = &pipe1;
  pipe2.u.command[1] = &tr;

  struct command pipe3;
  pipe3.type = PIPE_COMMAND;
  pipe3.u.command[0] = &pipe2;
  pipe3.u.command[1] = &cut;

  // Actual code

  // Dummy heads for doubly-linked list for close files and pids
  node_t close_list = initialize_llist();
  node_t pid_list = initialize_llist();

  execute_commands_helper (&pipe3, STDIN_FILENO, STDOUT_FILENO, close_list, pid_list);

  // Wait on all of the processes that are still running
  node_t n;
  int pid_count = 0;
  for (n = pid_list->next; n != pid_list; n = n->next)
  {
    waitpid (n->val, &status, 0);
    pid_count++;
  }

  printf("pids: %d\n", pid_count);

  return 0;
}
