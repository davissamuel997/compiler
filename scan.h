#ifndef SCAN_H
#define SCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"
#include "structs.h"
#include "pl0_constants.h"

#define MAX_NUM_LENGTH 5
#define MAX_TOKEN_LENGTH 11

#define ERROR_INVALID_VAR_NAME 1
#define ERROR_NUM_TOO_LONG 2
#define ERROR_VAR_TOO_LONG 3
#define ERROR_INVALID_SYMBOL 4

// PROTOTYPES
extern void scan();
void createCleanInput(FILE* input, FILE* cleanOutput);
Token* getNextToken(FILE* cleanInput);
int isValidSymbol(char c);
int getID(Token* t);
void startLexemeTable(FILE *ifp);
void writeLexemeTable(FILE* ifp, Token* token);
void writeLexemeList(FILE* ifp, Token* token);
void printError(int errorNum);

void scan()
{
    // Token
    Token* token;
    
    // Files
    FILE *input = fopen(INPUT_FILE, "r");
    FILE *clean = fopen(CLEAN_FILE, "w+");
    FILE *table = fopen(TABLE_FILE, "w");
    FILE *list  = fopen(LIST_FILE, "w");
    
    // Parse flags
    int error, endOfFile;
    
    // Create the clean input file & seek back to file start
    createCleanInput(input, clean);
    fseek(clean, 0, SEEK_SET);
    
    // Create the header to the lexeme table file
    startLexemeTable(table);
    
    // Parse clean input
    do
    {
        // Get next token
        token = getNextToken(clean);
           
        // If valid token, write to lexeme table and list
        if (token->endOfFile != EOF && !(token->error))
        {
            writeLexemeList(list, token);
            writeLexemeTable(table, token);
        }
        // Else if error, output error to console
        else if (token->error)
        {
            printError(token->error);
        }
        
        // Clean up token memory
        error = token->error;
        endOfFile = token->endOfFile;
        free(token->value);
        free(token);
    
    } while (endOfFile != EOF && !error);
        
    // Close out file pointers
    fclose(input);
    fclose(clean);
    fclose(table);
    fclose(list);
}

// Reads in a PL/0 File and Removes Comments -- Outputs to CLEAN_FILE
void createCleanInput(FILE* input, FILE* cleanOutput)
{
    int prevChar = -99;
    int currChar = -99;
    int comment = 0;
    int offset = 0;
    
    if (input != NULL)
    {
        // Read char by char until end of file
        while ((currChar = fgetc(input)) != EOF)
        {
            // If read '/*', start comment (ignore)
            if (prevChar == (int) '/' && currChar == (int) '*')
            {
                comment = 1;
            }
            // If commenting and read '*/' - end comment
            else if (comment && prevChar == (int) '*' && currChar == (int) '/')
            {
                // Offset needed to ignore closure
                comment = 0;
                offset = 2;                
            }
            
            // If not commenting and have read at least two chars -- write to file
            if (!comment && prevChar != -99)
            {
                (!offset) ? fprintf(cleanOutput, "%c", (char) prevChar) : offset--;
            }
            
            prevChar = currChar;
        }
        
        // If not commenting at this point, add the last character
        if (!comment)
            fprintf(cleanOutput, "%c", (char) prevChar);
    }
}

