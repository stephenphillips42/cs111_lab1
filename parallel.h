#ifndef PARALLEL_COMMAND_HEADER
#define PARALLEL_COMMAND_HEADER

#include "command.h"
#include "file_tree.h"

typedef struct command_array_ {
  command_t command_tree;
  file_tree files;
  size_t ranking;
} command_array;

// Adds a command to the list of processes, ranked by parallelizability
void add_command_t (command_t, command_array**, size_t*, size_t*);

// Executes all the commands in the command array in parallel
int excute_parallel (command_array*, size_t);

void free_command_array_dependents (command_array);

#endif