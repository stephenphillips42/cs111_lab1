#ifndef FILE_TREE_H
#define FILE_TREE_H

// Struct to organize files in different commands
struct file_tree_struct {
  char *filename; // Path
  int fd; // file descriptor
  int count; // Number of times file is used
  char written_to; // File written to flag
  // Binary Tree pointers
  struct file_tree_struct *left;
  struct file_tree_struct *right;
};
typedef struct file_tree_struct* file_tree;

// Binary tree ops
file_tree create_file_tree (char*, char);
int insert_file_tree (file_tree*, char*, char);
void free_file_tree (file_tree*);
void print_file_tree(file_tree);
int get_file_desc (file_tree, char*);
#endif