// Returns the next token or error code if next token is invalid
Token* getNextToken(FILE* cleanInput)
{
    Token* token = calloc(1, sizeof(Token));
    char buffer[MAX_TOKEN_LENGTH + 1];
    char c;
    int index = 0;
    int end_token = 0;
        
    while(!end_token && (fscanf(cleanInput, "%c", &c) != EOF))
    {           
        // If not set yet, determine type (from first non-whitespace char)
        if (token->type == NOT_SET && !isspace(c))
        {
            buffer[index++] = c;
            
            if (isalpha(c))
            {
                token->type = ALPHA;
            }
            else if (isdigit(c))
            {
                token->type = NUMBER;
            }
            else if (isValidSymbol(c))
            {
                token->type = SYMBOL;
            }
            else
            {
                // Invalid non-whitespace read (invalid symbol)
                end_token = 1;
                token->error = ERROR_INVALID_SYMBOL;
                strcpy(buffer, "Error 4");
            }
        }
        // Else continue reading token (actions determined by type)
        else
        {
            // TYPE: ALPHA
            if (token->type == ALPHA)
            {
                // If digit or alpha add to buffer and keep reading token
                if ((isalpha(c) || isdigit(c)) && index < MAX_TOKEN_LENGTH)
                {
                    buffer[index++] = c;
                }
                else if ((isalpha(c) || isdigit(c)) && index >= MAX_TOKEN_LENGTH)
                {
                    token->error = ERROR_VAR_TOO_LONG;
                    strcpy(buffer, "Error 3");
                }
                // Else (symbol or white space) end token
                else
                {
                    end_token = 1;
                    fseek(cleanInput, -1, SEEK_CUR);
                }  
            }
            // TYPE: NUMBER
            else if (token->type == NUMBER)
            {
                // If digit, add to buffer and keep reading token
                if (isdigit(c) && index < MAX_NUM_LENGTH)
                {
                    buffer[index++] = c;
                }
                else if (isdigit(c) && index >= MAX_NUM_LENGTH)
                {
                    token->error = ERROR_NUM_TOO_LONG;
                    strcpy(buffer, "Error 2");
                }
                // If alpha, invalid ident error
                else if (isalpha(c))
                {
                    end_token = 1;
                    token->error = ERROR_INVALID_VAR_NAME;
                    strcpy(buffer, "Error 1");
                }
                // Else white space or symbol, end number
                else
                {
                    end_token = 1;
                    fseek(cleanInput, -1, SEEK_CUR);
                }
            }
            // TYPE: SYMBOL
            else if (token->type == SYMBOL)
            {
                // If '<>', '<=', '>=' or ':=' -> add to buffer and end token
                if ((buffer[0] == '<' && c == '>') ||
                    (buffer[0] == '<' && c == '=') ||
                    (buffer[0] == '>' && c == '=') ||
                    (buffer[0] == ':' && c == '='))
                {
                    end_token = 1;
                    buffer[index++] = c;
                }
                // If next char is valid - end token
                else if (isalpha(c) || isdigit(c) || isspace(c) || isValidSymbol(c))
                {
                    end_token = 1;
                    fseek(cleanInput, -1, SEEK_CUR);
                }
                // If invalid, read an invalid symbol - return error
                else
                {
                    end_token = 1;
                    token->error = ERROR_INVALID_SYMBOL;
                    strcpy(buffer, "Error 4");
                }
            }
        }    

        // If not error, terminate the buffer string (gets overwritten if more added)
        if (!token->error)
            buffer[index] = '\0';        
    }
    
    // End of File Test
    if (index == 0)
    {
        strcpy(buffer, "EOF");
        token->endOfFile = EOF;
    }
    
    // Copy buffer to string and return
    token->value = malloc(sizeof(char) * (strlen(buffer) + 1));
    strcpy(token->value,buffer);
    
    // Determine id for token
    if (!(token->error) && token->endOfFile != EOF)
        token->id = getID(token);
    
    return token;
}

// Returns 1 if c is a valid PL/0 symbol
int isValidSymbol(char c)
{
    if (c == '+' ||
        c == '-' ||
        c == '*' ||
        c == '/' ||
        c == '=' ||
        c == '<' ||
        c == '>' ||
        c == '(' ||
        c == ')' ||
        c == ',' ||
        c == ';' ||
        c == '.' ||
        c == ':')
        
        return 1;
        
    else
        return 0;
}

