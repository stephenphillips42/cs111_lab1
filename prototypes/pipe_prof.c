#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/**
 * Executes the command "cat scores | grep Villanova".  In this quick-and-dirty
 * implementation the parent doesn't wait for the child to finish and
 * so the command prompt may reappear before the child terminates.
 *
 * @author Jim Glenn
 * @version 0.1 9/23/2004
 */

#define IN_HALF 0
#define OUT_HALF 1

int main(int argc, char **argv)
{
  int pipefd[2];
  int pid;

  char *cat_args[] = {"cat", "scores", NULL};
  char *grep_args[] = {"grep", "Villanova", NULL};

  // make a pipe (fds go in pipefd[IN_HALF] and pipefd[OUT_HALF])

  pipe(pipefd);

  pid = fork();

  if (pid == 0)
    {
      // child gets here and handles "grep Villanova"

      // replace standard input with input part of pipe

      dup2(pipefd[IN_HALF], STDIN_FILENO);

      // close unused hald of pipe

      close(pipefd[OUT_HALF]);

      // execute grep

      execvp("grep", grep_args);
    }
  else
    {
      // parent gets here and handles "cat scores"

      // replace standard output with output part of pipe

      dup2(pipefd[OUT_HALF], STDOUT_FILENO);

      // close unused unput half of pipe

      close(pipefd[IN_HALF]);

      // execute cat

      execvp("cat", cat_args);
    }
}