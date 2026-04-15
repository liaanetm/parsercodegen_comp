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
#include <string.h>

#define norw 17                   // num of reserved words
#define idenmax 11                // identifier length
#define strmax 256                // str max length
#define MAX_SYMBOL_TABLE_SIZE 500 // table size

// Structure for enumeration
typedef enum
{
  skipsym = 1,       // Skip / ignore token
  identsym = 2,      // Identifier
  numbersym = 3,     // Number
  beginsym = 4,      // begin
  endsym = 5,        // end
  ifsym = 6,         // if
  fisym = 7,         // fi
  thensym = 8,       // then
  whilesym = 9,      // while
  dosym = 10,        // do
  odsym = 11,        // od
  callsym = 12,      // call
  constsym = 13,     // const
  varsym = 14,       // var
  procsym = 15,      // procedure
  writesym = 16,     // write
  readsym = 17,      // read
  elsesym = 18,      // else
  plussym = 19,      // +
  minussym = 20,     // -
  multsym = 21,      // *
  slashsym = 22,     // /
  eqsym = 23,        // =
  neqsym = 24,       // <>
  lessym = 25,       // <
  leqsym = 26,       // <=
  gtrsym = 27,       // >
  geqsym = 28,       // >=
  lparentsym = 29,   // (
  rparentsym = 30,   // )
  commasym = 31,     // ,
  semicolonsym = 32, // ;
  periodsym = 33,    // .
  becomessym = 34,   // :=
} TokenType;

typedef struct
{
  int kind;      // const = 1, var = 2, proc = 3
  char name[12]; // name up to 11 chars
  int val;       // number (ASCII value)
  int level;     // L level
  int addr;      // M address
  int mark;      // to indicate unavailable or deleted
} symbol;

// Global symbol table
symbol symbolTable[MAX_SYMBOL_TABLE_SIZE];

// Token list is global and can be accessed by the main function
int tokenList[strmax + 1] = {0};    // To store all the tokens
int tokenCount = 0;                 // Counter to keep track of the token list, also stores the total number of tokens
int tokenCounter = 0;               // helpful for fetching tokens from the tokenlist
char *nameTable[strmax + 1] = {""}; // Array to store the name table
int nameTableLength = 0;            // Keeps track of the name table length
int symbolTableCounter = 0;         // Keeps track the items added to the symbol table

int instructions[MAX_SYMBOL_TABLE_SIZE][3]; // Stores the instructions in a 2D table
int cx = 0;                                 // Keeps track of the instructions being added to the 2D array
char nameOP_storage[strmax + 1][4] = {""};  // Stores the names of the OP code instructions
int nameOPcounter = 0;                      // Keeps track of the OP code names
int errorFlag = 0;                          // Global flag that turns on whenever an error occurs
char errorMessage[100];                     // Stores error messages
int level = 0;

// Function prototypes
void program();
void block();
void constDeclaration();
int varDeclaration();
void statement();
void expression();
void condition();
void term();
void factor();
int symbolTableCheck(char identName[12]);
void insertSymbolTable(int kind, char name[12], int val, int level, int address, int mark);
void printInst(FILE *op);
void scanner(FILE *ip);
TokenType mapReservedWordAndIdentifier(char *str);
TokenType mapSpecialSym(char *buff);
TokenType reservedOrIdentifier(char buffer[], int bufferLength, char *reservedWord[], char *nameTable[], int *nameTableLength, int *idenIndex);
void emit(int num, int l, int m);
void getNextToken();
void procedure_declaration();

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Usage: ./parsercodegen_comp <input_file>\n");
    return 1;
  }

  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL)
  {
    printf("Error: could not open file %s\n", argv[1]);
    return 1;
  }

  scanner(fp);

  program();

  FILE *op = fopen("elf.txt", "w");

  if (errorFlag == 1)
  {
    fprintf(op, "%s\n", errorMessage);
  }
  else
  {
    printInst(op);
  }

  fclose(op);
  return 0;
}

// Checks whether the buffer contains a reserved word or identifier
TokenType reservedOrIdentifier(char buffer[], int bufferLength, char *reservedWord[], char *nameTable[], int *nameTableLength, int *idenIndex)
{

  // Boolean Flag
  int found = -1;

  // Iterates over the reserved words and checks similarities with buffer
  for (int j = 0; j < norw; j++)
  {
    if (strcmp(reservedWord[j], buffer) == 0)
    {
      *idenIndex = -1;                             // If they match the index identifier remains -1 (no identifier found)
      return mapReservedWordAndIdentifier(buffer); // The token number is returned
    }
  }

  // If it is not a reserved word, this loop checks if the buffer lines up with any identifiers in the name table
  for (int i = 0; i < *nameTableLength; i++)
  {
    // If it is in the name table, break
    if (strcmp(nameTable[i], buffer) == 0)
    {
      found = i; // Found is updated to index i
      break;
    }
  }

  if (found == -1)
  {
    // If it is not in the name table then add it
    nameTable[*nameTableLength] = malloc(bufferLength + 1);

    strcpy(nameTable[*nameTableLength], buffer);
    found = *nameTableLength; // Found is updated
    (*nameTableLength)++;
  }

  // Stores the identifier index from the name table
  *idenIndex = found;

  // Returns the identifier token number
  return identsym;
}

