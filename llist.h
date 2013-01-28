#ifndef LLIST_HEADER
#define LLIST_HEADER

typedef struct node_
{
  int val;
  struct node_ *next;
  struct node_ *previous;
} node;

typedef node* node_t;

node_t initialize_llist ();

void insert_node (node_t head, int val);

void remove_last_element (node_t head);

#endif
