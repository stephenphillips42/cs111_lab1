#include "llist.h"
#include "stdlib.h"

#include "stdio.h"

// For debugging
void
print_llist (node_t head)
{
  /*node_t n;
  for (n = head->next; n != head; n = n->next)
    printf("Node: %d\n", n->val);
  printf("Node: %d\n\n", n->val);
*/
}

node_t initialize_llist ()
{
  // Create dummy head of linked list
  node_t head = (node_t) malloc (sizeof (node));
  head->val = -1; // Doesn't matter what val is

  head->next = head;
  head->previous = head;

  return head;
}

node_t
insert_end_node (node_t llist, int val)
{
  node_t insert = (node_t) malloc (sizeof (node));
  insert->val = val;

  insert->previous = llist->previous;
  insert->previous->next = insert;

  llist->previous = insert;
  insert->next = llist;

  return insert;
}

int
remove_last_element (node_t llist)
{
  if (llist->previous = llist)
    return 0;
  
  node_t last = llist->previous;
  last->previous->next = llist;
  llist->previous = last->previous;
  free (last);

  return 1;
}

void
clear_llist (node_t llist)
{
  while(remove_last_element (llist))
    {
      // Do nothing
    }
}