// This function maps the reserved word
TokenType mapReservedWordAndIdentifier(char *str)
{

  if (strcmp(str, "begin") == 0)
  {
    return beginsym;
  }

  if (strcmp(str, "end") == 0)
  {
    return endsym;
  }

  if (strcmp(str, "if") == 0)
  {
    return ifsym;
  }

  if (strcmp(str, "fi") == 0)
  {
    return fisym;
  }

  if (strcmp(str, "then") == 0)
  {
    return thensym;
  }

  if (strcmp(str, "while") == 0)
  {
    return whilesym;
  }

  if (strcmp(str, "do") == 0)
  {
    return dosym;
  }

  if (strcmp(str, "od") == 0)
  {
    return odsym;
  }

  if (strcmp(str, "call") == 0)
  {
    return callsym;
  }

  if (strcmp(str, "const") == 0)
  {
    return constsym;
  }

  if (strcmp(str, "var") == 0)
  {
    return varsym;
  }

  if (strcmp(str, "procedure") == 0)
  {
    return procsym;
  }

  if (strcmp(str, "write") == 0)
  {
    return writesym;
  }

  if (strcmp(str, "read") == 0)
  {
    return readsym;
  }

  if (strcmp(str, "else") == 0)
  {
    return elsesym;
  }

  return 0; // It is not a reserved word
}

// This function maps the special symbols and alsp detects escape sequences
TokenType mapSpecialSym(char *buff)
{
  if (strcmp(buff, "+") == 0)
  {

    return plussym;
  }

  if (strcmp(buff, "-") == 0)
  {
    return minussym;
  }

  if (strcmp(buff, "/") == 0)
  {
    return slashsym;
  }

  if (strcmp(buff, "*") == 0)
  {
    return multsym;
  }

  if (strcmp(buff, "(") == 0)
  {
    return lparentsym;
  }

  if (strcmp(buff, ")") == 0)
  {
    return rparentsym;
  }

  if (strcmp(buff, "=") == 0)
  {
    return eqsym;
  }

  if (strcmp(buff, ",") == 0)
  {
    return commasym;
  }

  if (strcmp(buff, ";") == 0)
  {
    return semicolonsym;
  }

  if (strcmp(buff, ".") == 0)
  {
    return periodsym;
  }

  if (strcmp(buff, "<") == 0)
  {
    return lessym;
  }

  if (strcmp(buff, ">") == 0)
  {
    return gtrsym;
  }

  if (strcmp(buff, "<=") == 0)
  {
    return leqsym;
  }

  if (strcmp(buff, ">=") == 0)
  {
    return geqsym;
  }

  if (strcmp(buff, "<>") == 0)
  {
    return neqsym;
  }

  if (strcmp(buff, ":=") == 0)
  {
    return becomessym;
  }

  // Returns 0 when either a space or an escape sequence is found
  if (strcmp(buff, " ") == 0 || strcmp(buff, "\n") == 0 || strcmp(buff, "\t") == 0 || strcmp(buff, "\r") == 0)
  {
    return 0;
  }

  return skipsym; // Returns when there is an invalid symbol
}

