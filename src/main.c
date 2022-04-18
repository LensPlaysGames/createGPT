#include <stdbool.h>
#include <stdio.h>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  // TODO: Write a valid GPT header to a file :shrug: .

  /* THE IDEA:
   * |-- Open a file for writing (at some path, probably given)
   * |   This will be the disk image file that with GPT partitions.
   * |   I guess I should only do this if there are no errors during any other part.
   * |   I'll use STATE.Write to determine whether or not
   * |-- 
   */

  return 0;
}
