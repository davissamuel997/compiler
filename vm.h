#ifndef VM_H
#define VM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"
#include "structs.h"
#include "pm0_constants.h"

#define MAXSTACK 2000
#define MAXCODE 500

// PROTOTYPES
extern void vm();
int* initStack(int maxSize);
Instruction* initCode(int maxCodeSize);
Instruction* readCode(int maxCodeSize);
int endOfProgram();
void fetch();
void execute();
void executeOPR();
int base();
char* opToString(int OP);
char* stackToString();
char* intToString(int num);
void startStackTrace(FILE* outputFilePtr);

// GLOBALS
int* stack;
int PC;
int BP;
int SP;
Instruction IR;
Instruction* code;
int linesOfCode;

// Invokes virtual machine
void vm()
{
	// Initialize variables
	FILE* ofp = fopen(STACKTRACE_FILE, "w");
	stack = initStack(MAXSTACK);
	code = readCode(MAXCODE);
	
	PC = 0;
	BP = 0;
	SP = -1;

	int line;
	char* stackString;
	
	// Start display to screen
	printf("Output:\n");
	
	// Mirror Code & Start Stack Trace to Debug File
	startStackTrace(ofp);
	
	// Loop the code until end state
	while(!endOfProgram())
	{
		// Store the line number before fetch & execute
		line = PC;
		fetch();
		execute();
		
		fprintf(ofp, "%3d%7s%4d%6d", line, opToString(IR.OP), IR.L, IR.M);
		fprintf(ofp, "%6d%6d%6d   ", PC, BP, SP);
		
		stackString = stackToString();
		fprintf(ofp, "%s\n", stackString);
		free(stackString);
	}
	
	free(code);
	free(stack);
	fclose(ofp);
}

// Initialize stack
int* initStack(int maxSize)
{
	return calloc(maxSize, sizeof(int));
}

// Initialize code 
Instruction* initCode(int maxCodeSize)
{
	return calloc(maxCodeSize, sizeof(Instruction));
}

// Read Code
Instruction* readCode(int maxCodeSize)
{
	// Variables
	int OP, L, M;
	int i, line = 0;
	Instruction* buffer = initCode(maxCodeSize);
	Instruction* code;
	
	// Open file
	FILE* fp = fopen(CODE_FILE, "r");
	
	// If valid file
	if (fp != NULL)
	{
		// While not end of file, keep scanning Instruction triplets
		while((fscanf(fp, "%d %d %d", &OP, &L, &M)) != EOF)
		{
			buffer[line].OP = OP;
			buffer[line].L = L;
			buffer[line].M = M;
			line++;
		}
	}

	fclose(fp);
	
	// Allocate the correct size code array and dump the buffer
	code = calloc(line, sizeof(Instruction));
	
	for (i=0; i < line; i++)
	{
		code[i] = buffer[i];
	}
	
	linesOfCode = line;
	free(buffer);
	
	return code;
}

// Returns 1 if program has reached end state (OPR 0 0, with SP = -1)
int endOfProgram()
{
	if (SP == -1 && IR.OP == OPR && IR.L == 0 && IR.M == 0)
		return 1;
	
	return 0;
}

// Fetches the next Instruction and increments the program counter
void fetch()
{
	IR = code[PC];
	PC += 1;
}

// Executes the Instruction in the Instruction register
void execute()
{
	int input;
	
	switch(IR.OP)
	{
		// Push literal on stack
		case (LIT):
			SP += 1;
			stack[SP] = IR.M;
			break;
			
		// Execute OPR Instruction (uses M value)
		case (OPR):
			executeOPR();
			break;
		
		// Load stack value
		case (LOD):
			SP += 1;
			stack[SP] = stack[ base() + IR.M];
			break;
			
		// Store value in stack
		case (STO):
			stack[ base() + IR.M] = stack[SP];
			SP -= 1;
			break;
			
		// Call Procedure
		case (CAL):
			stack[SP+1] = base();
			stack[SP+2] = BP;
			stack[SP+3] = PC;
			BP = SP + 1;
			PC = IR.M;
			break;
		
		// Increment the stack
		case (INC):
			SP += IR.M;
			break;
		
		// Jump unconditional
		case (JMP):
			PC = IR.M;
			break;
			
		// Jump conditional
		case (JPC):
			if (stack[SP] == 0)
				PC = IR.M;
			SP -= 1;
			break;
		
		// Output to screen
		case (OUT):
			printf("%d\n", stack[SP]);
			SP -= 1;
			break;
		
		// Input from user
		case (IN):
			SP += 1;
			printf("Input integer value: ");
			scanf("%d", &stack[SP]);
			break;
		
		// Invalid Instruction
		default:
			break;
	}
}

