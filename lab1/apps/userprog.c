#include "usertraps.h"

void main (int x)
{
  Printf("Hello World!\n");
  Printf("current pid is: %d\n\n", Getpid());
  while(1); // Use CTRL-C to exit the simulator
}