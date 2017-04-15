#include "usertraps.h"
#include "misc.h"

void main (int argc, char *argv[])
{
  int * arr, arr2, arr3, arr4;
  int i, size;

  Printf("Test 1: Normal malloc (256) size)\n");
  arr = malloc(256);
  for (i = 0; i < (64); i++) { arr[i] = i; }
  for (i = 0; i < (64); i++)
  {
    if((i > 0) && (i % 8 == 0))  { Printf("\n"); }
    Printf("%d\t", arr[i]);
  }
  Printf("\n");

  Printf("Test 3: multiple malloc\n");
  arr = malloc(128);
  arr2 = malloc(128);
  arr3 = malloc(350);
  arr4 = malloc(1024);
  mfree(arr);
  mfree(arr2);
  mfree(arr3);
  mfree(arr4);

  Printf("Test 2: maloc of non-multiple of 4 (25)\n");
  arr = malloc(25);
  for (i = 0; i < (25 / 4); i++) { arr[i] = i; }
  for (i = 0; i < (25 / 4); i++)
  {
    if((i > 0) && (i % 8 == 0))  { Printf("\n"); }
    Printf("%d\t", arr[i]);
  }
  Printf("\n");
  
  Printf("Test 4: attempt to malloc larger than max\n");
  arr = malloc(4100);
  Printf("malloc returned %d\n", (int)arr);
  mfree(arr);

  Printf("Tests complete\n");
}