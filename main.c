#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct  {
    unsigned int funct:6;
    unsigned int shamt:5;
    unsigned int regd:5;
    unsigned int regt:5;
    unsigned int regs:5;
    unsigned int opcode:6;
} instr_type_R;


typedef struct {
    unsigned int imm:16;
    unsigned int regt:5;
    unsigned int regs:5;
    unsigned int opcode:6;
} instr_type_I;


union {
    unsigned int raw;
    instr_type_R R;
    instr_type_I I;
} instr;

// struct instr_type_J {
//     unsigned int opcode:6;
//     unsigned int offset:26;
// } itj;

// CPU components
int REGISTERS[32];
int PC;
int MEMORY[250];
unsigned int CODE[1024];
int CODE_LENGTH = 0;

// decimal to integer conversion
int fromBinary(const char *s) {
  return (int) strtol(s, NULL, 2);
}

void file_load(char* input_file) {
    char *line = (char *)malloc(sizeof(char) * 48);
    char dsc1, dsc2, dsc3;
    FILE *fin;
    int i1, i2;
    unsigned int i3;
    fin = fopen(input_file, "r");

    fgets(line, 48, fin); // Read the first line from file

    int j = 0;
    while(strcmp(line, "MEMORY\r\n")!=0 && j++ < 32) {
        //Process REGISTER section        
        if (strcmp(line, "REGISTERS\r\n")!=0) {
            // process line
            sscanf(line, "%c%d %d", &dsc1, &i1, &i2);
            REGISTERS[i1] = i2;
        }
        fgets(line, 48, fin); // Read the line from file
    }

    while(strcmp(line, "CODE\r\n")!=0){
        // Process MEMORY section
        if (strcmp(line, "MEMORY\r\n")!=0) {
            // process line
            sscanf(line, "%d %d", &i1, &i2);
            MEMORY[i1 / 4] = i2;
        }
        fgets(line, 48, fin); // Read the line from file
    }

    while(!feof(fin)) {
        // Process CODE section
        if (strcmp(line, "CODE\r\n")!=0) {
            // process line
            CODE[CODE_LENGTH] = fromBinary(line);
            ++CODE_LENGTH;
        }
        
        fgets(line, 48, fin); // Read the line from file
    }
    fclose(fin);
}



void cpu( /* char* output_file */ ) {

    while (PC < CODE_LENGTH) {

        // IF
        unsigned int CodeVal = CODE[PC];
        char InstructType = '0';
        
        // ID (indentify opcodes: 8 possible) 0 R, 2 or 3 is J, and then everything else is I
        int opcode = (CodeVal >> 26);
        switch(opcode){
            case 0: 
                InstructType = 'R';
                instr.raw = CodeVal;
                printf("Opcode: %u\n", instr.R.opcode);
                break;

            case 2 || 3: 
                InstructType = 'J';
                break;

            default: 
                InstructType = 'I';
                instr.raw = CodeVal;
                printf("Opcode: %u\n", instr.I.opcode);
                break;
            
        } 
        
        // RF

        // ALU / EX

        // MEM

        // WB

        // PC++
        PC++;
    }
}

int main(int argc, char const *argv[])
{
    /* Ask for user input */
    char* fni = (char *)malloc(sizeof(char) * 32);
    char* fno = (char *)malloc(sizeof(char) * 32);

    printf("Input File: ");
    scanf("%s", fni);

    //printf("Output File: ");
    //scanf("%s", fno);

    printf("Load Program\n");
    file_load(fni);
    printf("CPU Run\n");
    cpu();

    // printf("");

    return 0;
}