// Function that checks for escape sequences (Scanner was turned into an internal module)
void scanner(FILE *ip)
{

  //  Array for reserved words
  char *reservedWord[] = {"null", "begin", "call", "const", "do", "else", "end", "if", "odd", "procedure", "read", "then", "var", "while", "write", "fi", "od"};

  // Buffer used for the lexeme grouping process
  char *bufferLexeme = malloc(strmax + 1);

  // Array to store the lexemes (for printing purposes)
  char *lexemes[strmax + 1] = {""};

  // Used to read each char
  char ch;

  int i = 0;         // Counter to keep track of the buffer
  int lexLength = 0; // Counter to keep track of the lexemes array

  // While true
  while (1)
  {

    // First character is gathered
    ch = fgetc(ip);

    // Checks for the EOF character
    if (ch == EOF)
    {
      break;
    }

    // Checks if char is a letter or a number
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
    {
      i = 0;
      /*
       If the rest of the characters being read are also letters or numbers it can potentially
       be a reserved word or an identifier
      */
      while ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))
      {

        // Chars are added to the buffer until a non letter and non number character is reached
        bufferLexeme[i] = ch;
        // Moves along the ip pointer and gathers the character
        ch = fgetc(ip);

        // Buffer tracker is updated
        i++;
      }

      // Null terminator is added at the end
      bufferLexeme[i] = '\0';

      // If the character is neither a letter or a char, it the pointer skips it
      ungetc(ch, ip);

      // Identifier index variable is declared (it will be sent as a pointer so that it can be updated without it needing to be returned)
      int identifierIndex = -1;

      // Function is called to detect a reserved word or an identifier
      int token = reservedOrIdentifier(bufferLexeme, i, reservedWord, nameTable, &nameTableLength, &identifierIndex);

      // If the identifier token is return
      if (token == identsym)
      {
        // If the length of the identifier exceeds 11 characters
        if (strlen(bufferLexeme) > 11)
        {
          // If the identifier is too long, it's removed from the name table
          free(nameTable[nameTableLength - 1]);
          nameTable[nameTableLength - 1] = NULL;
          nameTableLength--; // Name table index is decreased

          tokenList[tokenCount] = skipsym;
          tokenCount++;
        }
        else
        {
          // If it is a valid identifier length, it gets added to the token list
          tokenList[tokenCount] = identsym;
          tokenCount++;

          // If it is not an identifier it also gets added
          if (identifierIndex != -1)
          {
            tokenList[tokenCount] = identifierIndex;
            tokenCount++;
          }
        }
      }

      else
      {
        // Token (reserved word) is added to the token list
        tokenList[tokenCount] = token;
        tokenCount++;
      }

      // Memory is allocated for an element in the lexeme array that is the same length as the buffer length
      lexemes[lexLength] = malloc(strlen(bufferLexeme) + 1);

      // Buffer is copied to the lexeme array
      strcpy(lexemes[lexLength], bufferLexeme);g
      lexLength++; // Lexeme array tracker is updated

      // Buffer is cleared
      bufferLexeme[0] = '\0';
      i = 0;
    }

    // If the character is strictly numbers
    else if (ch >= '0' && ch <= '9')
    {
      i = 0;

      // Checks if the next character is also a number
      while (ch >= '0' && ch <= '9')
      {

        bufferLexeme[i] = ch;
        ch = fgetc(ip);
        i++;
      }

      // Null Terminator
      bufferLexeme[i] = '\0';
      ungetc(ch, ip);

      // If the number is longer than 5 digits
      if (strlen(bufferLexeme) > 5)
      {

        // Add skipsym to the token list
        tokenList[tokenCount] = skipsym;
        tokenCount++;

        // Add the lexeme to the lexeme array
        lexemes[lexLength] = malloc(strlen(bufferLexeme) + 1);
        strcpy(lexemes[lexLength], bufferLexeme);
        lexLength++;
      }

      else
      {
        // If the number is valid the token is added
        tokenList[tokenCount] = numbersym;
        tokenCount++;

        // The numerical representation of the number is also added to the token list
        tokenList[tokenCount] = atoi(bufferLexeme);
        tokenCount++;

        // Lexeme is added to the lexeme array
        lexemes[lexLength] = malloc(strlen(bufferLexeme) + 1);
        strcpy(lexemes[lexLength], bufferLexeme);
        lexLength++;
      }

      // Buffer is cleared
      bufferLexeme[0] = '\0';
      i = 0;
    }

    // If it is not a letter or a number, it could be a special symbol
    else
    {

      i = 0;
      bufferLexeme[i] = ch;

      // If the char is any of these three, it could be a <=, >=, or a :=
      if (ch == '<' || ch == '>' || ch == ':')
      {

        ch = fgetc(ip);

        if (ch == '=' || (bufferLexeme[i] == '<' && ch == '>'))
        {
          bufferLexeme[i + 1] = ch;
          bufferLexeme[i + 2] = '\0';
        }
        else
        {
          ungetc(ch, ip);
          bufferLexeme[i + 1] = '\0';
        }

        int token = mapSpecialSym(bufferLexeme);

        // If the token is not an escape sequence
        if (token != 0)
        {
          // Token is added
          tokenList[tokenCount] = token;
          tokenCount++;

          // Lexeme is added to the lexeme array
          lexemes[lexLength] = malloc(strlen(bufferLexeme) + 1);
          strcpy(lexemes[lexLength], bufferLexeme);
          lexLength++;

          // Clear the buffer
          bufferLexeme[0] = '\0';
          i = 0;
          continue;
        }
      }

      // If the character is / it could either be division or a comment
      if (ch == '/')
      {
        ch = fgetc(ip);

        // Checks if it is a comment
        if (ch == '*')
        {

          // Ignores everything inside the comment
          while ((ch = fgetc(ip)) != '/')
          {
            continue;
          }
          continue;
        }

        int token = mapSpecialSym(bufferLexeme);

        // If the token is not an escape sequence
        if (token != 0)
        {
          // Token is added to the token array
          tokenList[tokenCount] = token;
          tokenCount++;

          // Lexeme is added to the lexeme array
          lexemes[lexLength] = malloc(strlen(bufferLexeme) + 1);
          strcpy(lexemes[lexLength], bufferLexeme);
          lexLength++;

          // Clear the buffer
          bufferLexeme[0] = '\0';
          i = 0;
          continue;
        }
      }

      // If it is a valid special symbol
      else
      {
        // Null terminator
        bufferLexeme[i + 1] = '\0';

        int token = mapSpecialSym(bufferLexeme);

        // If token is not an escape sequence
        if (token != 0)
        {
          // Token is added to the token list
          tokenList[tokenCount] = token;
          tokenCount++;

          // Copies lexeme to the lexeme array
          lexemes[lexLength] = malloc(strlen(bufferLexeme) + 1);
          strcpy(lexemes[lexLength], bufferLexeme);
          lexLength++;

          // Clear the buffer
          bufferLexeme[0] = '\0';
          i = 0;
          continue;
        }
      }
    }
  }

  fclose(ip);
}

