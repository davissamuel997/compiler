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
    int typeID;
    int level;
    int address;
    int constValue;
    
} Symbol; 
    
#endif