#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sleeping.h"


static void EachBody(cpBody *body, cpBool *all_sleeping){
  // If we find an active body, set all_sleeping to 0
  if(!cpBodyIsSleeping(body)) *all_sleeping = 0;
}

int test_all_sleeping (cpSpace *space) {
  int all_sleeping = 1;
  cpSpaceEachBody(space, (cpSpaceBodyIteratorFunc)EachBody, &all_sleeping);
  // all_sleeping = 1 means that all the bodies are sleeping
  if (all_sleeping) { return 1; }
  else { return 0; }
}
