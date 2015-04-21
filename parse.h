#ifndef PARSE_H
#define PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"
#include "structs.h"
#include "pl0_constants.h"
#include "pm0_constants.h"

// Constants defined
#define MAX_STACK_HEIGHT 2000
#define MAX_CODE_LENGTH 500
#define MAX_IDENTIFIERS_CHAR_LENGTH 12

// GLOBALS 
int token;
FILE *ifp;
FILE *ofp;
int codeIndex = 0;
int symbolIndex = 0;
int lexLevel = 0;

// defined variable of type instruction struct that holds are input code
Instruction parseCode[MAX_CODE_LENGTH];
Symbol symbolTable[MAX_STACK_HEIGHT];

// Recursive Parsing
void parse();
void program();
void block();
void statement();
void condition();
void expression();
void term();
void factor();

// Tokens & Lexemes
void getToken();
int getNumber();
char* getIdenName();

// Error catching
void error(int err_id);

// Code Generation
void enter(char* name, int typeID, int level, int address, int constValue);
void gen(int OP, int L, int M);
Symbol find(char* symbolName);
void ifBlock();
void whileBlock();

// Helpers
int isValidRelationalOperator();
void writeMCode();
int mapOPRCode();
int relativeSymbolLevel(int symbolLevel);

// Parse the lexem list file
void parse()
{
    ifp = fopen(LIST_FILE, "r");
    ofp = fopen(CODE_FILE, "w");
    
    program();

    writeMCode();
    
    fclose(ifp);
    fclose(ofp);
}

void writeMCode()
{
    int i;

    for (i = 0; i < codeIndex; i++)
        fprintf(ofp, "%d %d %d\n", parseCode[i].OP, parseCode[i].L, parseCode[i].M);
}

void program()
{
    getToken();
    block();
    if (token != PERIOD_SYM)
        error(9);
}

void block()
{
    int varCounter = 0, space = 3, tempJMP, tempSymbolIndex, prevSymbolIndex;
    char symbolName[MAX_IDENTIFIERS_CHAR_LENGTH];

    // Increase lex level
    lexLevel++;
    
    // Store current symbolIndex for later
    prevSymbolIndex = symbolIndex;
    
    // Gen JMP command and store the address of this gen for later modification
    tempJMP = codeIndex;
    gen(JMP, 0, 0);
    
    // Check for constants
    if (token == CONST_SYM)
    {
        do
        {
            // Check next token is ident
            getToken();
            if (token != IDENT_SYM)
                error(4);

            // Store the index of the symbol table and enter in constant
            tempSymbolIndex = symbolIndex;
            enter(getIdenName(), CONST_SYM, lexLevel, 0, 0);
            
            // Check next token is equal sign
            getToken();
            if (token != EQL_SYM)
                error(3);
            
            // Check next token is a number value
            getToken();
            if (token != NUM_SYM)
                error(2);

            // Update the constant in symbol table with its value
            symbolTable[tempSymbolIndex].constValue = getNumber();
            
            // Get next token and repeat loop if token is a comma
            getToken();
        } while (token == COMMA_SYM);
        
        // Check final token is a semicolon after const symbol
        if (token != SEMICOLON_SYM)
            error(5);
        
        getToken();
    }
    
    // Check for vars in the block
    if (token == VAR_SYM)
    {
        do 
        {
            // Check next token is ident
            getToken();

            if (token != IDENT_SYM)
                error(4);

            enter(getIdenName(), VAR_SYM, lexLevel, space + varCounter, 0);

            varCounter++;
            
            // Get next token and repeat loop if token is a comma
            getToken();
        } while (token == COMMA_SYM);
        
        // Check final token is a semicolon for var line
        if (token != SEMICOLON_SYM)
            error(5);
        
        getToken();
    }
    
    // Check for subprocedure
    while (token == PROC_SYM)
    {
        getToken();
        if (token != IDENT_SYM)
            error(6);
        
        // Add procedure to symbol table
        enter(getIdenName(), PROC_SYM, lexLevel, codeIndex, 0);
        
        getToken();
        if (token != SEMICOLON_SYM)
            error(5);
        
        getToken();
        
        // Recursively call block to read subprocedure's block
        block();
        
        if (token != SEMICOLON_SYM)
            error(5);
        
        getToken();
    }

    // Modify JMP with codeIndex after recursive call to potential blocks (subprocedures)
    parseCode[tempJMP].M = codeIndex;

    // Reserve stack space for block
    gen(INC, 0, space + varCounter);
    
    statement();
    
    // Restore previous symbolIndex
    symbolIndex = prevSymbolIndex;
    
    // Decrease lexLevel
    lexLevel--;
}

