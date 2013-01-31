// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "file_tree.h"
#include "llist.h"

#include "string.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <stdio.h>

#define IN_HALF 0
#define OUT_HALF 1

// Debug

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
print_list(node_t head)
{
  node_t n;
  for (n = head->next; n != head; n = n->next)
    printf("Node: %d\n", n->val);
  printf("Node: %d\n", n->val);
}

void
print_cmdtype(command_t c)
{
  switch (c->type)
  {
    case AND_COMMAND:
      printf("AND_COMMAND\n");
      break;         
    case SEQUENCE_COMMAND:
      printf("SEQUENCE_COMMAND\n");
      break;    
    case OR_COMMAND:
      printf("OR_COMMAND\n");
      break;          
    case PIPE_COMMAND:
      printf("PIPE_COMMAND\n");
      break;        
    case SIMPLE_COMMAND:
      printf("SIMPLE_COMMAND %s\n", c->u.word[0]);
      break;      
    case SUBSHELL_COMMAND:
      printf("SUBSHELL_COMMAND\n");
      break;    
    default:
      printf("Unknown\n");
  }
}

void
exit_with_status(int status)
{
  // FOR DEBUGGING
  printf("ERROR %d  %s\n", status, strerror (status));
  exit (status);
}
void execute_commands_helper (command_t, int, int, node_t, node_t);

