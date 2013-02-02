#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "command-internals.h"
#include "alloc.h"
#include "parallel.h"
#include "file_tree.h"
#include "llist.h"

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
        if (cmd->input)
          insert_file_tree (head, cmd->input, 0);
        if (cmd->output)
          insert_file_tree (head, cmd->output, 1);
        get_files (cmd->u.subshell_command, head);
        break;
    }
}

bool
files_dependent (file_tree files1, file_tree files2)
{
  if (!files1 || !files2)
    return false; // Can't search null values

  if (files_dependent (files1->left, files2))
    return true;
  else if (files_dependent (files1->right, files2))
    return true;
  else 
    {
      file_tree search = find_file (files2, files1->filename);
      // Return false if we didn't find anything
      if (search == 0)
        return false;
      else // Otherwise we check if one of them is being written to
        return (files1->written_to || files2->written_to);
    }
}

bool
dependent_commands_arrs (command_array cmd_arr1, command_array cmd_arr2)
{
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
  int i;

  // Shift everything to match ranking
  // This is definitely not the most efficient sort. However, if we don't place
  //  commands into their proper position, we could get less optimal scheduling

  i = (int)(arr_size);
  while (0 < i && cmd_info->ranking < (*cmd_arr)[i-1].ranking)
  {
    (*cmd_arr)[i] = (*cmd_arr)[i-1];
    i--;
  }
  // initialize the values of the command
  (*cmd_arr)[i] = *cmd_info;

  #ifdef DEBUG
    int j;
    for (j = 0; j != (int)arr_size+1; j++) {
      printf ("%d -- cmd: %x, files: %x, rank: %d\n",
        j, 
        (unsigned int)(*cmd_arr)[j].command_tree, 
        (unsigned int)(*cmd_arr)[j].files,
        (*cmd_arr)[j].ranking);
      print_command ((*cmd_arr)[j].command_tree);
    }
    printf("FINAL ENDED\n\n\n\n");
  #endif
}

// Adds a command to the list of processes, ranked by parallelizability
void
add_command_t (command_t cmd, command_array **cmd_arr,  
          size_t *arr_size, size_t *arr_capacity)
{
  command_array cmd_info;

  cmd_info.command_tree = cmd;

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
excute_parallel (command_array *cmd_arr, size_t arr_size)
{
  int status;
  size_t i;
  node_t n;
  node_t pid_list;
  
  // List of pids to wait for
  pid_list = initialize_llist ();

  i = 0;
  // Go through all the processes
  while (i < arr_size)
    {
      int pid;
      size_t current = cmd_arr[i].ranking;
      while (i < arr_size && cmd_arr[i].ranking == current)
        {
          pid = fork();
          // Spawn the child process to execute the command
          if (pid == 0) // child process
            {
              execute_command (cmd_arr[i].command_tree);
              exit (command_status (cmd_arr[i].command_tree));
            }
          insert_node (pid_list, pid);
          i++;
        }
      // Wait for all the processes of this ranking to finish
      for (n = pid_list->next; n != pid_list; n = n->next)
        waitpid(n->val, &status, 0);
      // Processes finished waiting for
      while (pid_list != pid_list->next)
        remove_last_element (pid_list);
    }
  // Return the status of the last process waited on
  return status;
}

// Frees the command arrays commands and file tree
void free_command_array_dependents (command_array cmd_arr)
{
	free_command (cmd_arr.command_tree);
    free_file_tree (cmd_arr.files);
}