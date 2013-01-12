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
    SPECIAL,     // State just after a backslash (\)
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

typedef struct string_ {
  char * arr;
  size_t size;
  size_t capacity;
} string;

typedef struct token_array_
{
  char **arr;
  size_t size;
  size_t capacity;
} token_array;

typedef struct command_stack_ {
  command_t *stack;
  size_t top;
  size_t capacity;
} command_stack;

typedef struct operator_stack_ {
  enum command_type *stack;
  size_t top;
  size_t capacity;
} operator_stack;

#define GET(cmd_strm) cmd_strm->get(cmd_strm->arg)
#define CHECK_GROW(arr, size, capacity) \
    if (capacity < size) \
      { \
        checked_grow_alloc ((void *) arr, &capacity); \
      }

// Need this?
bool 
is_simple_cmd_char (char i) 
{
  return isalnum(i) || i == '!' || i == '+' || i == ',' || i == '-' 
      || i == '.' || i == '/' || i == ':' || i == '@' || i == '^' || i == '_';
  // Switch  table? Faster?
}

int
op_precedence (enum command_type type)
{
  switch (type)
    {
      case AND_COMMAND:
        return 0;
      case SEQUENCE_COMMAND:
        return 0;
      case OR_COMMAND:
        return 0;
      case PIPE_COMMAND:
        return 0;
      case SIMPLE_COMMAND:
        return 0;
      case SUBSHELL_COMMAND:
        return 0;
      default:
        return 0;
    }
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

void
add_char (string *str, char c)
{
  CHECK_GROW(str->arr, str->size, str->capacity);

  str->arr[str->size] = c;
  str->size++;
}

void
next_word (token_array *tokens, string *word)
{
  if(word->size == 0) // For cases of multiple spaces in a row
    return;

  CHECK_GROW(word->arr, word->size, word->capacity)

  word->arr[word->size] = 0;

  CHECK_GROW(tokens->arr, tokens->size, tokens->capacity)

  tokens->arr[tokens->size] = word->arr;
  tokens->size++;

  word->size = 0;
  word->capacity = 16;
  word->arr = (char *) checked_malloc (word->capacity * sizeof (char));
}

void
add_tokens (token_array *tokens, command_stack *cmd_stack)
{
  CHECK_GROW(tokens->arr, tokens->size, tokens->capacity);

  tokens->arr[tokens->size] = 0;

  CHECK_GROW(cmd_stack->stack, cmd_stack->top, cmd_stack->capacity);

  command_t cmd = (command_t) checked_malloc (sizeof (struct command));
  cmd->type = SIMPLE_COMMAND;
  cmd->status = -1;
  cmd->u.word = tokens->arr;
  // This is the part that we will implement later with input and output
  cmd->input = 0;
  cmd->output = 0;
  
  // Reset Tokens
  tokens->size = 0;
  tokens->capacity = 8;
  tokens->arr = (char **) checked_malloc (tokens->capacity * sizeof (char *));

  cmd_stack->stack[cmd_stack->top] = cmd;
  cmd_stack->top++;
}

void add_op (enum command_type type, operator_stack *op_stack, 
                command_stack *cmd_stack)
{
  CHECK_GROW(op_stack->stack, op_stack->top, op_stack->capacity);

  // Add logic to pop operators off the stack of less than or equal precedence
  while (op_stack->top > 0 && op_precedence (type) <= 
            op_precedence (op_stack->stack[op_stack->top]))
    {

      // Create new command of type on top of op_stack
      command_t cmd = (command_t) 
        checked_malloc (sizeof (struct command));
      cmd->type = type;
      cmd->status = -1;
      cmd->input = 0;
      cmd->output = 0; // TODO: Fix these if needed

      // Pop the top two commands off the stack
      cmd_stack->top--;
      cmd->u.command[1] = cmd_stack->stack[cmd_stack->top];
      cmd_stack->top--;
      cmd->u.command[0] = cmd_stack->stack[cmd_stack->top];

      // Push the newly created command on the top
      cmd_stack->stack[cmd_stack->top] = cmd;
      print_command(cmd);
      cmd_stack->top++;

      // Pop the operator off the operator stack
      op_stack->top--;
    }

  op_stack->stack[op_stack->top] = type;
  op_stack->top++;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  // State of the Parser
  enum State state = START;

  // Token arrays
  token_array tokens;
  tokens.size = 0;
  tokens.capacity = 8;
  tokens.arr = (char **) checked_malloc (tokens.capacity * sizeof (char *));

  string word;
  word.size = 0;
  word.capacity = 16;
  word.arr = (char *) checked_malloc (word.capacity * sizeof (char));

  // Stacks
  command_stack cmd_stack;
  cmd_stack.top = 0; 
  cmd_stack.capacity = 16;
  cmd_stack.stack = 
      checked_malloc (cmd_stack.capacity * sizeof (command_t));

  operator_stack op_stack;
  op_stack.top = 0;
  op_stack.capacity = 8;
  op_stack.stack = (enum command_type *) 
      checked_malloc (op_stack.capacity * sizeof (enum command_type));


  while(state != FINAL)
    {
      int i = GET(s);
      if (i < 0)
        {
          printf("%d\n", i);
          printf("%d\n", cmd_stack.top);
          error (1, 0, "Error in standard input");
        }
      else if(i == 0)
        {
          error (1, 0, "End of file");
        }
      
      char c = (char)i;

      // Check state
      switch (state) 
        {
          case START:
            switch (c)
              {
                case '|':
                case '&':
                case ';':
                  error (1, 0, "Invalid character at start");
                  break;
                case ' ':
                case '\t':
                case '\n':
                  // Loop back to original state
                  break;
                default:
                  add_char (&word, c);
                  state = NORMAL;
                  break;
              }
            break;
          case NORMAL:
            switch (c)
              {
                // TODO: We will add these later
                //case '<': 
                //  break;
                //case '>':
                //  break;
                //case '|':
                //  state = PIPE;
                //  return 0;
                //  break;
                //case '&':
                //  state = AMPERSAND;
                //  return 0;
                //  break;
                //case ';':
                //  state = SEMI_COLON;
                //  return 0;
                //  break;
                //case '\\':
                //  state = SPECIAL;
                //  break;
                case ' ':
                case '\t':
                  next_word (&tokens, &word);
                  break;
                case '\n':
                  next_word (&tokens, &word);
                  add_tokens (&tokens, &cmd_stack);
                  state = SEMI_COLON;
                  break;
                default:
                  add_char (&word, c);
                  break;
              }
            break;
          case SPECIAL:
            if(c != '\n')
              add_char(&word, c);
            break;
          case SEMI_COLON:
            switch (c)
              {
                // TODO: Deal with error and special cases later
                case ' ':
                case '\t':
                case '\n':
                  // Ignore whitespace in this state
                  break;
                default:
                  add_op (SEQUENCE_COMMAND, &op_stack, &cmd_stack);
                  add_char (&word, c);
                  state = NORMAL;
                  break;
              }
            break;
          case PIPE:
            break;
          case PIPE_SPACE:
            break;
          case OR:
            break;
          case AMPERSAND:
            break;
          case AND:
            break;
          default:
            break;
        }
    }

  //error (1, 0, "command reading not yet implemented");
  return 0;
}
