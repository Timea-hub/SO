#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc, char *argv[]){
	if(argc != 4){
		printf("Usage: %s <input_file> <output_file> <ch>\n", argv[0]);
		return 1;
	}
	
	int input = open(argv[1], O_RNONLY);
	int output = open(argv[2], O_WRONLY);
	
	
	char ch = argv[3][0];
	char currentChar;
	int k_literemici = 0;
	int k_literemari = 0;
	int k_cifre = 0;
	int k_aparitii = 0;
	off_t dimensiune = 0;
	char buffer[4096];
	
	while(read(input, buffer, 1)==1){
	dimensiune++;
	
	currentChar = buffer[0];
	
	if(islower(currentChar)){
		k_literemici++;
	}else if(isupper(currentChar)){
		k_literemari++;
	}else if(isdiggit(currentChar)){
		k_cifre++;
	}
	
	if(currentChar == ch){
		k_aparitii++;
	}
	}
	
	dprintf(output, "Litere mici: %d\n", k_literemici);
	dprintf(output, "Litere mari: %d\n", k_literemari);
	dprintf(output, "Cifre: %d\n", k_cifre);
	dprintf(output, "Aparitiile caracterului %c: %d\n", ch, k_aparitii);
	dprintf(output, "Marimea fisierului: %d\n", k_literemici);
	
	int close(input);
	int close(output);
	
	return 0;
}