// How will we actually do this?
int
execute_simple (command_t c, int input_fd, int output_fd, node_t close_list) 
{
  int pid;
  node_t n;
  
  if (strcmp(c->u.word[0], "exec") == 0)
    {
      pid = 0;
      // get rid of "exec" in front
      int i;
      for (i = 0; c->u.word[i]; i++)
          c->u.word[i] = c->u.word[i + 1];
    }
  else
    {
      pid = fork();
    }

  if (pid == 0) // in child
  {
    // If the input is redirected to a file, reset stdin to that file
    if (c->input)
      {
        int fd = open (c->input, O_RDONLY);
        if(fd < 0)
          {
            for(n = close_list->next; n != close_list; n = n->next)
              close(n->val);
            perror("Cannot open file");
            exit (1);
          }
        dup2(fd, STDIN_FILENO);
        if (input_fd != STDIN_FILENO)
          insert_node (close_list, input_fd);
      }
    else // Otherwise use the passed in input file descriptor
      {
        // replace standard input with input part of pipe
        dup2(input_fd, STDIN_FILENO);
      }

    // If the output is redirected to a file, reset stdout to that file
    if (c->output)
      {
        int fd = open (c->output, O_CREAT | O_TRUNC | O_WRONLY, 
          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if(fd < 0)
          {
            for(n = close_list->next; n != close_list; n = n->next)
              close(n->val);
            perror("Cannot open file");
            exit (1);
          }
        if (output_fd != STDOUT_FILENO)
          insert_node (close_list, output_fd);
        dup2(fd, STDOUT_FILENO);
      }
    else // Otherwise use the passed in output file descriptor
      {
        // replace standard output with output part of pipe
        dup2(output_fd, STDOUT_FILENO);
      }

    // Close neccessary files
    for(n = close_list->next; n != close_list; n = n->next)
      close(n->val);
    
    // execute command
    execvp(c->u.word[0], c->u.word);
    perror("Error: ");
    exit (120);
  }

  return pid;
}

int
execute_subshell_command (command_t c, int input_fd, int output_fd, node_t close_list, node_t pid_list)
{
  int pid, status;
  node_t n;
  
  pid = fork();

  if (pid == 0) // In child
  {
    // If the input is redirected to a file, reset stdout to that file
    if (c->input)
      {
        int fd = open (c->input, O_RDONLY);
        if(fd < 0)
          {
            for (n = close_list->next; n != close_list; n = n->next)
                close (n->val);

            perror("Cannot open file");
            exit(1);
          }
        
        dup2(fd, STDIN_FILENO);
        
        input_fd = fd;

      }
    // If the output is redirected to a file, reset stdout to that file
    if (c->output)
      {
        int fd = open (c->output, O_CREAT | O_TRUNC | O_WRONLY, 
          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if(fd < 0) 
          {
            for (n = close_list->next; n != close_list; n = n->next)
                close (n->val);

            perror("Cannot open file");
            exit(1);
          }
        
        dup2(fd, STDOUT_FILENO);
    
        output_fd = fd;
      }

    // Don't want to wait on parent processes
    while (pid_list != pid_list->next)
      remove_last_element (pid_list);

    // TODO: Need to parallelize this
    execute_commands_helper (c->u.subshell_command, input_fd, output_fd, close_list, pid_list);

    for (n = close_list->next; n != close_list; n = n->next)
      close (n->val);

    // Wait for child processes
    for(n = pid_list->next; n != pid_list; n = n->next)
        waitpid(n->val, &status, 0);

 
    exit(!!status);
  }

  return pid;
}

void
execute_commands_helper (command_t c, int input_fd, int output_fd, node_t close_list, node_t pid_list)
{
  int status = 0, pid;
  int pipefd[2];
  node_t n;

  #ifdef DEBUG
  print_cmdtype (c);
  #endif

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
        insert_node (close_list, pipefd[OUT_HALF]);
        execute_commands_helper (c->u.command[1], pipefd[IN_HALF], output_fd, close_list, pid_list);
        // Don't want to close the output half anymore
        remove_last_element (close_list);

        // Close the pipe (not needed anymore)
        close(pipefd[IN_HALF]);
        close(pipefd[OUT_HALF]);
        break;

      case SUBSHELL_COMMAND:
        pid = execute_subshell_command (c, input_fd, output_fd, close_list, pid_list);
        insert_node (pid_list, pid);
        break;

      case AND_COMMAND:
        // Execute left side
        execute_commands_helper (c->u.command[0], input_fd, output_fd, close_list, pid_list);
        
        // Close neccessary files
        for(n = close_list->next; n != close_list; n = n->next)
          close(n->val);
        
        // Wait for the left side to either exit or finish
        for (n = pid_list->next; n != pid_list; n = n->next)
          {

            if(waitpid (n->val, &status, 0) < 0)
              {
                perror("Error in process");
                printf("Unexpected error in process %d", n->val);
              }
          }
        if (status) // If we get a bad signal in the last command
          {
            return;
          }
        // Left side exited successfully, remove their pids from the list and execute right side
        while (pid_list->next != pid_list)
          remove_last_element (pid_list);
        execute_commands_helper (c->u.command[1], input_fd, output_fd, close_list, pid_list);
        break;

      case OR_COMMAND:

        // Execute left side
        execute_commands_helper (c->u.command[0], input_fd, output_fd, close_list, pid_list);
        
        // Close neccessary files
        for(n = close_list->next; n != close_list; n = n->next)
          close(n->val);
        while (close_list != close_list->next)
          remove_last_element (close_list);
        
        // Wait for the left side to either exit or finish
        for (n = pid_list->next; n != pid_list; n = n->next)
          {
            waitpid (n->val, &status, 0);
          }
        if (status)
          {
            // Left side exited unsuccessfully, remove their pids from the list and execute right side
            // TODO: Need to kill child processes
            while (pid_list->next != pid_list)
              remove_last_element (pid_list);
            execute_commands_helper (c->u.command[1], input_fd, output_fd, close_list, pid_list);
          }
        break;

      case SEQUENCE_COMMAND:

        // TODO: Parallelize this
        execute_commands_helper (c->u.command[0], input_fd, output_fd, close_list, pid_list);
        for (n = pid_list->next; n != pid_list; n = n->next)
          {
            waitpid (n->val, &status, 0);
          }
        while (pid_list->next != pid_list)
          remove_last_element (pid_list);
        execute_commands_helper (c->u.command[1], input_fd, output_fd, close_list, pid_list);
        break;
    }
}

void
execute_command (command_t c, bool time_travel)
{
  node_t n;
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  // Dummy heads for doubly-linked list for close files and pids
  node_t close_list = initialize_llist();
  node_t pid_list = initialize_llist();

  execute_commands_helper (c, STDIN_FILENO, STDOUT_FILENO, close_list, pid_list);

  // Close all leftover open files
  for (n = close_list->next; n != close_list; n = n->next)
    close(n->val);

  // Wait on all of the processes that are still running
  int status = 0, pid_count = 0;
  // Go in the reverse of the list, to get the most recent last
  for (n = pid_list->next; n != pid_list; n = n->next)
    {
      waitpid (n->val, &status, 0);
      pid_count++;
    }

  time_travel = false;
  if (time_travel) { ; }

  c->status = status;
  //error (1, 0, "command execution not yet implemented");
}
