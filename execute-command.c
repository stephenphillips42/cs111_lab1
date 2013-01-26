// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "file_tree.h"

#include <error.h>

// DEBUG
#include "stdio.h"

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
        get_files(cmd->u.subshell_command, head);
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
  free_file_tree (&files);
  printf ("Exiting...\n");
  time_travel = false;
  if (time_travel) { ; }

  //error (1, 0, "command execution not yet implemented");
}
