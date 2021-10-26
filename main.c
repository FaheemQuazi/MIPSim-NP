#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// CPU components
int REGISTERS[32];
int PC;
int MEMORY[250];
int CODE[1024];
int CODE_LENGTH = 0;

// decimal to integer conversion
int fromBinary(const char *s) {
  return (int) strtol(s, NULL, 2);
}

void file_load(char* input_file) {
    char *line = (char *)malloc(sizeof(char) * 48);
    char dsc1, dsc2;
    FILE *fin;
    int i1, i2;
    fin = fopen(input_file, "r");

    fscanf(fin, "%s", line); // Read the first line from file
    fscanf(fin, "%c", &dsc1); // Read end of line char

    while(strcmp(line, "MEMORY")!=0){
        // Process REGISTER section        
        if (strcmp(line, "REGISTERS")!=0) {
            // process line
            sscanf(line, "%c %d %d %c", &dsc1, &i1, &i2, &dsc2);
            REGISTERS[i1] = i2;
        }
        fscanf(fin, "%s", line); // Read the line from file
        fscanf(fin, "%c", &dsc1); // Read end of line char
    }

    while(strcmp(line, "CODE")!=0){
        // Process MEMORY section
        if (strcmp(line, "MEMORY")!=0) {
            // process line
            sscanf(line, "%d %d %c", &i1, &i2, &dsc1);
            MEMORY[i1 / 4] = i2;
        }
        fscanf(fin, "%s", line); // Read the line from file
        fscanf(fin, "%c", &dsc1); // Read end of line char
    }

    while(!feof(fin)) {
        // Process CODE section
        if (strcmp(line, "CODE")!=0) {
            // process line
            CODE[CODE_LENGTH] = fromBinary(line);
            ++CODE_LENGTH;
        }
        fscanf(fin, "%s", line); // Read the line from file
        fscanf(fin, "%c", &dsc1); // Read end of line char
    }
    fclose(fin);
}



void cpu(char* output_file) {
    bool reg_dest;
    bool branch;
    bool alu_op;
    bool mem_write;
    bool alu_src;
    bool reg_write;

    bool SPIN = false;
    while (!SPIN) {
        // IF
        int inst = CODE[PC];
        
        // ID and RF (indentify opcodes: 8 possible)
        // (inst >> 25) & 0b000000;
        //I-load word:  100011 
        //I-store word: 101011
        //R-addi:       001000
        //I-BEQ:        000100
        //I-BNE:        000101
        
        //Send to ALU 
        //R-Add:        000000 funct: 100000        
        //R-Sub:        000000 funct: 100010        
        //R-SLT:        000000 funct: 101010
        
        

        // ALU / EX

        // MEM

        // WB

        // PC++
    }
}

int main(int argc, char const *argv[])
{
    /* Ask for user input */
    char* fni = (char *)malloc(sizeof(char) * 32);
    char* fno = (char *)malloc(sizeof(char) * 32);

    printf("Input File: ");
    scanf("%s", fni);

    printf("Output File: ");
    scanf("%s", fno);

    printf("Load File\n");
    file_load(fni);

    for (int i = 0; i < CODE_LENGTH; i++) {
        printf("%u\n", CODE[i]);
    }

    return 0;
}