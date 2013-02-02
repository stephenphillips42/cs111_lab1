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
  // TEMPORARY!!
  size_t index;
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
files_dependent (file_tree files1, file_tree files2)
{
  if (!files1 || !files2)
    return false; // Can't search null values

  if (!files_dependent (files1->left, files2) &&
        !files_dependent (files1->right, files2))
    {
      file_tree search = find_file (files2, files1->filename);
      // Return false if we didn't find anything
      if (search == 0)
        return false;
      else // Otherwise we check if one of them is being written to
        return (files1->written_to || files2->written_to);
    }
  else
    return true;
}

bool
dependent_commands_arrs (command_array cmd_arr1, command_array cmd_arr2)
{
  //(void) cmd_arr1, (void) cmd_arr2;

  return files_dependent (cmd_arr1.files, cmd_arr2.files);
}

// Finds ranking of cmd in cmd_arr. Assumes cmd_arr is non-zero sized
size_t
find_ranking (command_array *cmd_info, command_array **cmd_arr, size_t arr_size)
{
  int position;
  position = arr_size - 1;
  // Find the first dependent command
  while (0 <= position && 
          !dependent_commands_arrs (*cmd_info, (*cmd_arr)[position]))
    {
      position--;
    }
  // Found the first command it is dependent on

  // Special case for -1
  if (position == -1)
  {
    // Not dependent on anything
    return 0;
  }

  // The ranking is one more than the first dependent command
  return (*cmd_arr)[position].ranking + 1;
}

// Changes cmd_arr to have the cmd_info in its proper place (assumes
//  cmd_info has already been completely filled)
void
place_command_by_ranking (command_array *cmd_info, command_array **cmd_arr, 
                          size_t arr_size)
{
  int i, j;

  // Shift everything to match ranking
  // This is definitely not the most efficient sort. However, if we don't place
  //  commands into their proper position, we could get less optimal scheduling

  printf("Rank of this one: %d\n", cmd_info->ranking);
  i = (int)(arr_size);
  while (0 < i && cmd_info->ranking < (*cmd_arr)[i-1].ranking)
  {
    (*cmd_arr)[i] = (*cmd_arr)[i-1];
    for (j = 0; j != (int)arr_size; j++) {
      printf ("%d -- cmd: %x, files: %x, rank: %d\n",
        j, 
        (unsigned int)(*cmd_arr)[j].command_tree, 
        (unsigned int)(*cmd_arr)[j].files,
        (*cmd_arr)[j].ranking);
      print_command ((*cmd_arr)[j].command_tree);
    }
    printf("ENDED\n\n\n\n");
    i--;
  }
  // initialize the values of the command
  (*cmd_arr)[i] = *cmd_info;

    for (j = 0; j != (int)arr_size+1; j++) {
      printf ("%d -- cmd: %x, files: %x, rank: %d\n",
        j, 
        (unsigned int)(*cmd_arr)[j].command_tree, 
        (unsigned int)(*cmd_arr)[j].files,
        (*cmd_arr)[j].ranking);
      print_command ((*cmd_arr)[j].command_tree);
    }
    printf("FINAL ENDED\n\n\n\n");
  #ifdef DEBUG
  #endif
}

int random_global = 1;
void
add_command_t (command_t cmd, command_array **cmd_arr,  
          size_t *arr_size, size_t *arr_capacity)
{
  command_array cmd_info;

  cmd_info.command_tree = cmd;
  cmd_info.index = random_global++;

  // Make the File Tree
  file_tree head = 0;
  get_files (cmd_info.command_tree, &head);
  cmd_info.files = head;

  // Check Size of the array, allocate more memory if needed
  if (*arr_capacity <= *arr_size)
    {
      size_t new_capacity = (*arr_capacity) * (sizeof (command_array));
      *cmd_arr = checked_grow_alloc (*cmd_arr, &new_capacity);
      *arr_capacity = new_capacity / (sizeof (command_array));
    }

  // Initialize new command_array element's command tree
  if (*arr_size != 0)
    {
      cmd_info.ranking = find_ranking (&cmd_info, cmd_arr, *arr_size);
      place_command_by_ranking (&cmd_info, cmd_arr, *arr_size);
    }
  else // The first command is always independent
    {
      cmd_info.ranking = 0;
      (*cmd_arr)[*arr_size] = cmd_info;
    }

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
          //printf("This is the thing\n");
          add_command_t (command, &cmd_arr, &arr_size, &arr_capacity);
        }
      for (i = 0; i < arr_size; i++)
        {
          printf ("# Index: %d, Rank: %d\n", cmd_arr[i].index, cmd_arr[i].ranking);
          print_command (cmd_arr[i].command_tree);
          //print_file_tree (cmd_arr[i].files);
          free_command (cmd_arr[i].command_tree);
          free_file_tree (cmd_arr[i].files);
        }
      free (cmd_arr);
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
