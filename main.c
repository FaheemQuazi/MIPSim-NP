/*********************************************************
 *      MIPSim-NP : MIPS Non-Piplined Processor Sim      *
 *********************************************************
 * Authors: Faheem Quazi, Gabriel Torres, Arien Stanley
 * Class: ECE 5367 @ University of Houston
 * 
 * Description:
 *  This project aims to simulate the operation of a non-pipelined MIPS processor. It reads a config
 *  file (input.txt) which includes initial conditions for registers and RAM/Memory, and the instructions
 *  as a binary string. It then outputs the pipline steps it takes per instruction, and when code
 *  execution completes, the contents of RAM/memory and registers (output.txt).
 * 
 * Referencing https://opencores.org/projects/plasma/opcodes
 * Implemented Instructions:
 *  ALU: ADD, ADDI, ADDIU, ADDU, AND, ANDI, LUI, NOR, OR, ORI, SLT, SLTI, SLTIU, SLTU, SUB SUBU, XOR, XORI
 *  SHIFTER: SLL, SLLV, SRA, SRAV, SRL SRLV
 *  BRANCH: BEQ, BGEZ, BGEZAL, BGTZ, BLEZ, BLTZ, BLTZAL, BNE, J, JAL, JALR, JR
 *  MEMORY: LW, SW
 * Missing Instructions:
 *  MULTIPLY: all multiply instructions
 *  BRANCH: BREAK, MFC0, MTC0, SYSCALL
 *  MEMORY: All others not listed
 *  
 * 
 * Limitations:
 *  - Can only read proprietary config file format
 *  - Not all RISC/MIPS instructions are implemented
 *  - RAM/Memory stored as ints i.e. cannot pick an address that is not a multiple of 4.
 *  - Code ROM limited to 1024 instructions.
 *  - No write prevention to R0.
 * 
 * Extra Notes:
 * - This was done for a class project!
 * - Hi mom
 *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* Instruction Type Definitions */
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

/* store the current instruction here */
union {
    unsigned int raw;
    instr_type_R R;
    instr_type_I I;
    instr_type_J J;
} instr;

/* CPU components */
int REGISTERS[32];          // 32 Registers
int MEMORY[250];            // 250*4 blocks of RAM
int PC = 0;                 // Program Counter
unsigned int CODE[1024];    // Code ROM
int CODE_LENGTH = 0;        // Position of last instruction

// convert binary string to integer
int fromBinary(const char *s) {
  return (int) strtol(s, NULL, 2);
}

/* Handle loading of input file */
void file_load(char* input_file) {
    char *line = (char *)malloc(sizeof(char) * 48); // Line read from input file
    char dsc1, dsc2, dsc3; // Misc. characters read from line to be discarded
    FILE *fin; // Input file pointer
    int i1, i2; // Used for register number and content, memory location and content
    fin = fopen(input_file, "r"); // Open input file

    fgets(line, 48, fin); // Read the first line from file

    int j = 0; // Used to count registers read from file
    while(strcmp(line, "MEMORY\r\n")!=0 && j++ < 32) {
        // Process REGISTERS section       
        if (strcmp(line, "REGISTERS\r\n")!=0) {
            // Get register number and value from line
            sscanf(line, "%c%d %d", &dsc1, &i1, &i2);
            // Put register content into REGISTERS array
            REGISTERS[i1] = i2;
        }
        fgets(line, 48, fin); // Read the line from file
    }

    while(strcmp(line, "CODE\r\n")!=0){
        // Process MEMORY section
        if (strcmp(line, "MEMORY\r\n")!=0) {
            // Get memory location and value from line
            sscanf(line, "%d %d", &i1, &i2);
            // Put MEMORY content into MEMORY array
            MEMORY[i1 / 4] = i2;
        }
        fgets(line, 48, fin); // Read the line from file
    }
    // Read lines to the end
    while(!feof(fin)) {
        // Process CODE section
        if (strcmp(line, "CODE\r\n")!=0) {
            // Get instruction from line
            CODE[CODE_LENGTH] = fromBinary(line);
            ++CODE_LENGTH;
        }
        
        fgets(line, 48, fin); // Read the line from file
    }
    fclose(fin); // Close input file
}

/* Dump contents of registers and memory to file */
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

