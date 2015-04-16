// Craig Livingston and Samuel Davis
// COP 3402 - 3/26/15
// Assignment #3 - Tiny Compiler
// Compile Driver

#include <stdio.h>

#include "scan.h"
#include "parse.h"
#include "vm.h"
#include "files.h"
#include "pl0_constants.h"
#include "pm0_constants.h"

void printFile(char *fileToPrint);

int main(int argc, char* argv[])
{
    int i;
    
    scan();
    parse();
    vm();
 
    for (i=1; i < argc; i++)
    {
        if (strcmp(argv[i],"-l") == 0)
        {
            printf("Lexeme List:\n");
            printFile(LIST_FILE);
            printf("\n");
        }
        else if (strcmp(argv[i],"-v") == 0)
        {
            printf("VM Stack Trace:\n");
            printFile(STACKTRACE_FILE);
        }
        else if (strcmp(argv[i],"-a") == 0)
        {
            printf("Assembly Code:\n");
            printFile(CODE_FILE);
        }
    }
    
    return 0;
}

void printFile(char *fileToPrint)
{
    int c;
    FILE *file;
    file = fopen(fileToPrint, "r");
    if (file) {
        while ((c = getc(file)) != EOF)
            putchar(c);
        fclose(file);
    }
}