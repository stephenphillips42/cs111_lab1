// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "alloc.h"
#include "command.h"
#include "parallel.h"

static char const *program_name;
static char const *script_name;

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
      int status;

      size_t arr_size = 0;
      size_t arr_capacity = 2;
      cmd_arr = checked_malloc (arr_capacity * sizeof (command_array));

      while ((command = read_command_stream (command_stream)))
        {
          add_command_t (command, &cmd_arr, &arr_size, &arr_capacity);
        }
        status = excute_parallel (cmd_arr, arr_size);
        // Free all the commands
        for (i = 0; i < arr_size; i++)
          {
            free_command_array_dependents (cmd_arr[i]);
          }
      free (cmd_arr);
      return status;
    }
  else
    {
      while ((command = read_command_stream (command_stream)))
        {
          execute_command (command);
          test = command_status (command);
          free_command (command);
        }
      return test;
    }

}