void statement()
{
    Symbol foundSymbol;

    if (token == IDENT_SYM)
    {
        foundSymbol = find(getIdenName());

        if (foundSymbol.typeID == CONST_SYM)
            error(12);

        getToken();

        if (token != BECOMES_SYM)
            error(13);
        
        getToken();
        expression();

        gen(STO, relativeSymbolLevel(foundSymbol.level), foundSymbol.address);
    }
    
    else if (token == CALL_SYM)
    {
        // Check for ident symbol
        getToken();
        if (token != IDENT_SYM)
            error(14);
        
        // Search symbol table for procedure
        foundSymbol = find(getIdenName());
        
        if (foundSymbol.typeID == PROC_SYM)
        {
            gen(CAL, relativeSymbolLevel(foundSymbol.level), foundSymbol.address);
        }
        else
        {
            // Call must be followed by a procedure identifier
            error(14);
        }
        
        getToken();
    }
    
    else if (token == BEGIN_SYM)
    {
        getToken();

        statement();

        while (token == SEMICOLON_SYM)
        {
            getToken();

            statement();
        }
        
        if (token != END_SYM)
            error(26);
        
        gen(OPR, 0, 0);
        
        getToken();
    }
    
    else if (token == IF_SYM)
    {
        ifBlock();
    }
    
    else if (token == WHILE_SYM)
    {
        whileBlock();
    }

    else if (token == WRITE_SYM)
    {
        getToken();

        if (token != IDENT_SYM)
            error(14);

        foundSymbol = find(getIdenName());

        if (foundSymbol.typeID == CONST_SYM)
        {
            gen(LIT,0,foundSymbol.constValue);
        }
        else if (foundSymbol.typeID == VAR_SYM)
        {
            gen(LOD, relativeSymbolLevel(foundSymbol.level), foundSymbol.address);
        }

        gen(OUT, 0, 0);

        getToken();
    }

    else if (token == READ_SYM)
    {
        getToken();
        if (token != IDENT_SYM)
            error(14);

        foundSymbol = find(getIdenName());

        gen(IN, 0, 0);

        gen(STO, relativeSymbolLevel(foundSymbol.level) , foundSymbol.address);

        getToken();
    } 
}

void condition()
{
    int relOperator;

    if (token == ODD_SYM)
    {
        getToken();
        expression();
        gen(OPR, 0, ODD);
    }

    else
    {
        expression();

        if (!isValidRelationalOperator())
            error(20);
        
        relOperator = token;

        getToken();
        expression();
        gen(OPR, 0, mapOPRCode(relOperator));
    }
}

void expression()
{
    int add_op;

    if (token == PLUS_SYM || token == MINUS_SYM)
    {
        add_op = token;

        getToken();

        term();

        if (add_op == MINUS_SYM)
            gen(OPR, 0, NEG);
    }

    else
        term();
    
    while (token == PLUS_SYM || token == MINUS_SYM)
    {
        add_op = token;

        getToken();

        term();

        if (add_op == PLUS_SYM)
            gen(OPR, 0, ADD);
        else
            gen(OPR, 0, SUB);
    }
}

void term()
{
    int mul_op;

    factor();

    while (token == MULT_SYM || token == SLASH_SYM)
    {
        mul_op = token;

        getToken();

        factor();

        if (mul_op == MULT_SYM)
            gen(OPR, 0, MUL);

        else
            gen(OPR, 0, DIV);
    }
}

void factor()
{
    Symbol foundSymbol;

    if (token == IDENT_SYM)
    {   
        foundSymbol = find(getIdenName());

        if (foundSymbol.typeID == CONST_SYM)
        {
            gen(LIT, 0, foundSymbol.constValue);
        }
        else if (foundSymbol.typeID == VAR_SYM)
        {
            gen(LOD, relativeSymbolLevel(foundSymbol.level), foundSymbol.address);
        }
        
        getToken();
    }

    else if (token == NUM_SYM)
    {
        gen(LIT, 0, getNumber());

        getToken();
    }

    else if (token == LPAREN_SYM)
    {
        getToken();

        expression();

        if (token != RPAREN_SYM)
            error(22);

        getToken();
    }

    else
        error(24);
}

void getToken()
{    
    char *symbolName;

    fscanf(ifp, "%d", &token);
}

char* getIdenName()
{
    char buffer[MAX_IDENTIFIERS_CHAR_LENGTH];
    char *str;

    fscanf(ifp, "%s", buffer);

    str = malloc ((strlen(buffer) + 1) * sizeof (char));

    strcpy(str, buffer);

    return str;
}

int isValidRelationalOperator()
{

    int relationalOperatorCheck = 0;

    if (token >= EQL_SYM && token <= GEQ_SYM)
        relationalOperatorCheck = 1;

    return relationalOperatorCheck;
}

