#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "string.h"
#include "alloc.h"
#include "file_tree.h"

#define DEBUG
#ifdef DEBUG
#include "stdio.h"
#endif

file_tree
create_file_tree (char *filename, char written_to)
{
  file_tree tree = checked_malloc (sizeof (struct file_tree_struct));
  tree->filename = filename;
  
  tree->count = 1;
  tree->written_to = written_to;
  tree->left = tree->right = 0;

  return tree;
}

// This inserts or updates a file tree (0 = insert, 1 = update)
int
insert_file_tree (file_tree *head, char *filename, char written_to)
{
  if (*head == 0)
    {
      *head = create_file_tree(filename, written_to);
      return 0;
    }
  int cmp = strcmp (filename, (*head)->filename);
  if (cmp < 0)
    return insert_file_tree (&(*head)->left, filename, written_to);
  else if (cmp > 0)
    return insert_file_tree (&(*head)->right, filename, written_to);
  else
    {
      (*head)->count++;
      (*head)->written_to |= written_to;
      return 1;
    }
}

void
free_file_tree (file_tree head)
{
  if(head == 0)
    return;

  // The words are freed by the commands, so we don't free them here

  free_file_tree (head->left);
  free_file_tree (head->right);
  free(head);
}

void
print_indented_file_tree (int indent, file_tree head)
{
  if(head->left)
    {
      print_indented_file_tree (indent + 2, head->left);
      printf("%*s/", indent + 2, "");
    }

  printf (" \n%*s%s, wrtn: %d, cnt: %d\n", indent, "", 
    head->filename, head->written_to, head->count);
  
  if(head->right)
    {
      printf("%*s\\", indent + 2, "");
      print_indented_file_tree (indent + 2, head->right);
    }
}

void
print_file_tree(file_tree head)
{
  if(head)
    print_indented_file_tree (0, head);
}

file_tree find_file (file_tree head, char const *filename)
{
  if (!head)
    return 0; // Have not found it, return 0

  int cmp = strcmp(head->filename, filename);
  if (cmp < 0)
    return find_file (head->left, filename);
  else if (0 < cmp)
    return find_file (head->right, filename);
  else // found the node
    return head;
}
