//=============================================================================
// FILE:
//      input_for_cff_1.c
//
// DESCRIPTION:
//      Sample input file for ControlFlowFlattening.
//
//=============================================================================

#include <stdio.h>

void simple_branch(int value){
  printf("Before branch\n");

  if(value > 5) {
    printf("value > 5\n");
  }
  else{
    printf("value <= 5\n");
  }

  printf("After branch\n");
}

int main(int argc, char ** argv)
{
  simple_branch(argc);
}