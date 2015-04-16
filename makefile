all: vm.exe scan.exe compile.exe

vm.exe: vm.c vm.h files.h structs.h pm0_constants.h
	gcc -o vm.exe vm.c
    
scan.exe: scan.c scan.h files.h structs.h pl0_constants.h
	gcc -o scan.exe scan.c
    
compile.exe: compile.c parse.h scan.h vm.h files.h structs.h pl0_constants.h pm0_constants.h
	gcc -o compile.exe compile.c