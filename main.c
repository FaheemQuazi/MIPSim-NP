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
    int imm:16;
    unsigned int regt:5;
    unsigned int regs:5;
    unsigned int opcode:6;
} instr_type_I;

typedef struct {
    int imm:26;
    unsigned int opcode:6;
} instr_type_J;

union {
    unsigned int raw;
    instr_type_R R;
    instr_type_I I;
    instr_type_J J;
} instr;

// CPU components
int REGISTERS[32];
int MEMORY[250];
int PC = 0;
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

void file_dumpRegMem(FILE* fout) {
    // assume fout is already opened
    int i;

    // Print register info to output file
    fprintf(fout, "\nREGISTERS\n");
    for(i = 0; i < 32; ++i){
        if(REGISTERS[i] != 0){
            fprintf(fout, "R%d %d\n", i, REGISTERS[i]);
        }
    }

    // Print memory info to output file
    fprintf(fout, "\nMEMORY\n");
    for(i = 0; i < 250; ++i){
        if(MEMORY[i] != 0){
            fprintf(fout, "%d %d\n", i*4, MEMORY[i]);
        }
    }
}

void cpu(char* output_file) {
    FILE *fout = fopen(output_file, "w");
    
    int clock = 1;   
    int ic = 1;
    bool rwMem = false;
    bool wbReg = false;

    while (PC < CODE_LENGTH) {
        rwMem = false;
        wbReg = false;

        // IF (and increment PC)
        fprintf(fout, "C#%d I%d-IF\n", clock++, ic);
        unsigned int CodeVal = CODE[PC++];
        
        // ID (indentify opcodes: 8 possible) 0 R, 2 or 3 is J, and then everything else is I
        fprintf(fout, "C#%d I%d-ID\n", clock++, ic);
        int opcode = (CodeVal >> 26);
        char InstructType = '0';
        switch(opcode)
        {
            case 0: //add, sub, slt
                InstructType = 'R';
                instr.raw = CodeVal;
                break;

            case 2 || 3: 
                InstructType = 'J';
                break;

            default: //storeword, loadword, addi, beq, bne
                InstructType = 'I';
                instr.raw = CodeVal;
                break;
            
        } 
        
        // ALU / EX
        fprintf(fout, "C#%d I%d-EX\n", clock++, ic);
        switch (InstructType)
        {
            case 'R':
                switch (instr.R.funct)
                {
                    case 0b100000: //ADD
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] + REGISTERS[instr.R.regt];
                        wbReg = true;
                        break;

                    case 0b100010: //SUB
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] - REGISTERS[instr.R.regt];
                        wbReg = true;
                        break;               

                    case 0b101010: //SLT 
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] < REGISTERS[instr.R.regt];
                        wbReg = true;
                        break;
                }
                break;
            case 'I':
                switch (instr.R.opcode)
                {
                    case 0b101011: //SW
                        MEMORY[(REGISTERS[instr.I.regs] + instr.I.imm) / 4] = REGISTERS[instr.I.regt];
                        rwMem = true;
                        break; 

                    case 0b100011: //LW
                        REGISTERS[instr.I.regt] = MEMORY[(REGISTERS[instr.I.regs] + instr.I.imm) / 4];
                        wbReg = true;
                        rwMem = true;
                        break;

                    case 0b001000: //ADDI
                        REGISTERS[instr.I.regt] = REGISTERS[instr.I.regs] + REGISTERS[instr.I.imm];
                        wbReg = true;
                        break;

                    case 0b000100: //BEQ
                        if (REGISTERS[instr.I.regs] == REGISTERS[instr.I.regt]) {
                            PC += instr.I.imm;
                        }
                        break;

                    case 0b000101: //BNE
                        if (REGISTERS[instr.I.regs] != REGISTERS[instr.I.regt]) {
                            PC += instr.I.imm;
                        }
                        break;
                    
                }
            
                break;
            case 'J':
                switch (instr.J.opcode) {
                    case 0b000010: // J
                        PC += instr.J.imm;
                        break;

                    case 0b000011: // JAL
                        REGISTERS[30] = PC;
                        PC += instr.J.imm;
                        break;
                }
                break;
        }

        // MEM/WB
        if (rwMem) {
            fprintf(fout, "C#%d I%d-MEM\n", clock++, ic);
        }
        if (wbReg) {
            fprintf(fout, "C#%d I%d-WB\n", clock++, ic);
        }

        ic++;
    }

    file_dumpRegMem(fout);
    fclose(fout);
}

int main(int argc, char const *argv[])
{
    /* set initial values */
    REGISTERS[0] = 0;

    /* Ask for user input */
    char* fni = (char *)malloc(sizeof(char) * 32);
    char* fno = (char *)malloc(sizeof(char) * 32);

    printf("Input File: ");
    scanf("%s", fni);

    printf("Output File: ");
    scanf("%s", fno);

    printf("Load Program\n");
    file_load(fni);

    printf("CPU Run\n");
    cpu(fno);
    
    printf("Done\n");

    return 0;
}