// Executes OPR Instructions using M value
void executeOPR()
{
	switch(IR.M)
	{
		// Return to caller
		case(RET):
			SP = BP - 1;
			PC = stack[SP + 3];
			BP = stack[SP + 2];
			break;
		
		// Negate
		case(NEG):
			stack[SP] *= -1;
			break;
		
		// Add
		case(ADD):
			SP -= 1;
			stack[SP] = stack[SP] + stack[SP + 1];
			break;
			
		// Subtract
		case(SUB):
			SP -= 1;
			stack[SP] = stack[SP] - stack[SP + 1];
			break;
			
		// Multiply
		case(MUL):
			SP -= 1;
			stack[SP] = stack[SP] * stack[SP + 1];
			break;
			
		// Divide
		case(DIV):
			SP -= 1;
			stack[SP] = stack[SP] / stack[SP + 1];
			break;
		
		// Odd
		case(ODD):
			stack[SP] = (stack[SP] %2 == 0) ? 0 : 1;
			break;
		
		// Modulus
		case(MOD):
			SP -= 1;
			stack[SP] = stack[SP] % stack[SP + 1];
			break;
			
		// Equal
		case(EQL):
			SP -= 1;
			stack[SP] = (stack[SP] == stack[SP+1]) ? 1 : 0;
			break;
			
		// Not Equal
		case(NEQ):
			SP -= 1;
			stack[SP] = (stack[SP] != stack[SP+1]) ? 1 : 0;
			break;
		
		// Less than
		case(LSS):
			SP -= 1;
			stack[SP] = (stack[SP] < stack[SP+1]) ? 1 : 0;
			break;
			
		// Less than or equal
		case(LEQ):
			SP -= 1;
			stack[SP] = (stack[SP] <= stack[SP+1]) ? 1 : 0;
			break;
		
		// Greater than
		case(GTR):
			SP -= 1;
			stack[SP] = (stack[SP] > stack[SP+1]) ? 1 : 0;		
			break;
		
		// Greater than or equal
		case(GEQ):
			SP -= 1;
			stack[SP] = (stack[SP] >= stack[SP+1]) ? 1 : 0;
			break;
		
		// Invalid M
		default:
			break;
	}
}

// Returns base L levels down stack
int base()
{
	int baseLevel = BP;
	int level = IR.L;
	
	while (level > 0)
	{
		baseLevel = stack[baseLevel];
		level--;
	}
	
	return baseLevel;
}

// Returns string based on OP's int value
char* opToString(int OP)
{
	switch(OP)
	{
		case 1:
			return "lit";
		case 2:
			return "opr";
		case 3:
			return "lod";
		case 4:
			return "sto";
		case 5:
			return "cal";
		case 6:
			return "inc";
		case 7:
			return "jmp";
		case 8:
			return "jpc";
		case 9:
			return "out";
		case 10:
			return " in";
		default:
			return "   ";
	}
}

char* stackToString()
{	
	// Buffer for output String
	int index = 0;
	int stackSize = (SP < 0) ? (SP * -1) : SP;
	char* buffer = calloc((stackSize * 10),sizeof(char));
	
	// Array of BPs
	int tempBP = BP;
	int indexBP = (tempBP <= 0) ? -1 : 0;
	int arrayBP[stackSize];
	
	// Output string
	char* temp;
	char* str;
	
	// If no stack (SP == -1)
	if (SP < 0)
		buffer[0] = '\0';

	// Find activation records (using base pointer values)
	while (SP >= 0 && indexBP >= 0 && tempBP != 0)
	{
		arrayBP[indexBP] = tempBP;
		tempBP = stack[tempBP];
		
		if (tempBP != 0)
			indexBP++;
	}
	
	// Read the stack until you hit SP
	while (SP >= 0 && index <= SP)
	{
		// if reading a BP, add activation record marking
		if (indexBP >= 0 && index == arrayBP[indexBP])
		{
			strcat(buffer, "| ");
			indexBP--;
		}
		
		// Concatenate the next stack value to the string
		temp = intToString(stack[index]);
		strcat(buffer, temp);
		free(temp);
		index++;
		
		// Add space if not last value in stack
		if (index <= SP)
			strcat(buffer, " ");
	}

	// Allocate & dump buffer
	str = calloc(strlen(buffer) + 1, sizeof(char));
	strcpy(str, buffer);
	free(buffer);
	
	return str;
}

// Returns a string representation of an integer
char* intToString(int num)
{
	// Variables
	int len = 0;
	int n = num;
	int neg = 0;
	int rem;
	int i;	
	char *str;
	
	// Case: 0
	if (num == 0)
	{
		// Done as calloc, since function dynamically allocates in other case
		str = calloc(2, sizeof(char));
		strcpy(str, "0");
		return str;
	}
		
	// Determine digit length of number
	while (n != 0)
	{
		len++;
		n /= 10;
	}
	
	// Negatives
	if (num < 0)
	{
		neg = 1;
		len++;
		num *= -1;
	}
	
	// Allocate a string
	str = calloc(len+1,sizeof(char));
	
	// Continually mod & divide by 10 to get digits
	for (i=0; i < len; i++)
	{
		rem = num % 10;
		num /= 10;
		str[len - i - 1] = rem + '0';
	}
	
	// Sign for negative number
	if (neg)
		str[0] = '-';
	
	str[len] = '\0';
	
	return str;
}

// Mirrors contents of code array in file & adds header to stack trace
void startStackTrace(FILE* outputFilePtr)
{
	int i;
	
	if (outputFilePtr != NULL)
	{
		// Print Header
		fprintf(outputFilePtr, "Line   OP    L    M\n");
	
		for (i=0; i < linesOfCode; i++)
		{
			fprintf(outputFilePtr, "%3d%7s%4d%6d\n", i, opToString(code[i].OP), code[i].L, code[i].M);
		}
		
		fprintf(outputFilePtr, "\n");
		
		fprintf(outputFilePtr, "%46s\n", "pc    bp    sp   stack"); 
		fprintf(outputFilePtr, "Initial values%12s%6s%6s\n", "0", "0", "-1");
	}
}

#endif