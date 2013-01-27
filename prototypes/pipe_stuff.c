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

typedef enum {
  PIPE,
  SIMPLE
} command_type;

typedef struct command_
{
  command_type type;
  union
  {
    char ** word;
    struct command_ *commands[2];
  } u;
} command;

// How will we actually do this?
int
execute_simple (command *cmd, int input_fd, int output_fd, node_t close_list) 
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
    execvp(cmd->u.word[0], cmd->u.word);
  }

  return pid;
}

void
pipe_commands(command *cmd, int input_fd, int output_fd, node_t close_list, node_t pid_list)
{
  //int left_pid, right_pid;
  if (cmd->type == SIMPLE)
    {
      int pid = execute_simple (cmd, input_fd, output_fd, close_list);
      insert_node (pid_list, pid);
    }

  if(cmd->type == PIPE)
    {
      int pipefd[2];
      pipe(pipefd);
      
      insert_node (close_list, pipefd[IN_HALF]);
      pipe_commands (cmd->u.commands[0], input_fd, pipefd[OUT_HALF], close_list, pid_list);
      remove_last_element (close_list);

      insert_node (close_list, pipefd[OUT_HALF]);\
      pipe_commands (cmd->u.commands[1], pipefd[IN_HALF], output_fd, close_list, pid_list);
      remove_last_element (close_list);

      close(pipefd[IN_HALF]);
      close(pipefd[OUT_HALF]);
    }
}

int main(int argc, char **argv)
{
  int pipefd[2];
  int pid1, pid2, status;
  int i;

  // Make command tree
  char *cat_args[] = {"cat", "previous_examples", NULL};
  char *grep_args[] = {"grep", "pid", NULL};
  char *tr_args[] = {"tr", "aeiou", "-----", NULL};
  char *cut_args[] = {"cut", "-b", "1-10", NULL};

  command cat;
  cat.type = SIMPLE;
  cat.u.word = cat_args;

  command grep;
  grep.type = SIMPLE;
  grep.u.word = grep_args;

  command tr;
  tr.type = SIMPLE;
  tr.u.word = tr_args;

  command cut;
  cut.type = SIMPLE;
  cut.u.word = cut_args;

  command pipe1;
  pipe1.type = PIPE;
  pipe1.u.commands[0] = &cat;
  pipe1.u.commands[1] = &grep;

  command pipe2;
  pipe2.type = PIPE;
  pipe2.u.commands[0] = &pipe1;
  pipe2.u.commands[1] = &tr;

  command pipe3;
  pipe3.type = PIPE;
  pipe3.u.commands[0] = &pipe2;
  pipe3.u.commands[1] = &cut;

  // Actual code

  // Dummy heads for doubly-linked list for close files and pids
  node_t close_list = initialize_llist();
  node_t pid_list = initialize_llist();

  int count = 0; // number of files to wait for;

  pipe_commands (&pipe3, STDIN_FILENO, STDOUT_FILENO, close_list, pid_list);

  node_t n;
  int pid_count = 0;
  for (n = pid_list->next; n != pid_list; n = n->next)
  {
    // This works for something simple like this
    // wait (&status);
    // Trying to move to:
    waitpid (n->val, &status, 0);
    printf("Stuff and things\n");
    pid_count++;
  }

  printf("pids: %d\n", pid_count);
  printf("count: %d\n", count);
  
  for(i=0;i<count;i++);

  return 0;
}
