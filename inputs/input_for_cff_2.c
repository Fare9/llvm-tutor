//=============================================================================
// FILE:
//      input_for_cff_2.c
//
// DESCRIPTION:
//      Sample input file for ControlFlowFlattening.
//
//=============================================================================

#include <stdio.h>

void obfuscate_me(int number){
    printf("Checking how big the number is\n");
    int counter = 1;
    if(number < 5) {
        printf("number < 5\n");
        counter++;
    }
    else {
        printf("number >= 5\n");
        counter+= 2;
    }
    printf("Some divisors\n");
    if(number % 3) {
        printf("number %% 3\n");
        counter++;
    }
    if(number % 5) {
        printf("number %% 5\n");
        counter++;
    }
}

int main(int argc, char **argv)
{
    obfuscate_me(argc);
}
