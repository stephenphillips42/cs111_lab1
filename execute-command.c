// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "file_tree.h"
#include "llist.h"

#include "string.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <error.h>
#include <stdlib.h>

#define IN_HALF 0
#define OUT_HALF 1

// Debug
#include <stdio.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void
get_files (command_t cmd, file_tree *head)
{
  switch (cmd->type)
    {
      case AND_COMMAND:
      case SEQUENCE_COMMAND:
      case OR_COMMAND:
      case PIPE_COMMAND:
        get_files (cmd->u.command[0], head);
        get_files (cmd->u.command[1], head);
        break;
      case SIMPLE_COMMAND:
        if (cmd->input)
          insert_file_tree (head, cmd->input, 0);
        if (cmd->output)
          insert_file_tree (head, cmd->output, 1);
        break;
      case SUBSHELL_COMMAND:
        get_files (cmd->u.subshell_command, head);
        break;
    }
}

void
exit_with_status(int status)
{
  // FOR DEBUGGING
  printf("ERROR %d  %s\n", status, strerror (status));
  exit (status);
}

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
  int status, pid;
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
        pipe(pipefd);
        
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

void
execute_command (command_t c, bool time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  c = c;
  file_tree files = 0;
  printf ("Entering...\n");
  get_files (c, &files);
  print_file_tree(files);
  free_file_tree (&files);

  // Dummy heads for doubly-linked list for close files and pids
  node_t close_list = initialize_llist();
  node_t pid_list = initialize_llist();

  execute_commands_helper (c, STDIN_FILENO, STDOUT_FILENO, close_list, pid_list);

  // Wait on all of the processes that are still running
  node_t n;
  int status, pid_count = 0;
  for (n = pid_list->next; n != pid_list; n = n->next)
  {
    waitpid (n->val, &status, 0);
    pid_count++;
  }

  printf("pids: %d\n", pid_count);

  time_travel = false;
  if (time_travel) { ; }

  //error (1, 0, "command execution not yet implemented");
}
