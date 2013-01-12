// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include "alloc.h"
#include "ctype.h"
#include "stdlib.h"

#include "stdio.h"

enum State
  {
    START,       // Initial State
    NORMAL,      // Simple Command State
    PIPE,        // State just after a Pipe (|)
    PIPE_SPACE,  // State in 
    OR,          // State just after an Or (||)
    AMPERSAND,   // State just after an Ampersand (&)
    AND,         // State just after an And (&&)
    SEMI_COLON,  // State just after a semicolon (;)
    FINAL        // Finish states
  };

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream {
  int (*get) (void *);
  void * arg;
};

#define GET(cmd_strm) cmd_strm->get(cmd_strm->arg)
#define CHECK_GROW(arr, size, capacity) \
    if (capacity < size) \
      { \
        checked_grow_alloc ( (void *) arr, &capacity); \
      }

// Need this?
bool 
is_simple_cmd_char (char i) {
  return isalnum(i) || i == '!' || i == '+' || i == ',' || i == '-' 
      || i == '.' || i == '/' || i == ':' || i == '@' || i == '^' || i == '_';
  // Switch table? Faster?
}

command_stream_t
make_command_stream (int (*get_byte) (void *),
         void *arg)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  //error (1, 0, "command reading not yet implemented");
  command_stream_t s = 
    (command_stream_t) checked_malloc (sizeof (struct command_stream));
  s->get = get_byte;
  s->arg = arg;
  return s;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  // State of the Parser
  enum State state = START;

  // Size variables of the stacks and arrays
  size_t tokens_size = 0;
  size_t tokens_capacity = 8;
  size_t word_size = 0;
  size_t word_capacity = 16;

  size_t cmdstack_top = 0;
  size_t cmdstack_capacity = 8;
  size_t opstack_top = 0;
  size_t opstack_capacity = 8;
  
  // Token arrays
  char **tokens = (char **) checked_malloc (tokens_capacity * sizeof (char *));
  char *word = (char *) checked_malloc (word_capacity * sizeof (char));

  // Stacks
  command_t *cmd_stack = (command_t *) 
      checked_malloc (cmdstack_capacity * sizeof (command_t));
  enum command_type *op_stack = (enum command_type *) 
      checked_malloc (opstack_capacity * sizeof (enum command_type));

  while(state != FINAL)
    {
      int i = GET(s);
      if (i <= 0)
        {
          error (1, 0, "Error in standard input");
        }
      
      char c = (char)i;
      CHECK_GROW(word, word_size, word_capacity);

      // Check state
      switch(state) 
        {
          case START:
            break;
          case NORMAL:
            break;
          case PIPE:
            break;
          case OR:
            break;
          case AMPERSAND:
            break;
          case AND:
            break;
          case SEMI_COLON:
            break;
          default:
            break;
        }
      
    }

  //error (1, 0, "command reading not yet implemented");
  return 0;
}