// Look up function: finds the identifier in the symbol table based on name.
// Searches backwards and skips marked symbols.
int symbolTableCheck(char identName[12])
{
  for (int i = symbolTableCounter - 1; i >= 0; i--)
  {
    if (symbolTable[i].mark == 1)
      continue;
    if (strcmp(identName, symbolTable[i].name) == 0)
      return i;
  }
  return -1;
}

// insert to symbol table
void insertSymbolTable(int kind, char name[12], int val, int level, int address, int mark)
{

  symbolTable[symbolTableCounter].kind = kind;
  strcpy(symbolTable[symbolTableCounter].name, name);
  symbolTable[symbolTableCounter].val = val;
  symbolTable[symbolTableCounter].level = level;
  symbolTable[symbolTableCounter].addr = address;
  symbolTable[symbolTableCounter].mark = mark;

  symbolTableCounter++;
}

// Emit function stores instructions in the 2D array
void emit(int num, int l, int m)
{

  instructions[cx][0] = num;
  instructions[cx][1] = l;
  instructions[cx][2] = m;

  cx++;
}

// Identifiers and numbers take two entries in tokenList (symbol + value),
//  so we advance tokenCounter by 2 instead of 1.
void getNextToken()
{

  if (tokenList[tokenCounter] == identsym || tokenList[tokenCounter] == numbersym)
  {
    tokenCounter += 2;
  }
  else
  {
    tokenCounter++;
  }
}

// Expression
void expression()
{

  //"-" at the start means an expression can begin with a negative sign
  if (tokenList[tokenCounter] == minussym)
  {
    getNextToken();
    term(); // Term is called

    // If error is found, function terminates immediately
    if (errorFlag == 1)
      return;
    emit(2, 0, 1);
    strcpy(nameOP_storage[nameOPcounter], "OPR");
    nameOPcounter++;
  }
  else
  {
    term(); // Term is called
    if (errorFlag == 1)
      return;
  }

  // While loop checks for plus or minus sign
  while (tokenList[tokenCounter] == plussym || tokenList[tokenCounter] == minussym)
  {
    // If token is a plus sign, term is called and error check is done
    if (tokenList[tokenCounter] == plussym)
    {
      getNextToken();
      term();

      // If error is found, function terminates
      if (errorFlag == 1)
        return;

      // If no errors are found, ADD instruction is emitted
      emit(2, 0, 2);
      strcpy(nameOP_storage[nameOPcounter], "OPR");
      nameOPcounter++;
    }

    // Negative sign was found, similar process is repeated for the negative sign
    else
    {
      getNextToken();
      term();
      if (errorFlag == 1)
        return;
      emit(2, 0, 3);
      strcpy(nameOP_storage[nameOPcounter], "OPR");
      nameOPcounter++;
    }
  }
}

// Condition
void condition()
{

  expression(); // Expression is called

  // Series of if statemetns check for each relational operator, then corresponding instruction is added to the 2D array (emit function)
  if (tokenList[tokenCounter] == eqsym)
  {

    getNextToken();
    expression();
    emit(2, 0, 6);
    strcpy(nameOP_storage[nameOPcounter], "OPR");
    nameOPcounter++;
  }
  else if (tokenList[tokenCounter] == neqsym)
  {

    getNextToken();
    expression();
    emit(2, 0, 7);
    strcpy(nameOP_storage[nameOPcounter], "OPR");
    nameOPcounter++;
  }
  else if (tokenList[tokenCounter] == lessym)
  {

    getNextToken();
    expression();
    emit(2, 0, 8);
    strcpy(nameOP_storage[nameOPcounter], "OPR");
    nameOPcounter++;
  }
  else if (tokenList[tokenCounter] == leqsym)
  {

    getNextToken();
    expression();
    emit(2, 0, 9);
    strcpy(nameOP_storage[nameOPcounter], "OPR");
    nameOPcounter++;
  }
  else if (tokenList[tokenCounter] == gtrsym)
  {

    getNextToken();
    expression();
    emit(2, 0, 10);
    strcpy(nameOP_storage[nameOPcounter], "OPR");
    nameOPcounter++;
  }
  else if (tokenList[tokenCounter] == geqsym)
  {

    getNextToken();
    expression();
    emit(2, 0, 11);
    strcpy(nameOP_storage[nameOPcounter], "OPR");
    nameOPcounter++;
  }
  // Otherwise an error was encountered
  else
  {
    printf("Error: condition must contain comparison operator");
    strcpy(errorMessage, "Error: condition must contain comparison operator");
    errorFlag = 1;
    return;
  }
}

