// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include "alloc.h"
#include "ctype.h"
//#include <stdlib.h>
#include <stdio.h>
//#include "string.h"

/*
TODO:
1) Fix the input and output varibles for simple commands
2) Create a parenthesis operation for subshell commands
3) Error handling - double check and make more robust
4) Comments (different state for every state)
*/

enum State
  {
    START,              // Initial State
    COMMENT_START,      // Comment just after the start
    NORMAL,             // Simple Command State
    COMMENT_NORMAL,     // Comment after a normal word
    SPECIAL,            // State just after a backslash (\)
    PIPE,               // State just after a Pipe (|)
    PIPE_SPACE,         // State just after a Pipe and whitespace
    COMMENT_PIPE,       // Comment after a pipe
    OR,                 // State just after an Or (||)
    COMMENT_OR,         // Comment after an or
    AMPERSAND,          // State just after an Ampersand (&)
    AND,                // State just after an And (&&)
    COMMENT_AND,        // Comment after an and
    SEMI_COLON,         // State just after a semicolon (;)
    COMMENT_SEMI_COLON, // Comment just after a semi-colon
    SUBSHELL,           // State while in parenthesis
    AFTER_SUBSHELL,     // State right after a closed parenthesis
    FINAL               // Finish states
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

typedef struct token_array_ {
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

// Error handling
// Global var to keep track of all the newlines
int g_newlines = 0;
void
error_and_message(char * message)
{
  error (1, 0, "%d: %s", g_newlines + 1, message);
}


// Helper functions
int
op_precedence (enum command_type type)
{
  switch (type)
    {
      case SUBSHELL_COMMAND:
        return 4;
      case SIMPLE_COMMAND:
        return 3;
      case PIPE_COMMAND:
        return 2;
      case AND_COMMAND:
      case OR_COMMAND:
        return 1;
      case SEQUENCE_COMMAND:
        return 0;
      default:
        return 172;
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

// String and token operations
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
  if (word->size == 0) // For cases of multiple spaces in a row
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

// Stack operations
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

void
pop_op_stack (operator_stack *op_stack, command_stack *cmd_stack)
{
  // Pop the operator off the operator stack
  op_stack->top--;

  // Create new command of type on top of op_stack
  command_t cmd = (command_t) 
    checked_malloc (sizeof (struct command));
  cmd->type = op_stack->stack[op_stack->top];
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
  cmd_stack->top++;
}

void add_op (enum command_type type, operator_stack *op_stack, 
                command_stack *cmd_stack)
{
  CHECK_GROW(op_stack->stack, op_stack->top, op_stack->capacity);

  // Add logic to pop operators off the stack of less than or equal precedence
  while (op_stack->top > 0 && op_precedence (type) <= 
            op_precedence (op_stack->stack[op_stack->top - 1]))
    {
      pop_op_stack (op_stack, cmd_stack);
    }

  op_stack->stack[op_stack->top] = type;
  op_stack->top++;
}

void
finish_op_stack (operator_stack *op_stack, command_stack *cmd_stack)
{
  while (op_stack->top > 0)
    {
      pop_op_stack (op_stack, cmd_stack);
    }
}

// State functions
void
start_state (char c, enum State *state, string *word)
{
  switch (c)
    {
      case '|':
      case '&':
      case ';':
      case ')':
        //error (1, 0, "Invalid character at start");
        error_and_message ("Invalid character at start");
        break;
      case '\n':
        g_newlines++;
        break;
      case ' ':
      case '\t':
        // Loop back to start state
        break;
      case '(':
        printf("%s", "Start to Subshell");
        *state = SUBSHELL;
        break;
      case '#':
        *state = COMMENT_START;
        break;
      default:
        add_char (word, c);
        *state = NORMAL;
        break;
    }
}

void
normal_state (char c, enum State *state, token_array *tokens, string *word, 
                  command_stack *cmd_stack, size_t depth, bool *in_subshell)
{
  switch (c)
    {
      // TODO: We will add these later
      //case '<': 
      //  break;
      //case '>':
      //  break;
      case '|':
        next_word (tokens, word);
        add_tokens (tokens, cmd_stack);
        *state = PIPE;
        break;
      case '&':
        next_word (tokens, word);
        add_tokens (tokens, cmd_stack);
        *state = AMPERSAND;
        break;
      case ';':
        next_word (tokens, word);
        add_tokens (tokens, cmd_stack);
        *state = SEMI_COLON;
        break;
      //case '\\':
      //  state = SPECIAL;
      //  break;
      case ' ':
      case '\t':
        next_word (tokens, word);
        break;
      case '(':
        printf("%s", "hello world");
        *state = SUBSHELL;
        break;
      case ')':
        if (depth == 0)
          {
            error_and_message("Unexpected close of parenthesis");
          }
        *in_subshell = false;
        *state = FINAL;
        break;
      case '#':
        // If we are not in the middle of a word
        if (word->size == 0)
          {
            add_tokens (tokens, cmd_stack);
            *state = COMMENT_NORMAL;
          }
        else
          {
            add_char(word, c);
          }
        break;
      case '\n':
        g_newlines++;
        next_word (tokens, word);
        add_tokens (tokens, cmd_stack);
        *state = FINAL;
        break;
      default:
        add_char (word, c);
        break;
    }
}

void
comment_state (enum State previous_state, char c, enum State *state)
{
  switch (c)
    {
      case '\n':
        g_newlines++;
        *state = previous_state;
        break;
      default:
        // Do nothing until hitting newline
        break;
    }
}

void
semi_colon_state (char c, enum State *state, string *word,
                        command_stack *cmd_stack, operator_stack *op_stack)
{
  switch (c)
    {
      case '&':
      case '|':
      case ';':
      case ')':
        error_and_message ("Incomplete semicolon");
        break;
      case '\n':
        g_newlines++;
        break;
      case '#':
        *state = COMMENT_SEMI_COLON;
        break;
      case ' ':
      case '\t':
        // Ignore whitespace in this state
        break;
      case '(':
        *state = SUBSHELL;
        break;
      default:
        add_op (SEQUENCE_COMMAND, op_stack, cmd_stack);
        add_char (word, c);
        *state = NORMAL;
        break;
    }
}

void
pipe_state (char c, enum State *state, string *word, 
                  command_stack *cmd_stack, operator_stack *op_stack)
{
  switch (c)
    {
      case '\n':
        g_newlines++;
        break;
      case ' ':
      case '\t':
        add_op (PIPE_COMMAND, op_stack, cmd_stack);
        *state = PIPE_SPACE;
        break;
      case '(':
        *state = SUBSHELL;
        break;
      case '|':
        add_op (OR_COMMAND, op_stack, cmd_stack);
        *state = OR;
        break;
      case '&':
      case ';':
      case ')':
        error_and_message ("Incomplete pipe");
        break;
      case '#':
        *state = COMMENT_PIPE;
        break;
      default:
        add_char (word, c);
        add_op (PIPE_COMMAND, op_stack, cmd_stack);
        *state = NORMAL;
        break;
    }
}

void
pipe_space_state (char c, enum State *state, string *word)
{
  switch (c)
    {
      case '\n':
        g_newlines++;
        break;
      case ' ':
      case '\t':
        break;
      case '(':
        *state = SUBSHELL;
        break;
      case '&':
      case '|':
      case ';':
      case ')':
        error_and_message ("Incomplete Pipe");
        break;
      case '#':
        *state = COMMENT_PIPE;
        break;
      default:
        add_char (word, c);
        *state = NORMAL;
        break;
    }
}

void
or_state (char c, enum State *state, string *word)
{
  switch (c)
    {
      case '\n':
        g_newlines++;
        break;
      case ' ':
      case '\t':
        break;
      case '(':
        *state = SUBSHELL;
        break;
      case '&':
      case '|':
      case ';':
      case ')':
        error_and_message ("Incomplete Or");
        break;
      case '#':
        *state = COMMENT_OR;
        break;
      default:
        add_char (word, c);
        *state = NORMAL;
        break;
    }
}

void
ampersand_state (char c, enum State *state, command_stack *cmd_stack, 
                      operator_stack *op_stack)
{
  switch (c)
    {
      case '&':
        add_op (AND_COMMAND, op_stack, cmd_stack);
        *state = AND;                  
        break;
      default:
        error_and_message ("Incomplete And");
        break;
    }
}

void
and_state (char c, enum State *state, string *word)
{
  switch (c)
    {
      case '\n':
        g_newlines++;
        break;
      case ' ':
      case '\t':
        break;
      case '(':
        *state = SUBSHELL;
        break;
      case '&':
      case '|':
      case ';':
      case ')':
        error_and_message ("Incomplete And");
        break;
      case '#':
        *state = COMMENT_AND;
        break;
      default:
        add_char (word, c);
        *state = NORMAL;
        break;
    }
}

command_t parse_stream(command_stream_t s, size_t depth, bool *in_subshell);

void
subshell_state (enum State *state, command_stack *cmd_stack, size_t depth, 
                              command_stream_t s)
{
  size_t size = 0;
  size_t capacity = 2;
  command_t commands =  (command_t) checked_malloc (2 * sizeof (struct command));
  
  bool in_subshell = true;
  while(in_subshell)
  {
    command_t cmd = parse_stream(s, depth + 1, &in_subshell);
    CHECK_GROW(commands, size, capacity);
    commands[size] = *cmd;
    size++;
  }


  // Create subshell command
  command_t subshell_cmd = (command_t) checked_malloc (sizeof (struct command));
  subshell_cmd->type = SUBSHELL_COMMAND;
  subshell_cmd->status = -1;
  subshell_cmd->input = 0;
  subshell_cmd->output = 0;
  subshell_cmd->u.subshell_command = commands;

  print_command(subshell_cmd);
  // Add subshell command to command stack
  CHECK_GROW (cmd_stack->stack, cmd_stack->top, cmd_stack->capacity);
  cmd_stack->stack[cmd_stack->top] = subshell_cmd;
  cmd_stack->top++;

  *state = AFTER_SUBSHELL;
}

void
after_subshell_state (char c, enum State *state, size_t depth, bool *in_subshell)
{
  switch (c)
    {
      case '\n':
        g_newlines++;
        break;
      case '\t':
      case ' ':
        break; 
      case '|':
        *state = PIPE;
        break;
      case '&':
        *state = AMPERSAND;
        break;
      case ';':
        *state = SEMI_COLON;
        break;
      case ')':
        if(depth == 0)
          error_and_message("Unexpected close parenthesis");
        else
        {
          *in_subshell = false;
          *state = FINAL;
        }
      default:
        error_and_message("Invalid token after closed parenthesis");
        break;
    }
}

command_t
read_command_stream (command_stream_t s)
{
  bool subshell = true;
  return parse_stream(s, 0, &subshell);
}

command_t
parse_stream(command_stream_t s, size_t depth, bool *in_subshell)
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

  while (state != FINAL)
    {
      int i = 0;
      if (state != SUBSHELL)
        i = GET(s);
      if (i < 0)
        {
          if(state != NORMAL && state != START)
            {
              error_and_message ("Error on last line");
              return 0;
            }
          state = FINAL;
          continue;
        }
      
      char c = (char)i;

      // Check state
      switch (state) 
        {
          case START:
            start_state (c, &state, &word);
            break;
          case COMMENT_START:
            comment_state (START, c, &state);
            break;
          case NORMAL:
            normal_state (c, &state, &tokens, &word, &cmd_stack, depth, in_subshell);
            break;
          case COMMENT_NORMAL:
            // Special case of comment since after the newline it goes 
            //   back to final state
            comment_state (FINAL, c, &state);
            break;
          case SPECIAL:
            if (c != '\n')
              add_char (&word, c);
            break;
          case SEMI_COLON:
            semi_colon_state (c, &state, &word, &cmd_stack, &op_stack);
            break;
          case PIPE:
            pipe_state (c, &state, &word, &cmd_stack, &op_stack);
            break;
          case PIPE_SPACE:
            pipe_space_state (c, &state, &word);
            break;
          case COMMENT_PIPE:
            comment_state (PIPE_SPACE, c, &state);
            break;
          case OR:
            or_state (c, &state, &word);
            break;
          case COMMENT_OR:
            comment_state (OR, c, &state);
            break;
          case AMPERSAND:
            ampersand_state (c, &state, &cmd_stack, &op_stack);
            break;
          case AND:
            and_state (c, &state, &word);
            break;
          case COMMENT_AND:
            comment_state (AND, c, &state);
            break;
          case SUBSHELL:
            printf("%s", "hello world");
            subshell_state (&state, &cmd_stack, depth, s);
            break;
          case AFTER_SUBSHELL:
            after_subshell_state(c, &state, depth, in_subshell);
            break;
          default:
            break;
        }
    }

  finish_op_stack (&op_stack, &cmd_stack);
  //error (1, 0, "command reading not yet implemented");
  return cmd_stack.stack[0];
}