/* Main CPU function */
void cpu(char* output_file) {
    FILE *fout = fopen(output_file, "w");
    
    int clock = 1;          // Clock cycle count 
    int ic = 1;             // Instruction count
    bool rwMem = false;     // Did we read/write to memory/RAM?
    bool wbReg = false;     // Did we write back to a register?

    while (PC < CODE_LENGTH) {
        rwMem = false; 
        wbReg = false;

        /* IF: Printing the Clock number increment, as well as the instruction count*/
        fprintf(fout, "C#%d I%d-IF\n", clock++, ic);
        unsigned int CodeVal = CODE[PC++];
        
        // ID: Indentify opcodes - 8 possible: 0 is R, 2 or 3 is J, and then everything else is I
        fprintf(fout, "C#%d I%d-ID\n", clock++, ic); 
        int opcode = (CodeVal >> 26); //Shifting the Code Value we got 26 spots to the right to only focus on the opcode
        char InstructType = '0';

        /* Switch Statement to differentiate the opcodes from eachother to get the isntruction type */
        switch(opcode) 
        {
            case 0: //Add, Sub, Slt, etc. with an opcode of 0 are R-type.
                InstructType = 'R';
                instr.raw = CodeVal;
                break;

            case 2 || 3: //Any Jump Instruction opcodes seperated here.
                InstructType = 'J';
                instr.raw = CodeVal;
                break;

            default: //Store Word, Load Word, Addi, BEQ, and BNE are assigned the I type, as the default case.
                InstructType = 'I'; 
                instr.raw = CodeVal;
                break;
            
        } 
        
        /* ALU / EX Instructions carried out here using the Instruction type seperated in the previous step */
        fprintf(fout, "C#%d I%d-EX\n", clock++, ic);

        /* Switch Statement to use the function or opcode of the given instruction to determine the register values */
        switch (InstructType)
        {
            case 'R': // R-Type Instruction: Seperated into it's 3 Arithmetic operations 
            /* Nested Switch to use the instruction type as well as the opcode value of the given code value */
                switch (instr.R.funct)
                {
                    
                    case 0b100000: //ADD Addition
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] + REGISTERS[instr.R.regt]; // Rd = Rs + Rt
                        wbReg = true; //Enable the write back to register
                        break;

                    case 0b100010: //SUB Subtract
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] - REGISTERS[instr.R.regt]; // Rd = Rs - Rt
                        wbReg = true; //Enable the write back to register
                        break;

                    case 0b100011: //SUBU Unsigned Subtract
                        REGISTERS[instr.R.regd] = (int)((unsigned int)REGISTERS[instr.R.regs] - (unsigned int)REGISTERS[instr.R.regt]); // Rd = Rs - Rt
                        wbReg = true; //Enable the write back to register
                        break;
                        
                    case 0b100101: //OR
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] | REGISTERS[instr.R.regt];
                        wbReg = true;
                        break;
                        
                    case 0b101010: //SLT Set on Less Than
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] < REGISTERS[instr.R.regt]; // Rd = Rs < Rt
                        wbReg = true; //Enable the write back to register
                        break;

                    case 0b101011: //SLTU Set on Less Than Unsigned
                        REGISTERS[instr.R.regd] = (unsigned int)REGISTERS[instr.R.regs] < (unsigned int)REGISTERS[instr.R.regt]; // Rd = Rs < Rt
                        wbReg = true; //Enable the write back to register
                        break;

                    case 0b100001: //ADDU Add Unisnged
                        REGISTERS[instr.R.regd] = (int)((unsigned int)REGISTERS[instr.R.regs] + (unsigned int)REGISTERS[instr.R.regt]);
                        wbReg = true;
                        break;
                        
                    case 0b100100: //AND And
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] && REGISTERS[instr.R.regt];
                        wbReg = true;
                        break;

                    case 0b100110: //XOR Exclusive OR
                        REGISTERS[instr.R.regd] = REGISTERS[instr.R.regs] ^ REGISTERS[instr.R.regt];
                        wbReg = true;
                        break;
                        
                    case 0b100111: //NOR Not Or
                        REGISTERS[instr.R.regd] = !(REGISTERS[instr.R.regs] | REGISTERS[instr.R.regt]);
                        wbReg = true;
                        break;

                    case 0b000000: //SLL Shift Left Logical
                        REGISTERS[instr.R.regd] = (REGISTERS[instr.R.regt] << REGISTERS[instr.R.funct]);
                        wbReg = true;
                        break;

                    case 0b000100: //SLLV Shift Left Logical Variable
                        REGISTERS[instr.R.regd] = (REGISTERS[instr.R.regt] << REGISTERS[instr.R.regs]);
                        wbReg = true;
                        break;
                        
                    case 0b000011: //SRA Shift Right Arithmetic 
                        REGISTERS[instr.R.regd] = (REGISTERS[instr.R.regt] >> REGISTERS[instr.R.funct]);
                        wbReg = true;
                        break;

                    case 0b000111: //SRAV Shift Right Arithmetic Variable
                        REGISTERS[instr.R.regd] = (REGISTERS[instr.R.regt] >> REGISTERS[instr.R.regs]);
                        wbReg = true;
                        break;
                    
                    case 0b000010: //SRL Shift Right Logical
                        REGISTERS[instr.R.regd] = (REGISTERS[instr.R.regt] >> REGISTERS[instr.R.funct]);
                        wbReg = true;
                        break;
                
                    case 0b000110: //SRLV Shift Right Logical Variable
                        REGISTERS[instr.R.regd] = (REGISTERS[instr.R.regt] >> REGISTERS[instr.R.regs]);
                        wbReg = true;
                        break;

                    case 0b001001: //JALR Jump and Link Register 
                        REGISTERS[31] = PC; //Sets register[31] = to the value of the Program Counter
                        PC = REGISTERS[instr.R.regs]; // Program counter set to value in Rs
                        break;
                        
                    case 0b001000: //JR Jump Register 
                        PC = REGISTERS[instr.R.regs]; // Program counter set to value in Rs
                        break;
                }
                break;
            case 'I': // I-Type Instruction: Seperated into it's 5 Arithmetic operations 
            /* Nested Switch to use the instruction type as well as the function value of the given code value */
                switch (instr.I.opcode)
                {
                    case 0b001010: //SLTI Set on Less Than
                        REGISTERS[instr.I.regt] = REGISTERS[instr.I.regs] < instr.I.imm; // Rd = Rs < Rt
                        wbReg = true; //Enable the write back to register
                        break;

                    case 0b001011: //SLTIU Set on Less Than Unsigned
                        REGISTERS[instr.I.regt] = (unsigned int)REGISTERS[instr.I.regs] < (unsigned int)instr.I.imm; // Rd = Rs < Rt
                        wbReg = true; //Enable the write back to register
                        break;
                    
                    case 0b001110: //XORI XOR Imm
                        REGISTERS[instr.I.regt] = REGISTERS[instr.I.regs] ^ instr.I.imm;
                        wbReg = true;
                        break;
                        
                    case 0b001101: //ORI Or Immediate
                        REGISTERS[instr.I.regt] = REGISTERS[instr.I.regs] | instr.I.imm;
                        wbReg = true;
                        break;

                    case 0b001100: //ANDI And Immediate
                        REGISTERS[instr.I.regt] = REGISTERS[instr.I.regs] && REGISTERS[instr.I.imm];
                        wbReg = true;
                        break;
                        
                    case 0b101011: //SW Store Word
                        MEMORY[(REGISTERS[instr.I.regs] + instr.I.imm) / 4] = REGISTERS[instr.I.regt]; // [(Rs + immediate)/4] = Rt
                        rwMem = true; //Enable the Read/Write to memory/ram
                        break; 

                    case 0b001111: //LUI Load Upper Immediate
                        REGISTERS[instr.I.regt] = REGISTERS[instr.I.imm] << 16;
                        wbReg = true;

                    case 0b100011: //LW Load Word
                        REGISTERS[instr.I.regt] = MEMORY[(REGISTERS[instr.I.regs] + instr.I.imm) / 4]; // Rt = [(Rs + immediate)/4]
                        wbReg = true; //Enable the write back to register
                        rwMem = true; //Enable the Read/Write to memory/ram
                        break;

                    case 0b001000: //ADDI Add Immediate
                        REGISTERS[instr.I.regt] = REGISTERS[instr.I.regs] + REGISTERS[instr.I.imm]; // Rt= Rs + immediate
                        wbReg = true;
                        break;

                    case 0b001001: //ADDIU Add Immediate Unsigned
                        REGISTERS[instr.I.regt] = (int)((unsigned int)REGISTERS[instr.I.regs] + (unsigned int)REGISTERS[instr.I.imm]); // Rt= Rs + immediate
                        wbReg = true;
                        break;

                    case 0b000100: //BEQ Branch if Equal
                        if (REGISTERS[instr.I.regs] == REGISTERS[instr.I.regt]) { // Checks if Rs is equal to Rt for the brench condition
                            PC += instr.I.imm; // If it is equal, then increment the PC by immediate value
                        }
                        break;

                    case 0b000101: //BNE Branch if Not Equal
                        if (REGISTERS[instr.I.regs] != REGISTERS[instr.I.regt]) { // Checks if Rs is not equal to Rt for the brench condition
                            PC += instr.I.imm; // If it is equal, then increment the PC by immediate value
                        }
                        break;
                    
                    case 0b000111: //BGTZ Branch if greater than zero 
                        if(REGISTERS[instr.I.regs] > 0){ // Checks if Rs is greater than zero
                            PC += instr.I.imm; // If it is greater than zero, then increment the PC by the immediate value
                        }
                        break;
                    
                    case 0b000110: //BLEZ Branch if less than or equal to zero
                        if(REGISTERS[instr.I.regs] <= 0){
                            PC += instr.I.imm; // If it is less than or equal to zero, then increment the OC by the immediate value
                        }
                        break;

                    case 0b000001: //BGEZ Operations
                        switch (instr.I.regt) { // Rt field is used to differentiate these instructions
                            case 0b00001: // BGEZ 
                                if(REGISTERS[instr.I.regs] >= 0){
                                    PC = instr.I.imm;
                                }
                                break;
                            case 0b10001: // BGEZAL Branch On >= 0 And Link
                                if(REGISTERS[instr.I.regs] >= 0){
                                    REGISTERS[31] = PC;
                                    PC += instr.I.imm;
                                }
                                break;
                            case 0b00000: // BLEZ Branch On < 0
                                if(REGISTERS[instr.I.regs] < 0){
                                    PC += instr.I.imm;
                                }
                                break;
                            case 0b10000: // BLEZAL	Branch On < 0 And Link
                                if(REGISTERS[instr.I.regs] < 0){
                                    REGISTERS[31] = PC;
                                    PC += instr.I.imm;
                                }
                                break;
                        }
                }
            
                break;
            case 'J': // J-Type Instruction: Separated into it's 2 Arithmetic operations 
            /* Nested Switch to use the instruction type as well as the opcode value of the given code value */
                switch (instr.J.opcode) {
                    case 0b000010: //J Jump
                        PC += instr.J.imm; // Program counter incremented by the immediate of this J-type
                        break;

                    case 0b000011: //JAL Jump and Link
                        REGISTERS[31] = PC; //Sets register[31] = to the value of the Program Counter
                        PC += instr.J.imm; // Program counter incremented by the immediate of this J-type
                        break;
                    
                }
                break;
        }

        // Since we handle instruction execution and memory-writing together
        // We need to increment the clock if those happened (and log it)        
        if (rwMem) {
            fprintf(fout, "C#%d I%d-MEM\n", clock++, ic);
        }
        if (wbReg) {
            fprintf(fout, "C#%d I%d-WB\n", clock++, ic);
        }

        ic++; //Incrementing instruction count for the sake of the print functions
    }

    file_dumpRegMem(fout);
    fclose(fout);
}

int main(int argc, char const *argv[])
{
    /* set initial values */
    REGISTERS[0] = 0;

    /* Get settings from user */
    char* fni = (char *)malloc(sizeof(char) * 32);
    char* fno = (char *)malloc(sizeof(char) * 32);
    
    // Input
    printf("Input File: ");
    scanf("%s", fni);
    // Output
    printf("Output File: ");
    scanf("%s", fno);

    /* Load the file specified */
    printf("Load Program\n");
    file_load(fni);

    /* Run CPU */
    printf("CPU Run\n");
    cpu(fno); // This will run until code execution completes
    
    printf("Done\n");
    return 0;
}