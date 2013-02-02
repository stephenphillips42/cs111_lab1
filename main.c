// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "alloc.h"
#include "file_tree.h"
#include "command.h"
#include "command-internals.h"

static char const *program_name;
static char const *script_name;

typedef struct command_array_{
  command_t command_tree;
  file_tree files;
  size_t ranking;
} command_array;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

void
get_files (command_t cmd, file_tree *head)
{
  char **word;
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
        word = cmd->u.word;
        while (*word)
          {
            insert_file_tree (head, *word, 0);
            word++;
          }
        break;
      case SUBSHELL_COMMAND:
        get_files (cmd->u.subshell_command, head);
        break;
    }
}

bool
dependent_commands (command_array cmd_arr1, command_array cmd_arr2)
{
  return 1;
}

void
add_command_t (command_t cmd, command_array **cmd_arr,  
          size_t *arr_size, size_t *arr_capacity)
{
  // Check Size of the array, allocate more memory if needed
  if (*arr_capacity <= *arr_size)
    {
      size_t new_capacity = (*arr_capacity) * (sizeof (command_array));
      *cmd_arr = checked_grow_alloc (*cmd_arr, &new_capacity);
      *arr_capacity = new_capacity / (sizeof (command_array));
    }

  // Initialize new command_array element's command tree
  (*cmd_arr)[*arr_size].command_tree = cmd;

  // Make the File Tree
  file_tree head = 0;
  get_files (cmd, &head);
  (*cmd_arr)[*arr_size].files = head;


  // TODO: Ranking Initialization

  // Change Array Size
  (*arr_size)++;
}

int
main (int argc, char **argv)
{
  int command_number = 1;
  bool print_tree = false;
  bool time_travel = false;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "pt"))
      {
      case 'p': print_tree = true; break;
      case 't': time_travel = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);

  command_t command;
  command_array *cmd_arr;
  size_t arr_size = 0;
  size_t arr_capacity = 2;
  cmd_arr = checked_malloc (arr_capacity * sizeof (command_array));
  bool test;

  if (print_tree)
    {
      while ((command = read_command_stream (command_stream)))
        {
          printf ("# %d\n", command_number++);
          print_command (command);
        }
      return 0;
    }
  else if (time_travel)
    {
      size_t i;
      cmd_arr = (command_array *) checked_malloc (arr_capacity * (sizeof (command_array)));
      while ((command = read_command_stream (command_stream)))
        {
          add_command_t (command, &cmd_arr, &arr_size, &arr_capacity);
        }
      for (i = 0; i < arr_size; i++)
        {
          printf ("# %d\n", command_number++);
          print_command (cmd_arr[i].command_tree);
          print_file_tree (cmd_arr[i].files);
          free_command (cmd_arr[i].command_tree);
          free_file_tree (cmd_arr[i].files);
        }
      return 0;
    }
  else
    {
      while ((command = read_command_stream (command_stream)))
        {
          execute_command (command, time_travel);
          test = command_status (command);
          free_command (command);
        }
      return test;
    }

}