void enter(char* name, int typeID, int level, int address, int constValue) {

    if (address > MAX_STACK_HEIGHT)
        error(28);

    symbolTable[symbolIndex].name = name;
    symbolTable[symbolIndex].typeID = typeID;
    symbolTable[symbolIndex].level = level;
    symbolTable[symbolIndex].address = address;
    symbolTable[symbolIndex].constValue = constValue;
    
    symbolIndex++;
}

void gen(int OP, int L, int M) {
    if (codeIndex > MAX_CODE_LENGTH)
        error(27);

    else
    {
        parseCode[codeIndex].OP = OP;
        parseCode[codeIndex].L = L;
        parseCode[codeIndex].M = M;

        codeIndex++;
    }
}

Symbol find(char* symbolName) {

    int i;

    for (i = (symbolIndex - 1); i >= 0; i--) 
    {
        if (strcmp(symbolName, symbolTable[i].name) == 0)
        {
            return symbolTable[i];
        }
    }

    error(11);
}

int getNumber()
{    
    int num;

    if (token == NUM_SYM)
        fscanf(ifp, "%d", &num);

    return num;
}

void whileBlock()
{
    int codeIndex1 = codeIndex, codeIndex2;

    getToken();

    condition();

    codeIndex2 = codeIndex;

    gen(JPC, 0, 0);
    
    if (token != DO_SYM)
        error(18);
    
    getToken();

    statement();

    gen(JMP, 0, codeIndex1);

    parseCode[codeIndex2].M = codeIndex;
}

void ifBlock()
{
    int codeIndex1;

    getToken();

    condition();
    
    if (token != THEN_SYM)
        error(16);
    
    getToken();

    codeIndex1 = codeIndex;

    gen(JPC, 0, 0);

    statement();

    parseCode[codeIndex1].M = codeIndex;
}

int relativeSymbolLevel(int symbolLevel)
{
    return lexLevel - symbolLevel;
}

// Maps PL0 Language Symbols to PM0 Machine Codes (Required due to inconsistencies in given tables)
int mapOPRCode(int token)
{
    switch(token)
    {
        case EQL_SYM:
            return EQL;
        case NEQ_SYM:
            return NEQ;
        case LESS_SYM:
            return LSS;
        case LEQ_SYM:
            return LEQ;
        case GTR_SYM:
            return GTR;
        case GEQ_SYM:
            return GEQ;
        default:
            // Just in case this is called with something other than relational operators
            // This will cause the VM to error
            return -99;
    }
}

void error(int err_id)
{
    switch(err_id)
    {
        case 1:
            printf("Use = instead of :=\n");
            exit(0);
        case 2:
            printf("= must be followed by a number.\n");
            exit(0);
        case 3:
            printf("Identifier must be followed by =.\n");
            exit(0);
        case 4:
            printf("const, var, procedure must be followed by identifier.\n");
            exit(0);
        case 5:
            printf("Semicolon or comma missing.\n");
            exit(0);
        case 6:
            printf("Incorrect symbol after procedure declaration.\n");
            exit(0);
        case 7:
            printf("Statement expected.\n");
            exit(0);
        case 8:
            printf("Incorrect symbol after statement part in block.\n");
            exit(0);
        case 9:
            printf("Period expected.\n");
            exit(0);
        case 10:
            printf("Semicolon between statements missing.\n");
            exit(0);
        case 11:
            printf("Undeclared identifier.\n");
            exit(0);
        case 12:
            printf("Assignment to constant or procedure is not allowed.\n");
            exit(0);
        case 13:
            printf("Assignment operator expected.\n");
            exit(0);
        case 14:
            printf("Call must be followed by an identifier.\n");
            exit(0);
        case 15:
            printf("Call of a constant or variable is meaningless.\n");
            exit(0);
        case 16:
            printf("then expected.\n");
            exit(0);
        case 17:
            printf("Semicolon or } expected.\n");
            exit(0);
        case 18:
            printf("do expected.\n");
            exit(0);
        case 19:
            printf("Incorrect symbol following statement.\n");
            exit(0);
        case 20:
            printf("Relational operator expected.\n");
            exit(0);
        case 21:
            printf("Expression must not contain a procedure identifier.\n");
            exit(0);
        case 22:
            printf("Right parenthesis missing.\n");
            exit(0);
        case 23:
            printf("The preceding factor cannot begin with this symbol.\n");
            exit(0);
        case 24:
            printf("An expression cannot begin with this symbol.\n");
            exit(0);
        case 25:
            printf("This number is too large.\n");
            exit(0);
        case 26:
            printf("end expected.\n");
            exit(0);
        case 27:
            printf("Code length too long.\n");
            exit(0);
        case 28:
            printf("Stack Overflow! Stack can't be larger than the MAX_STACK_HEIGHT\n");
            exit(0);
        default:
            printf("Undefined error message.\n");
            exit(0);
    }
}

#endif