#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct
{
	int OP;
	int L;
	int M;

} Instruction;

typedef struct
{
    char* value;
    int id;
    int type;
    int error;
    int endOfFile;
    
} Token;

typedef struct
{
    char *name;
    int id;
    int level;
    int stackPointer;
    
} Symbol; 
    
#endif