// Factor
void factor()
{

  int symIdx;

  // Checks if token is an identifier
  if (tokenList[tokenCounter] == identsym)
  {

    // If so, identifier is added to the name tabke
    symIdx = symbolTableCheck(nameTable[tokenList[tokenCounter + 1]]);

    // If -1 is returned, no corresponding identifier was found
    if (symIdx == -1)
    {

      printf("Error: undeclared identifier");
      strcpy(errorMessage, "Error: undeclared identifier");
      errorFlag = 1;
      return;
    }
    // If 1 is returned, symbol is an identifier and LIT instruction is emitted
    if (symbolTable[symIdx].kind == 1)
    {
      emit(1, 0, symbolTable[symIdx].val);
      strcpy(nameOP_storage[nameOPcounter], "LIT");
      nameOPcounter++;
    }
    else
    {

      // Identifier is a variable and LOD instruction is emitted
      emit(3, level - symbolTable[symIdx].level, symbolTable[symIdx].addr);
      strcpy(nameOP_storage[nameOPcounter], "LOD");
      nameOPcounter++;
    }

    getNextToken();
  }

  // If token is a number, LIT instruction is emitted
  else if (tokenList[tokenCounter] == numbersym)
  {
    emit(1, 0, tokenList[tokenCounter + 1]);
    strcpy(nameOP_storage[nameOPcounter], "LIT");
    nameOPcounter++;
    getNextToken();
  }

  // Checks for left parenthesis
  else if (tokenList[tokenCounter] == lparentsym)
  {

    getNextToken();
    expression(); // Expression is called

    // Calls for right parenthesis
    if (tokenList[tokenCounter] != rparentsym)
    {

      printf("Error: right parenthesis must follow left parenthesis");
      strcpy(errorMessage, "Error: right parenthesis must follow left parenthesis");
      errorFlag = 1;
      return;
    }

    getNextToken();
  }
  // An error was found
  else
  {
    printf("Error: arithmetic equations must contain operands, parentheses, numbers, or symbols");
    strcpy(errorMessage, "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols");
    errorFlag = 1;
    return;
  }
}

// Term
void term()
{

  factor(); // Factor is called
  if (errorFlag == 1)
    return;

  // Loop checks for multiplication or division symbol
  while (tokenList[tokenCounter] == multsym || tokenList[tokenCounter] == slashsym)
  {

    // If token is a multiplication symbol, then factor is called and MULT instruction is emitted
    if (tokenList[tokenCounter] == multsym)
    {

      getNextToken();
      factor();
      if (errorFlag == 1)
        return;
      emit(2, 0, 4);
      strcpy(nameOP_storage[nameOPcounter], "OPR");
      nameOPcounter++;
    }
    // Division symbol is emitted
    else
    {

      getNextToken();

      // Custom check for division by 0
      if (tokenList[tokenCounter] == numbersym && tokenList[tokenCounter + 1] == 0)
      {
        printf("Error: division by 0");
        strcpy(errorMessage, "Error: division by 0");
        errorFlag = 1;
        return;
      }

      factor();
      if (errorFlag == 1)
        return;
      emit(2, 0, 5);
      strcpy(nameOP_storage[nameOPcounter], "OPR");
      nameOPcounter++;
    }
  }
}

