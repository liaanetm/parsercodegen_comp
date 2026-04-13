/*
Assignment:
HW4 - Parser/Code Generator Complete Version for PL/0
      (procedures + lexicographical level management)

Author(s): <Full Name 1>, <Full Name 2>

Language: C (only)

To Compile:
  gcc -O2 -Wall -std=c11 -o parsercodegen_comp parsercodegen_comp.c

To Execute (on Eustis):
  ./parsercodegen_comp <input_file.txt>

where:
  <input_file.txt> is the path to the PL/0 source program

Notes:
  - Single integrated program: scanner + parser + code gen
  - parsercodegen_comp.c accepts ONE command-line argument
  - Scanner runs internally (no intermediate token file)
  - Implements recursive-descent parser for the full PL/0 grammar
    including procedure declarations and the call statement
  - Tracks lexicographical levels (nesting depth) for every symbol
  - Generates PM/0 assembly code with proper L fields for LOD/STO/CAL
    and emits CAL/RTN instructions for procedures
    (see Appendix A for the ISA)
  - All development and testing performed on Eustis

Class: COP 3402 - Systems Software - Spring 2026

Instructor: Dr. Jie Lin

Due Date: See Webcourses for the posted due date and time.
*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    printf("argc = %d\n", argc);

    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    printf("\n");

    if (argc > 1)
    {
        int x = atoi(argv[1]);   // convert string to int (simple)
        printf("Converted argv[1] to int: %d\n", x);
    }
    else
    {
        printf("No extra arguments provided.\n");
        printf("Try: ./00_args 123\n");
    }

    return 0;
}
