// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "file_tree.h"

#include "string.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <error.h>
#include <stdlib.h>

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

int
execute_simple_command (command_t c)
{
        int status;

        // Looks for whether or not there are inputs and outputs
        if(c->input || c->output)
          {
            printf ("Not implemented yet\n");
            return -1;
          }

        // Forks the process so that the child process may run the
        // command that was specified using execvp
        pid_t p = fork ();
        if (p < 0)
          { exit(1); } // Error
        if (p == 0)
          {
            // In child process
            execvp (c->u.word[0], c->u.word);
            exit(120); // Which exit status?
          }
        
        // Wait for child process to exit
        while (waitpid (p, &status, 0) < 0)
          continue;

        // Returns status of command execution
        c->status = status;
        return status;
}

int
execute_command_helper (command_t c)
{
  printf("entering...\n");
  int status;
  switch (c->type)
    {
      case AND_COMMAND:
        // Recursively runs through command tree to find simple
        // executable commands. If the first command is successful,
        // run the second command. If there is an error, return an error
        // status. If both commands run successfully, return 0.
        status = execute_command_helper (c->u.command[0]);
        if (status)
            exit_with_status (status);
        return execute_command_helper (c->u.command[1]);
        break;
      case SEQUENCE_COMMAND:
        // TODO: This will be parallelized in the future
        execute_command_helper (c->u.command[0]);
        // TODO: What is the proper behavior here?
        // if (status)
        //   exit_with_status (status); 
        return execute_command_helper (c->u.command[1]);
        break;
      case OR_COMMAND:
        // Recursively runs through command tree to find simple
        // executable commands. If the first command is not successful,
        // run the second command. If both are not succesful, return error.
        // Otherwise, return 0. 
        status = execute_command_helper (c->u.command[0]);
        if (status)
          return execute_command_helper (c->u.command[1]);
        else
          return status;
        break;
      case PIPE_COMMAND:
        printf ("Not implemented yet\n");
        break;
      case SIMPLE_COMMAND:
        status = execute_simple_command(c);
        break;
      case SUBSHELL_COMMAND:
        return execute_command_helper (c->u.subshell_command);
        break;
      default:
        break;
    }
    printf("Error: Unknown type of command\n");
    exit(1);
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

  execute_command_helper (c);

  time_travel = false;
  if (time_travel) { ; }

  //error (1, 0, "command execution not yet implemented");
}