// Statement
void statement()
{
  // Checks if token is an identifier
  if (tokenList[tokenCounter] == identsym)
  {
    // Looks up the identifier in the symbol table
    int symIndex = symbolTableCheck(nameTable[tokenList[tokenCounter + 1]]);
    if (symIndex == -1)
    {
      printf("Error: undeclared identifier");
      strcpy(errorMessage, "Error: undeclared identifier");
      errorFlag = 1;
      return;
    }

    // If token is in the symbol table but is not classified as an identifier, an error message is triggered
    if (symbolTable[symIndex].kind != 2)
    {
      printf("Error: only variable values may be altered");
      strcpy(errorMessage, "Error: only variable values may be altered");
      errorFlag = 1;
      return;
    }

    getNextToken();

    // Checks for :=
    if (tokenList[tokenCounter] != becomessym)
    {
      printf("Error: assignment statements must use :=");
      strcpy(errorMessage, "Error: assignment statements must use :=");
      errorFlag = 1;

      return;
    }

    getNextToken();

    expression(); // Expression is called

    if (errorFlag == 1)
      return;
    emit(4, level - symbolTable[symIndex].level, symbolTable[symIndex].addr);
    strcpy(nameOP_storage[nameOPcounter], "STO");
    nameOPcounter++;
    return;
  }

  // Checks if token is the begin keyword
  if (tokenList[tokenCounter] == beginsym)
  {
    getNextToken();

    // Custom error checks for empty begin/end block
    if (tokenList[tokenCounter] == endsym)
    {

      printf("Error: empty begin/end block");
      strcpy(errorMessage, "Error: empty begin/end block");
      errorFlag = 1;
      return;
    }

    statement(); // Statement is called as per the grammar rules
    if (errorFlag == 1)
      return;

    // While loop checks for semicolon
    while (tokenList[tokenCounter] == semicolonsym)
    {
      getNextToken();
      statement(); // Statement is called inside while loop
    }

    // If there is a begin present, it must be followed by an end keyword. Otherwise there is an error
    if (tokenList[tokenCounter] != endsym)
    {
      printf("Error: begin must be followed by end");
      strcpy(errorMessage, "Error: begin must be followed by end");
      errorFlag = 1;
      return;
    }
    getNextToken();
    return;
  }

  if (tokenList[tokenCounter] == ifsym)
  {
    getNextToken();

    condition();

    int jpcIndex = cx;
    emit(8, 0, 0); // JPC placeholder
    strcpy(nameOP_storage[nameOPcounter], "JPC");
    nameOPcounter++;

    // must have then
    if (tokenList[tokenCounter] != thensym)
    {
      printf("Error: if must be followed by then");
      strcpy(errorMessage, "Error: if must be followed by then");
      errorFlag = 1;
      return;
    }

    getNextToken();
    statement(); // then-block

    // check for else
    if (tokenList[tokenCounter] == elsesym)
    {
      int jmpIndex = cx;
      emit(7, 0, 0); // JMP placeholder (skip else)
      strcpy(nameOP_storage[nameOPcounter], "JMP");
      nameOPcounter++;

      // JPC should jump to start of else
      instructions[jpcIndex][2] = cx * 3;

      getNextToken();
      statement(); // else-block

      // JMP should jump to after else
      instructions[jmpIndex][2] = cx * 3;
    }
    else
    {
      // no else, JPC jumps to after then
      instructions[jpcIndex][2] = cx * 3;
    }

    // must end with fi
    if (tokenList[tokenCounter] != fisym)
    {
      printf("Error: if-then statement must end with fi");
      strcpy(errorMessage, "Error: if-then statement must end with fi");
      errorFlag = 1;
      return;
    }

    getNextToken();
    return;
  }

  // Checks for while reserved word
  if (tokenList[tokenCounter] == whilesym)
  {
    getNextToken();
    int loopIndex = cx * 3; // Stores the loop index
    condition();            // Condition is called
    if (errorFlag == 1)
      return;

    // Checks for do keyword
    if (tokenList[tokenCounter] != dosym)
    {
      printf("Error: while must be followed by do");
      strcpy(errorMessage, "Error: while must be followed by do");
      errorFlag = 1;
      return;
    }

    getNextToken();

    int jpcIndex = cx; // JPC index is stored to update the M field of the target instruction
    // JPC instruction is emitted
    emit(8, 0, 0);
    strcpy(nameOP_storage[nameOPcounter], "JPC");
    nameOPcounter++;

    statement(); // Statement is called
    if (errorFlag == 1)
      return;
    // JMP instructionis emitted
    emit(7, 0, loopIndex);
    strcpy(nameOP_storage[nameOPcounter], "JMP");
    nameOPcounter++;

    instructions[jpcIndex][2] = cx; // Target instruction's M field is updated

    // Checks for od reserved word
    if (tokenList[tokenCounter] != odsym)
    {
      printf("Error: do must be followed by od");
      strcpy(errorMessage, "Error: do must be followed by od");
      errorFlag = 1;
      return;
    }
    getNextToken();
    return;
  }

  // Checks for read reserved word
  if (tokenList[tokenCounter] == readsym)
  {
    getNextToken();
    // If following token is not an identifier, an error message is triggered
    if (tokenList[tokenCounter] != identsym)
    {
      printf("Error: const, var, and read keywords must be followed by identifier");
      strcpy(errorMessage, "Error: const, var, and read keywords must be followed by identifier");
      errorFlag = 1;
      return;
    }

    // Otherwise, a lookup of the symbol table is done
    int symIndex = symbolTableCheck(nameTable[tokenList[tokenCounter + 1]]);
    // Checks if identifier was not found
    if (symIndex == -1)
    {
      printf("Error: undeclared identifier\n");
      strcpy(errorMessage, "Error: undeclared identifier");
      errorFlag = 1;
      return;
    }

    // Checks if supposed identifier is not stored as kind 2
    if (symbolTable[symIndex].kind != 2)
    {
      printf("Error: only variable values may be altered\n");
      strcpy(errorMessage, "Error: only variable values may be altered");
      errorFlag = 1;
      return;
    }

    getNextToken();
    // READ instruction is emitted
    emit(9, 0, 2);
    strcpy(nameOP_storage[nameOPcounter], "SYS");
    nameOPcounter++;
    // STORE instruction is emitted
    emit(4, level - symbolTable[symIndex].level, symbolTable[symIndex].addr);
    strcpy(nameOP_storage[nameOPcounter], "STO");
    nameOPcounter++;
    return;
  }

  // Checks for write keyword
  if (tokenList[tokenCounter] == writesym)
  {
    getNextToken();
    expression(); // Expression is called
    // WRITE instruction is emitted
    emit(9, 0, 1);
    strcpy(nameOP_storage[nameOPcounter], "SYS");
    nameOPcounter++;
    return;
  }

  // Checks for call keyword
  if (tokenList[tokenCounter] == callsym)
  {
    getNextToken();
    if (tokenList[tokenCounter] != identsym)
    {
      printf("Error: procedure and call keywords must be followed by identifier");
      strcpy(errorMessage, "Error: procedure and call keywords must be followed by identifier");
      errorFlag = 1;
      return;
    }
    int symIdx = symbolTableCheck(nameTable[tokenList[tokenCounter + 1]]);
    if (symIdx == -1)
    {
      printf("Error: undeclared identifier");
      strcpy(errorMessage, "Error: undeclared identifier");
      errorFlag = 1;
      return;
    }
    if (symbolTable[symIdx].kind != 3)
    {
      printf("Error: call must be followed by a procedure identifier");
      strcpy(errorMessage, "Error: call must be followed by a procedure identifier");
      errorFlag = 1;
      return;
    }
    emit(5, level - symbolTable[symIdx].level, symbolTable[symIdx].addr);
    strcpy(nameOP_storage[nameOPcounter], "CAL");
    nameOPcounter++;
    getNextToken();
    return;
  }
}

