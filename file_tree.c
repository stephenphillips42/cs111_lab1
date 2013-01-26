#include "file_tree.h"
#include "string.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "alloc.h"

#define DEBUG
#ifdef DEBUG
#include "stdio.h"
#endif

file_tree
create_file_tree (char *filename, char written_to)
{
  file_tree tree = checked_malloc (sizeof (struct file_tree_struct));
  tree->filename = filename;
  tree->fd = open (filename, O_CREAT);
  
  tree->count = 1;
  tree->written_to = written_to;
  tree->left = tree->right = 0;

  printf ("%s %d %d\n", filename, written_to, tree->fd);

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
free_file_tree(file_tree *head)
{
  if(*head == 0)
    return;

  if ((*head)->count > 0)
    close ((*head)->fd);

  free_file_tree (&(*head)->left);
  free_file_tree (&(*head)->right);
  free(*head);
}