// Returns the ID for a given token (based on keywords and symbol table)
int getID(Token* t)
{
    // Type:  ALPHA
    if (t->type == ALPHA)
    {
        // Test for keywords
        if (strcmp(t->value, "odd") == 0)
            return ODD_SYM;
        if (strcmp(t->value, "begin") == 0)
            return BEGIN_SYM;
        if (strcmp(t->value, "end") == 0)
            return END_SYM;
        if (strcmp(t->value, "if") == 0)
            return IF_SYM;
        if (strcmp(t->value, "then") == 0)
            return THEN_SYM;
        if (strcmp(t->value, "while") == 0)
            return WHILE_SYM;
        if (strcmp(t->value, "do") == 0)
            return DO_SYM;
        if (strcmp(t->value, "call") == 0)
            return CALL_SYM;
        if (strcmp(t->value, "const") == 0)
            return CONST_SYM;
        if (strcmp(t->value, "var") == 0)
            return VAR_SYM;
        if (strcmp(t->value, "procedure") == 0)
            return PROC_SYM;
        if (strcmp(t->value, "write") == 0)
            return WRITE_SYM;
        if (strcmp(t->value, "read") == 0)
            return READ_SYM;
        if (strcmp(t->value, "else") == 0)
            return ELSE_SYM;
        
        // Else, not a keyword, so ident
        else
            return IDENT_SYM;
    }
    
    // Type:  NUMBER
    else if (t->type == NUMBER)
    {
        return NUM_SYM;
    }
    // Type: SYMBOL (Invalid symbols should be caught in read)
    else if (t->type == SYMBOL)
    {
        if (strcmp(t->value, "+") == 0)
            return PLUS_SYM;
        if (strcmp(t->value, "-") == 0)
            return MINUS_SYM;
        if (strcmp(t->value, "*") == 0)
            return MULT_SYM;
        if (strcmp(t->value, "/") == 0)
            return SLASH_SYM;
        if (strcmp(t->value, "=") == 0)
            return EQL_SYM;
        if (strcmp(t->value, "<>") == 0)
            return NEQ_SYM;
        if (strcmp(t->value, "<") == 0)
            return LESS_SYM;
        if (strcmp(t->value, "<=") == 0)
            return LEQ_SYM;
        if (strcmp(t->value, ">") == 0)
            return GTR_SYM;
        if (strcmp(t->value, ">=") == 0)
            return GEQ_SYM;
        if (strcmp(t->value, "(") == 0)
            return LPAREN_SYM;
        if (strcmp(t->value, ")") == 0)
            return RPAREN_SYM;
        if (strcmp(t->value, ",") == 0)
            return COMMA_SYM;
        if (strcmp(t->value, ";") == 0)
            return SEMICOLON_SYM;
        if (strcmp(t->value, ".") == 0)
            return PERIOD_SYM;
        if (strcmp(t->value, ":=") == 0)
            return BECOMES_SYM;
    }
    // If all else fails
    return -99;
}

// Starts the lexeme table table
void startLexemeTable(FILE *ifp)
{
    fprintf(ifp, "%-20s%s\n", "lexeme", "token type");
}

// Writes a valid token to the lexeme table file
void writeLexemeTable(FILE* ifp, Token* token)
{
    fprintf(ifp, "%-20s%d\n", token->value, token->id);
}

// Writes a valid token to the lexeme list file
void writeLexemeList(FILE* ifp, Token* token)
{
    fprintf(ifp, "%d ", token->id);
    
    if (token->id == 2 || token->id == 3)
        fprintf(ifp, "%s ", token->value);    
}

// Prints out an error based on an error number
void printError(int errorNum)
{
    switch(errorNum)
    {
        case (ERROR_INVALID_VAR_NAME):
            printf("ERROR:  Variable does not start with a letter\n");
            break;
        case (ERROR_NUM_TOO_LONG):
            printf("ERROR:  Number too long\n");
            break;
        case (ERROR_VAR_TOO_LONG):
            printf("ERROR:  Variable too long\n");
            break;
        case (ERROR_INVALID_SYMBOL):
            printf("ERROR:  Invalid symbol encountered\n");
            break;
        default:
            printf("ERROR:  Invalid error code entered\n");
            break;
    }
}

#endif