// Var-declaration: returns the number of variables
int varDeclaration()
{
  int numVars = 0;

  char identName[12];

  // Checks for var reserved keyword
  if (tokenList[tokenCounter] == varsym)
  {
    do
    {

      getNextToken();

      if (tokenList[tokenCounter] != identsym)
      {
        printf("Error: const, var, and read keywords must be followed by identifier");
        strcpy(errorMessage, "Error: const, var, and read keywords must be followed by identifier");
        errorFlag = 1;
        return -1;
      }

      // Checks for duplicates in the symbol table
      if (symbolTableCheck(nameTable[tokenList[tokenCounter + 1]]) != -1)
      {
        printf("Error: symbol name has already been declared");
        strcpy(errorMessage, "Error: symbol name has already been declared");
        errorFlag = 1;
        return -1;
      }

      strcpy(identName, nameTable[tokenList[tokenCounter + 1]]);

      // Identifier is inserted into the symbol table
      insertSymbolTable(2, identName, 0, level, numVars + 3, 0);

      numVars++;
      getNextToken();

    } while (tokenList[tokenCounter] == commasym);

    // Checks for semicolon
    if (tokenList[tokenCounter] != semicolonsym)
    {
      printf("Error: constant and variable declarations must be followed by a semicolon");
      strcpy(errorMessage, "Error: constant and variable declarations must be followed by a semicolon");
      errorFlag = 1;
      return -1;
    }

    getNextToken();
  }

  // Returns the number of variables
  return numVars;
}

// Const-declaration
void constDeclaration()
{

  char identName[12];
  // Checks for const reserved word
  if (tokenList[tokenCounter] == constsym)
  {
    do
    {
      getNextToken();
      // Checks if token is not an identifier
      if (tokenList[tokenCounter] != identsym)
      {
        printf("Error: const, var, and read keywords must be followed by identifier");
        strcpy(errorMessage, "Error: const, var, and read keywords must be followed by identifier");
        errorFlag = 1;
        return;
      }

      // Checks for duplicates in name table
      if (symbolTableCheck(nameTable[tokenList[tokenCounter + 1]]) != -1)
      {
        printf("Error: symbol name has already been declared");
        strcpy(errorMessage, "Error: symbol name has already been declared");
        errorFlag = 1;
        return;
      }

      strcpy(identName, nameTable[tokenList[tokenCounter + 1]]);

      getNextToken();

      // Checks for equal sign
      if (tokenList[tokenCounter] != eqsym)
      {
        printf("Error: constants must be assigned with =");
        strcpy(errorMessage, "Error: constants must be assigned with =");
        errorFlag = 1;
        return;
      }

      getNextToken();

      // Checks if current token is a number
      if (tokenList[tokenCounter] != numbersym)
      {
        printf("Error: constants must be assigned an integer value");
        strcpy(errorMessage, "Error: constants must be assigned an integer value");
        errorFlag = 1;
        return;
      }

      // If all previous checks are passed, constant gets inserted into the symbol table
      insertSymbolTable(1, identName, tokenList[tokenCounter + 1], level, 0, 0);
      getNextToken();

    } while (tokenList[tokenCounter] == commasym); // Continues checking in case there are multiple constants declared
    // Semicolon check
    if (tokenList[tokenCounter] != semicolonsym)
    {
      printf("Error: constant and variable declarations must be followed by a semicolon");
      strcpy(errorMessage, "Error: constant and variable declarations must be followed by a semicolon");
      errorFlag = 1;
      return;
    }
    getNextToken();
  }
}

// Block
void block()
{
  // Remember where this block's symbols start so we only mark them on exit
  int blockStart = symbolTableCounter;

  // Checks for constants and adds them to symbol table if successful
  constDeclaration();
  if (errorFlag == 1)
    return;

  // Checks for variables, adds them to symbol table, and returns the number of variables
  int numVars = varDeclaration();

  // Error check
  if (numVars == -1)
  {
    errorFlag = 1;
    return;
  }

  // Emit JMP to skip over any procedure bodies that follow
  int jmpIdx = cx;
  emit(7, 0, 0); // JMP placeholder – backpatched below
  strcpy(nameOP_storage[nameOPcounter], "JMP");
  nameOPcounter++;

  procedure_declaration();
  if (errorFlag == 1)
    return;

  // Backpatch JMP to land on the INC that follows
  instructions[jmpIdx][2] = cx * 3;

  // INC instruction is emitted
  emit(6, 0, numVars + 3);
  strcpy(nameOP_storage[nameOPcounter], "INC");
  nameOPcounter++;

  statement(); // Statement is called
  if (errorFlag == 1)
    return;

  // Mark only this block's symbols so outer scopes remain visible
  if (level > 0)
  {
    for (int i = blockStart; i < symbolTableCounter; i++)
      symbolTable[i].mark = 1;
  }
}

