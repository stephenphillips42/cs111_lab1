#include "llist.h"
#include "alloc.h"
#include "stdlib.h"

node_t
initialize_llist ()
{
  node_t head = checked_malloc (sizeof (node));
  head->val = -1;
  head->next = head;
  head->previous = head;

  return head;
}

void
insert_node (node_t head, int val)
{
  node_t insert = checked_malloc (sizeof (node));
  insert->val = val;

  insert->previous = head->previous;
  insert->previous->next = insert;
  head->previous = insert;
  insert->next = head;
}

void
remove_last_element (node_t head)
{
  node_t last = head->previous;
  last->previous->next = head;
  head->previous = last->previous;
  free (last);
}
