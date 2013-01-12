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

char *
read_line (command_stream_t s) 
{
  size_t line_capacity = 64;
  size_t line_size = 0;
  char * line = (char *) checked_malloc (64 * sizeof (char));
  
  // Read the line
  int i;
  i = GET(s);
  while(i > 0 && i != '\n')
    {
      char c = (char)i;
      CHECK_GROW(line, line_size, line_capacity);

      line[line_size] = c;
      line_size++;
      i = GET(s);
    }

  // Error checking
  if(i < 0)
    {
      error (1, 0, "Error in standard input");
    }
  // Should we do this error check?
  if(line_size == 0)
    {
      free (line);
      return 0;
    }

  // Terminate with null byte
  CHECK_GROW(line, line_size, line_capacity);
  line[line_size] = 0;

  printf("%s\n", line);
  return line;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  size_t total_size = 0;
  //size_t total_capacity = 8;
  size_t token_size = 0;
  size_t token_capacity = 16;
  
  char ** tokens = (char **) checked_malloc (8 * sizeof (char *));
  tokens[total_size] = (char *) checked_malloc (16 * sizeof (char));
  char * line;
  while(!(line = read_line(s)));

  int i;
  for(i = 0; line[i]; i++)
    {
      switch(line[i])
        {
          case ';':
          case '|':
          case '&':
          case '>':
          case '<':
          case '(':
          case ')':
            printf ("SPECIAL!!");
            break;
          case ' ':
          case '\t':
          case '\r':
            break;
          default:
            CHECK_GROW(tokens[total_size], token_size, token_capacity);
            tokens[total_size][token_size] = line[i];
            token_size++;
        }
    }
  printf("%s\n", tokens[total_size]);


  //error (1, 0, "command reading not yet implemented");
  return 0;
}