// Program
void program()
{

  // Detects skipsym error tokens
  for (int i = 0; i < tokenCount; i++)
  {
    // If identifier or number symbol is encountered i advances and skips the index or number that follows the tokens (2 1 would be skipped because 1 is an index in this context)
    if (tokenList[i] == identsym || tokenList[i] == numbersym)
    {
      i++;
      continue;
    }

    // Checks for skipsym before any other functions are called
    if (tokenList[i] == skipsym)
    {
      errorFlag = 1;
      printf("Error: Scanning error detected by lexer (skipsym present)");
      strcpy(errorMessage, "Error: Scanning error detected by lexer (skipsym present)");
      return;
    }
  }

  // Block is called
  block();

  // Custom error check verifies whether an input file is empty
  if (tokenList[0] == 0)
  {
    printf("Error: input file is empty");
    strcpy(errorMessage, "Error: input file is empty");
    errorFlag = 1;
    return;
  }

  // Error check
  if (errorFlag == 1)
  {
    return;
  }

  // Checks for the period at the end of a program
  if (tokenList[tokenCounter] != periodsym)
  {
    printf("Error: program must end with period");
    strcpy(errorMessage, "Error: program must end with period");
    errorFlag = 1;
    return;
  }

  // HALT instruction is emitted
  emit(9, 0, 3);
  strcpy(nameOP_storage[nameOPcounter], "SYS");
  nameOPcounter++;
}

void printInst(FILE *op)
{
  // Headers
  printf("Assembly code:\n");
  printf("+------+-------+---+-----+\n");
  printf("| Line |   OP  | L |  M  |\n");
  printf("+------+-------+---+-----+\n");
  for (int i = 0; i < cx; i++)
  {
    // Writes instructions into the output file in the number format (OPcode is a number)
    fprintf(op, " %d %d %d \n", instructions[i][0], instructions[i][1], instructions[i][2]);
    printf("|  %d  | %s |  %d  |  %d  |\n", i, nameOP_storage[i], instructions[i][1], instructions[i][2]); // Instructions are printed in the terminal with the OPcode instruction name
  }
  printf("+------+-------+---+-----+\n");

  // Symbol table header
  printf("Symbol Table:\n");
  printf("+------+-------------+-------+-------+---------+------+\n");
  printf("| Kind |     Name    | Value | Level | Address | Mark |\n");
  printf("+------+-------------+-------+-------+---------+------+\n");

  // Prints symbol table in the terminal
  for (int i = 0; i < symbolTableCounter; i++)
  {
    printf("| %d    |     %s       |   %d   |    %d  |    %d    |   %d  |\n", symbolTable[i].kind, symbolTable[i].name, symbolTable[i].val, symbolTable[i].level, symbolTable[i].addr, symbolTable[i].mark);
  }
  printf("+------+-------------+-------+-------+---------+------+\n");
}

// {} -> loop
// [] -> if condition
// PROCEDURE-DECLARATION function
void procedure_declaration()
{

  char identName[12];

  while (tokenList[tokenCounter] == procsym)
  {
    getNextToken();
    if (tokenList[tokenCounter] != identsym)
    {
      printf("Error: procedure and call keywords must be followed by identifier");
      strcpy(errorMessage, "Error: procedure and call keywords must be followed by identifier");
      errorFlag = 1;
      return;
    }
    strcpy(identName, nameTable[tokenList[tokenCounter + 1]]);
    getNextToken();
    if (tokenList[tokenCounter] != semicolonsym)
    {
      printf("Error: procedure declarations must be followed by a semicolon");
      strcpy(errorMessage, "Error: procedure declarations must be followed by a semicolon");
      errorFlag = 1;
      return;
    }
    getNextToken();
    insertSymbolTable(3, identName, 0, level, cx * 3, 0);
    level++;
    block();
    level--;
    if (errorFlag == 1)
      return;
    emit(2, 0, 0);
    strcpy(nameOP_storage[nameOPcounter], "OPR");
    nameOPcounter++;
    if (tokenList[tokenCounter] != semicolonsym)
    {
      printf("Error: procedure declarations must be followed by a semicolon");
      strcpy(errorMessage, "Error: procedure declarations must be followed by a semicolon");
      errorFlag = 1;
      return;
    }
    getNextToken();
  }
}


//TODO ident funtion
// TODO must emit CAL and RTN for procedure calls and procedure returns respectively.
//TODO Error: call must be followed by a procedure identifier
//TODDO W4 introduces new failure modes
/*
around procedure declarations (nested declarations, scope violations, calls to undeclared
procedures) that may require additional custom error messages.
*/
//To submit 
/*A required set of 3 error-case pairs demonstrating that your program correctly
14
catches the new HW4 errors introduced by the procedure-declaration and call grammar
extensions. Do not submit error-case pairs for the errors that were already required in
HW3 
*/
/*The 3 required error-case pairs must cover, at a minimum, the following
HW4-specific errors from Section 7.4:
1. procedure and call keywords must be followed by identifier —
triggered by a procedure or call keyword that is not immediately followed
by an identifier token.
2. procedure declarations must be followed by a semicolon —
triggered by omitting the trailing ; after a procedure’s body.
3. call must be followed by a procedure identifier — triggered by
calling an identifier whose symbol-table entry is not a procedure (e.g., a
variable or constant)
*/
//In total, this submission component contains 6 files
