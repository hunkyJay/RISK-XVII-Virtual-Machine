#include "vm_riskxvii.h"

uint32_t pc;                 // Program counter
uint32_t reg_bank[REG_NUM];  // Register array
unsigned char virtual_routines[VR_END - VR_START + 1];  // Virtual routines space
unsigned char heap_banks[HEAP_BANK_NUM * BANK_BLOCK_SIZE];  // Heap banks space

struct heap_node head;  // The head node of the linked list for heap management

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <memory_image_binary>\n", argv[0]);
        exit(1);
    }

    // Initialze vm and start running
    struct blob vm_memory;
    read_memory_image(argv[1], &vm_memory);
    
    init_heap();

    running_vm(&vm_memory);

    return 0;
}

void read_memory_image(const char* filename, struct blob* vm_memory) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        exit(1);
    }

    // Store instructions into instruct memory
    size_t inst_ret = fread(vm_memory->inst_mem, 1, INST_MEM_SIZE, fp);
    if (!inst_ret) {
        perror("Error reading instruct");
        exit(1);
    }

    // Store data into data memory
    size_t data_ret = fread(vm_memory->data_mem, 1, DATA_MEM_SIZE, fp);
    if (!data_ret) {
        perror("Error reading data");
        exit(1);
    }
    fclose(fp);
}

union instruction fetch_instruct(struct blob* vm_memory) {
    union instruction instruct;
    instruct.raw_instruct = *((uint32_t*)(vm_memory->inst_mem + pc));

    return instruct;
}

void execute_instruct(union instruction instruct, struct blob* vm_memory) {
    enum Opcode opcode = (enum Opcode)(instruct.raw_instruct & 0x7F);  // Use bitmask to get the last 7 bits
    //printf("%08x\n", instruct.raw_instruct);
        switch (opcode) {
            case R_TYPE:
                handle_R_instruct(instruct);
                break;

            case I_TYPE_ONE:
                handle_I1_instruct(instruct);
                break;

            case I_TYPE_TWO:
                handle_I2_instruct(instruct, vm_memory);
                break;

            case I_TYPE_THREE:
                handle_I3_instruct(instruct);
                break;

            case S_TYPE:
                handle_S_instruct(instruct, vm_memory);
                break;

            case SB_TYPE:
                handle_SB_instruct(instruct);
                break;

            case U_TYPE:
                handle_U_instruct(instruct);
                break;

            case UJ_TYPE:
                handle_UJ_instruct(instruct);
                break;

            default:
                instruct_not_implement(instruct);
                break;
        }
    // Guarantee the zero register
    reg_bank[0] = 0;
}

void running_vm(struct blob* vm_memory) {
    // Initialze the registers, program counter, virtual routine space
    for (int i = 0; i < REG_NUM; i++) {
        reg_bank[i] = 0;
    }
    pc = 0;
    for (int j = 0; j <= VR_END - VR_START; j++) {
        virtual_routines[j] = 0;
    }

    while (pc < INST_MEM_SIZE) {
        // Fetch instruction and execute until all finished
        union instruction instruct = fetch_instruct(vm_memory);
        //printf("%08x\n", instruct.raw_instruct);
        execute_instruct(instruct, vm_memory);
    }
}

void increment_pc() {
    pc += INSTRUCT_BYTES;  // Update program counter
}

void handle_R_instruct(union instruction instruct) {
    uint8_t rd = instruct.R_type.rd;
    uint8_t func3 = instruct.R_type.func3;
    uint8_t rs1 = instruct.R_type.rs1;
    uint8_t rs2 = instruct.R_type.rs2;
    uint8_t func7 = instruct.R_type.func7;

    // Check the instruction
    // add: R[rd] = R[rs1] + R[rs2]
    if (func3 == 0b000 && func7 == 0b0000000) {
        reg_bank[rd] = reg_bank[rs1] + reg_bank[rs2];
    }
    // sub: R[rd] = R[rs1] - R[rs2]
    else if (func3 == 0b000 && func7 == 0b0100000) {
        reg_bank[rd] = reg_bank[rs1] - reg_bank[rs2];
    }
    // xor: R[rd] = R[rs1] ˆ R[rs2]
    else if (func3 == 0b100 && func7 == 0b0000000) {
        reg_bank[rd] = reg_bank[rs1] ^ reg_bank[rs2];
    }
    // or: R[rd] = R[rs1] | R[rs2]
    else if (func3 == 0b110 && func7 == 0b0000000) {
        reg_bank[rd] = reg_bank[rs1] | reg_bank[rs2];
    }
    // and: R[rd] = R[rs1] & R[rs2]
    else if (func3 == 0b111 && func7 == 0b0000000) {
        reg_bank[rd] = reg_bank[rs1] & reg_bank[rs2];
    }
    // sll: R[rd] = R[rs1] « R[rs2]
    else if (func3 == 0b001 && func7 == 0b0000000) {
        reg_bank[rd] = reg_bank[rs1] << reg_bank[rs2];
    }
    // srl: R[rd] = R[rs1] » R[rs2]
    else if (func3 == 0b101 && func7 == 0b0000000) {
        reg_bank[rd] = reg_bank[rs1] >> reg_bank[rs2];
    }
    // sra: R[rd] = R[rs1] » R[rs2]
    else if (func3 == 0b101 && func7 == 0b0100000) {
        // Shifting bits should be less than register size (word size 32)
        uint32_t shifting_bits = reg_bank[rs2] % (WORD_BITS);
        // Rotate right shifting
        // Reference:
        // https://stackoverflow.com/questions/28303232/rotate-right-using-bit-operation-in-c
        reg_bank[rd] = (reg_bank[rs1] >> shifting_bits) |
                       (reg_bank[rs1] << (WORD_BITS - shifting_bits));
    }
    // slt: R[rd] = (R[rs1] < R[rs2]) ? 1 : 0
    else if (func3 == 0b010 && func7 == 0b0000000) {
        reg_bank[rd] = ((int32_t)reg_bank[rs1] < (int32_t)reg_bank[rs2]) ? 1 : 0;
    }
    // sltu: R[rd] = (R[rs1] < R[rs2]) ? 1 : 0
    else if (func3 == 0b011 && func7 == 0b0000000) {
        reg_bank[rd] = (reg_bank[rs1] < reg_bank[rs2]) ? 1 : 0;
    } else {
        instruct_not_implement(instruct);
    }

    increment_pc();
}

void handle_I1_instruct(union instruction instruct) {
    uint8_t rd = instruct.I_type.rd;
    uint8_t func3 = instruct.I_type.func3;
    uint8_t rs1 = instruct.I_type.rs1;
    uint32_t imm = instruct.I_type.imm;
    // Check sign bit, 1 or 0
    if (imm & 0x800) {
        imm |= 0xFFFFF000;  // Extend 12 bits to 32
    }


    // Check the instruction
    switch (func3) {
        // addi: R[rd] = R[rs1] + imm
        case 0b000:
            reg_bank[rd] = reg_bank[rs1] + imm;
            break;

        // xori: R[rd] = R[rs1] ˆ imm
        case 0b100:
            reg_bank[rd] = reg_bank[rs1] ^ imm;
            break;

        // ori: R[rd] = R[rs1] | imm
        case 0b110:
            reg_bank[rd] = reg_bank[rs1] | imm;
            break;

        // andi: R[rd] = R[rs1] & imm
        case 0b111:
            reg_bank[rd] = reg_bank[rs1] & imm;
            break;

        // slti: R[rd] = (R[rs1] < imm) ? 1 : 0
        case 0b010:
            reg_bank[rd] = ((int32_t)reg_bank[rs1] < (int32_t)imm) ? 1 : 0;
            break;

        // sltiu: R[rd] = (R[rs1] < imm) ? 1 : 0
        case 0b011:
            reg_bank[rd] = (reg_bank[rs1] < imm) ? 1 : 0;
            break;

        default:
            instruct_not_implement(instruct);
            break;
    }

    increment_pc();
}

void handle_I2_instruct(union instruction instruct, struct blob* vm_memory) {
    uint8_t rd = instruct.I_type.rd;
    uint8_t func3 = instruct.I_type.func3;
    uint8_t rs1 = instruct.I_type.rs1;
    uint32_t imm = instruct.I_type.imm;
    // Check sign bit, 1 or 0
    if (imm & 0x800) {
        imm |= 0xFFFFF000;
    }

    // Check the instruction
    switch (func3) {
        // lb: R[rd] = sext(M[R[rs1] + imm])
        case 0b000:
            // Get the sign extended value
            int8_t value_byte = (int8_t)load_byte((int32_t)reg_bank[rs1] + (int32_t)imm, vm_memory, instruct);
            // Load it into register rd
            reg_bank[rd] = (int32_t)value_byte;
            break;

        // lh: R[rd] = sext(M[R[rs1] + imm])
        case 0b001:
            // Get the sign extended value
            int16_t value_half = (int16_t)load_half_word((int32_t)reg_bank[rs1] + (int32_t)imm, vm_memory, instruct);
            // Load it into register rd
            reg_bank[rd] = (int32_t)value_half;
            break;

        // lw: R[rd] = M[R[rs1] + imm]
        case 0b010:
            reg_bank[rd] = load_word((int32_t)reg_bank[rs1] + (int32_t)imm, vm_memory, instruct);
            break;

        // lbu: R[rd] = M[R[rs1] + imm]
        case 0b100:
            reg_bank[rd] = (uint32_t)load_byte((int32_t)reg_bank[rs1] + (int32_t)imm, vm_memory, instruct);
            break;

        // lhu: R[rd] = M[R[rs1] + imm]
        case 0b101:
            reg_bank[rd] = (uint32_t)load_half_word((int32_t)reg_bank[rs1] + (int32_t)imm, vm_memory, instruct);
            break;

        default:
            instruct_not_implement(instruct);
            break;
    }

    increment_pc();
}

void handle_I3_instruct(union instruction instruct) {
    uint8_t rd = instruct.I_type.rd;
    uint8_t func3 = instruct.I_type.func3;
    uint8_t rs1 = instruct.I_type.rs1;
    uint32_t imm = instruct.I_type.imm;
    // Check sign bit, 1 or 0
    if (imm & 0x800) {
        imm |= 0xFFFFF000;
    }

    // Check the instruction
    // jalr: R[rd] = PC + 4; PC = R[rs1] + imm
    if (func3 == 0b000) {
        reg_bank[rd] = pc + INSTRUCT_BYTES;
        pc = (int32_t)reg_bank[rs1] + (int32_t)imm;
    } else {
        instruct_not_implement(instruct);
    }
}

void handle_S_instruct(union instruction instruct, struct blob* vm_memory) {
    uint8_t func3 = instruct.S_type.func3;
    uint8_t rs1 = instruct.S_type.rs1;
    uint8_t rs2 = instruct.S_type.rs2;
    uint32_t imm4_0 = instruct.S_type.imm4_0;
    uint32_t imm11_5 = instruct.S_type.imm11_5;
    uint32_t imm = (imm11_5 << 5) | imm4_0;
    // Check sign bit, 1 or 0
    if (imm & 0x800) {
        imm |= 0xFFFFF000;
    }

    // Check the instruction
    switch (func3) {
        // sb: M[R[rs1] + imm] = R[rs2]
        case 0b000:
            store_byte((int32_t)reg_bank[rs1] + (int32_t)imm, (uint8_t)reg_bank[rs2], vm_memory, instruct);
            break;

        // sh: M[R[rs1] + imm] = R[rs2]
        case 0b001:
            store_half_word((int32_t)reg_bank[rs1] + (int32_t)imm, (uint16_t)reg_bank[rs2], vm_memory, instruct);
            break;

        // sw: M[R[rs1] + imm] = R[rs2]
        case 0b010:
            store_word((int32_t)reg_bank[rs1] + (int32_t)imm, reg_bank[rs2], vm_memory, instruct);
            break;

        default:
            instruct_not_implement(instruct);
            break;
    }

    increment_pc();
}

void handle_SB_instruct(union instruction instruct) {
    uint32_t imm11 = instruct.SB_type.imm11;
    uint32_t imm4_1 = instruct.SB_type.imm4_1;
    uint8_t func3 = instruct.SB_type.func3;
    uint8_t rs1 = instruct.SB_type.rs1;
    uint8_t rs2 = instruct.SB_type.rs2;
    uint32_t imm10_5 = instruct.SB_type.imm10_5;
    uint32_t imm12 = instruct.SB_type.imm12;
    // Concatinate imm
    uint32_t imm = (imm12 << 11) | (imm11 << 10) | (imm10_5 << 4) | imm4_1;

    // Check the sign bit, o or 1
    if (imm & 0x800) {
        imm |= 0xFFFFF000;
    }
    // Check instruction
    int is_branch = 0;  // The flag to record whether it shoule be a branch
    switch (func3) {
        // beq: if(R[rs1] == R[rs2]) then PC = PC + (imm « 1)
        case 0b000:
            is_branch = (reg_bank[rs1] == reg_bank[rs2]);
            break;
        // bne: if(R[rs1] != R[rs2]) then PC = PC + (imm « 1)
        case 0b001:
            is_branch = (reg_bank[rs1] != reg_bank[rs2]);
            break;
        // blt: if(R[rs1] < R[rs2]) then PC = PC + (imm « 1)
        case 0b100:
            is_branch = ((int32_t)reg_bank[rs1] < (int32_t)reg_bank[rs2]);
            break;
        // bltu: if(R[rs1] < R[rs2]) then PC = PC + (imm « 1)
        case 0b110:
            is_branch = (reg_bank[rs1] < reg_bank[rs2]);
            break;
        // bge: if(R[rs1] >= R[rs2]) then PC = PC + (imm « 1)
        case 0b101:
            is_branch = ((int32_t)reg_bank[rs1] >= (int32_t)reg_bank[rs2]);
            break;
        // bgeu: if(R[rs1] >= R[rs2]) then PC = PC + (imm « 1)
        case 0b111:
            is_branch = (reg_bank[rs1] >= reg_bank[rs2]);
            break;
        default:
            instruct_not_implement(instruct);
            break;
    }

    if (is_branch) {
        pc += (int32_t)(imm << 1);
    } else {
        increment_pc();
    }
}

void handle_U_instruct(union instruction instruct) {
    uint8_t rd = instruct.U_type.rd;
    uint32_t imm31_12 = instruct.U_type.imm31_12;

    // lui: R[rd] = {31:12 = imm | 11:0 = 0
    uint32_t imm = imm31_12 << 12;
    reg_bank[rd] = imm;

    increment_pc();
}

void handle_UJ_instruct(union instruction instruct) {
    uint8_t rd = instruct.UJ_type.rd;
    uint32_t imm19_12 = instruct.UJ_type.imm19_12;
    uint32_t imm11 = instruct.UJ_type.imm11;
    uint32_t imm10_1 = instruct.UJ_type.imm10_1;
    uint32_t imm20 = instruct.UJ_type.imm20;
    // Concatinate imm
    uint32_t imm = (imm20 << 19) | (imm19_12 << 11) | (imm11 << 10) | imm10_1;

    // jal: R[rd] = PC + 4; PC = PC + (imm « 1)
    imm <<= 1;
    // Check sign bit, 1 or 0
    if (imm & 0x80000) {
        imm |= 0xFFF00000;
    }

    reg_bank[rd] = pc + INSTRUCT_BYTES;
    pc += (int32_t)imm;
}

void instruct_not_implement(union instruction instruct) {
    printf("Instruction Not Implemented: 0x%08x\n", instruct.raw_instruct);
    register_dump();
    exit(1);
}

void register_dump() {
    printf("PC = 0x%08x;\n", pc);
    for (int i = 0; i < REG_NUM; i++) {
        printf("R[%d] = 0x%08x;\n", i, reg_bank[i]);
    }
}

int is_valid_address(uint32_t address) {
    // From instruction menory start to virtual routine end addresses, 0 ~ 0x8ff
    if (address <= VIRTUAL_ROUTINE_END) {
        return 1;
    }

    // Check whether it is the allocated address in heap block
    struct heap_node* cursor = &head;
    while (cursor) {
        if ((cursor->allocated_size > 0) && (address >= cursor->address) &&
            (address <(cursor->address + cursor->allocated_size))) {
            return 1;
        }
        cursor = cursor->next;
    }

    return 0;
}

void illegal_operation(union instruction instruct) {
    printf("Illegal Operation: 0x%08x\n", instruct.raw_instruct);
    register_dump();
    exit(1);
}

uint8_t load_byte(uint32_t address, struct blob* vm_memory, union instruction instruct) {
    if (!is_valid_address(address)) {
        illegal_operation(instruct);
    }

    uint8_t b;
    if (address >= DATA_MEM_START && address <= DATA_MEM_END) {
        // Data area
        b = (uint8_t)vm_memory->data_mem[address - DATA_MEM_START];
    } else if (address <= INST_MEM_END) {
        // Instruction area
        b = (uint8_t)vm_memory->inst_mem[address];
    } else if (address >= VR_START && address <= VR_END) {
        // Virtual routines for read type
        b = (uint8_t)console_read_routine(address, vm_memory);
    } else {
        // Heap area
        b = (uint8_t)heap_banks[address - HEAP_START];
    }
    return b;
}

uint16_t load_half_word(uint32_t address, struct blob* vm_memory, union instruction instruct) {
    // Check illegal address for both first byte and second byte
    if (!is_valid_address(address) || !is_valid_address(address+1)) {
        illegal_operation(instruct);
    }

    uint16_t half_word;
    uint16_t first_byte;
    uint16_t second_byte;
    if (address >= DATA_MEM_START && address < DATA_MEM_END) {
        // Data mem
        // Get the two bytes
        first_byte = (uint16_t)vm_memory->data_mem[address - DATA_MEM_START];
        second_byte = (uint16_t)vm_memory->data_mem[address + 1 - DATA_MEM_START];
        // Concatenating two bytes together
        half_word = first_byte | (second_byte << 8);
    } else if (address < INST_MEM_END) {
        // Inst men
        // Get the two bytes
        first_byte = (uint16_t)vm_memory->inst_mem[address];
        second_byte = (uint16_t)vm_memory->inst_mem[address + 1];
        // Concatenating two bytes together
        half_word = first_byte | (second_byte << 8);
    } else if (address >= VR_START && address <= VR_END) {
        // Virtual routines for read type
        half_word = (uint16_t)console_read_routine(address, vm_memory);
    } else {
        // Heap area
        first_byte = (uint16_t)heap_banks[address - HEAP_START];
        second_byte = (uint16_t)heap_banks[address - HEAP_START + 1];
        // Concatenating two bytes together
        half_word = first_byte | (second_byte << 8);
    }
    return half_word;
}

uint32_t load_word(uint32_t address, struct blob* vm_memory, union instruction instruct) {
    // Check invalid address for all four bytes
    if (!is_valid_address(address) || 
        !is_valid_address(address+1) || 
        !is_valid_address(address+2) || 
        !is_valid_address(address+3)) {
        illegal_operation(instruct);
    }

    uint32_t word;
    uint32_t first_byte;
    uint32_t second_byte;
    uint32_t third_byte;
    uint32_t fourth_byte;
    if (address >= DATA_MEM_START && address <= (DATA_MEM_END - 3)) {
        // Data mem
        // Get the four bytes
        first_byte = (uint32_t)vm_memory->data_mem[address - DATA_MEM_START];
        second_byte = (uint32_t)vm_memory->data_mem[address + 1 - DATA_MEM_START];
        third_byte = (uint32_t)vm_memory->data_mem[address + 2 - DATA_MEM_START];
        fourth_byte = (uint32_t)vm_memory->data_mem[address + 3 - DATA_MEM_START];
        // Concatenating four bytes together
        word = first_byte | (second_byte << 8) | (third_byte << 16) | (fourth_byte << 24);
    } else if (address <= (INST_MEM_END - 3)) {
        // Inst mem
        // Get the four bytes
        first_byte = (uint32_t)vm_memory->inst_mem[address];
        second_byte = (uint32_t)vm_memory->inst_mem[address + 1];
        third_byte = (uint32_t)vm_memory->inst_mem[address + 2];
        fourth_byte = (uint32_t)vm_memory->inst_mem[address + 3];
        // Concatenating four bytes together
        word = first_byte | (second_byte << 8) | (third_byte << 16) | (fourth_byte << 24);
    } else if (address >= VR_START && address <= VR_END) {
        // Virtual routines for read type
        word = console_read_routine(address, vm_memory);
    } else {
        // Heap
        first_byte = (uint32_t)heap_banks[address - HEAP_START];
        second_byte = (uint32_t)heap_banks[address + 1 - HEAP_START];
        third_byte = (uint32_t)heap_banks[address + 2 - HEAP_START];
        fourth_byte = (uint32_t)heap_banks[address + 3 - HEAP_START];
        // Concatenating four bytes together
        word = first_byte | (second_byte << 8) | (third_byte << 16) | (fourth_byte << 24);
    }
    return word;
}

void store_byte(uint32_t address, uint8_t value, struct blob* vm_memory, union instruction instruct) {
    // Check invalid address
    if (!is_valid_address(address)) {
        illegal_operation(instruct);
    }

    if (address >= DATA_MEM_START && address <= DATA_MEM_END) {
        // Data mem
        vm_memory->data_mem[address - DATA_MEM_START] = value;
    } else if (address <= INST_MEM_END) {
        // Inst mem, read only
        illegal_operation(instruct);
    } else if (address >= VR_START && address <= VR_END) {
        // Virtual routines write type
        int is_routine = console_write_routine(address, (uint32_t)value, vm_memory, instruct);
        // Not routine
        if (!is_routine) {
            illegal_operation(instruct);
        }
    } else {
        // Heap area
        heap_banks[address - HEAP_START] = value;
    }
}

void store_half_word(uint32_t address, uint16_t value, struct blob* vm_memory, union instruction instruct) {
    // Check invalid address for both first byte and second byte
    if (!is_valid_address(address) || !is_valid_address(address+1)) {
        illegal_operation(instruct);
    }

    if (address >= DATA_MEM_START && address < DATA_MEM_END) {
        // Store the lower 8 bits
        vm_memory->data_mem[address - DATA_MEM_START] = (uint8_t)(value & 0xFF);
        // Store the higher 8 bits
        vm_memory->data_mem[address + 1 - DATA_MEM_START] = (uint8_t)((value >> 8) & 0xFF);
    } else if (address <= INST_MEM_END) {
        // Inst mem, read only
        illegal_operation(instruct);
    } else if (address >= VR_START && address <= VR_END) {
        // Virtual routines write type
        int is_routine = console_write_routine(address, (uint32_t)value, vm_memory, instruct);
        // Not routine
        if (!is_routine) {
            illegal_operation(instruct);
        }
    } else {
        // Heap area
        heap_banks[address - HEAP_START] = (uint8_t)(value & 0xFF);
        heap_banks[address + 1 - HEAP_START] = (uint8_t)((value >> 8) & 0xFF);
    }
}

void store_word(uint32_t address, uint32_t value, struct blob* vm_memory, union instruction instruct) {
    // Check invalid address for all four bytes
    if (!is_valid_address(address) ||
        !is_valid_address(address+1) ||
        !is_valid_address(address+2) ||
        !is_valid_address(address+3)) {
        illegal_operation(instruct);
    }

    if (address >= DATA_MEM_START && address <= DATA_MEM_END - 3) {
        // Store the 4 bytes respectively
        vm_memory->data_mem[address - DATA_MEM_START] = (uint8_t)(value & 0xFF);
        vm_memory->data_mem[address + 1 - DATA_MEM_START] = (uint8_t)((value >> 8) & 0xFF);
        vm_memory->data_mem[address + 2 - DATA_MEM_START] = (uint8_t)((value >> 16) & 0xFF);
        vm_memory->data_mem[address + 3 - DATA_MEM_START] = (uint8_t)((value >> 24) & 0xFF);
    } else if (address <= INST_MEM_END) {
        // Inst mem, read only
        illegal_operation(instruct);
    } else if (address >= VR_START && address <= VR_END) {
        // Virtual routines write type
        int is_routine = console_write_routine(address, value, vm_memory, instruct);
        // Not routine
        if (!is_routine) {
            illegal_operation(instruct);
        }
    } else {
        // Heap area
        heap_banks[address - HEAP_START] = (uint8_t)(value & 0xFF);
        heap_banks[address + 1 - HEAP_START] = (uint8_t)((value >> 8) & 0xFF);
        heap_banks[address + 2 - HEAP_START] = (uint8_t)((value >> 16) & 0xFF);
        heap_banks[address + 3 - HEAP_START] = (uint8_t)((value >> 24) & 0xFF);
    }
}

uint32_t console_read_routine(uint32_t address, struct blob* vm_memory) {
    switch (address) {
        // 0x0812 - Console Read Character
        case VR_READ_CHAR:
            uint32_t ch = (uint32_t)getchar();
            return ch;
            break;
        // 0x0816 - Console Read Signed Integer
        case VR_READ_SINT:
            int32_t sint;
            int scan_ret = scanf("%d", &sint);
            if (scan_ret != 1) {
                perror("Error scanf");
                exit(1);
            }
            return (uint32_t)sint;
            break;

        default:
            // Just read data from the address
            // Get the four bytes
            uint32_t first_byte = (uint32_t)virtual_routines[address - VR_START];
            uint32_t second_byte = (uint32_t)virtual_routines[address - VR_START + 1];
            uint32_t third_byte = (uint32_t)virtual_routines[address - VR_START + 2];
            uint32_t fourth_byte = (uint32_t)virtual_routines[address - VR_START + 3];
            // Concatenating four bytes together
            uint32_t data = first_byte | (second_byte << 8) | (third_byte << 16) | (fourth_byte << 24);
            return data;
            break;
    }
}

int console_write_routine(uint32_t address, uint32_t value, struct blob* vm_memory, union instruction instruct) {
    switch (address) {
        // 0x0800 - Console Write Character
        case VR_WRITE_CHAR:
            putchar((uint8_t)value);
            break;
        // 0x0804 - Console Write Signed Integer
        case VR_WRITE_SINT:
            printf("%d", (int32_t)value);
            break;
        // 0x0808 - Console Write Unsigned Integer
        case VR_WRITE_UINT:
            printf("%x", (uint32_t)value);
            break;
        // 0x080C - Halt
        case VR_HALT:
            printf("CPU Halt Requested\n");
            exit(0);
            break;
        // 0x0820 - Dump PC
        case VR_DUMP_PC:
            printf("%x", pc);
            break;
        // 0x0824 - Dump Register Banks
        case VR_DUMP_REG:
            register_dump();
            break;
        // 0x0828 - Dump Memory Word
        case VR_DUMP_WORD:
            uint32_t word = load_word((uint32_t)value, vm_memory, instruct);
            printf("%x", word);
            break;
        // 0x0830 - Malloc
        case VR_MALLOC:
            // Set R[28]
            reg_bank[28] = vm_malloc(value);
            break;
        // 0x0834 - Free
        case VR_FREE:
            int is_free = vm_free(value);
            if (!is_free) {
                illegal_operation(instruct);
            }
            break;
        default:
            return 0;  // No such routine
            break;
    }

    return 1;
}

void init_heap() {
    for(int i = 0; i < HEAP_BANK_NUM; i++) {
        heap_banks[i] = 0;
    }
    head.address = HEAP_START;
    head.bank_blocks = HEAP_BANK_NUM;
    head.allocated_size = 0;
    head.next = NULL;
}

uint32_t vm_malloc(uint32_t size) {
    // Calculate the required consecutive blocks to meet the size
    uint32_t required_blocks = (size + BANK_BLOCK_SIZE - 1) / BANK_BLOCK_SIZE;
    if (required_blocks == 0) {
        return 0;  // 0 blocks to allocate, edge case for malloc 0
    }
    struct heap_node* cursor = &head;  // Record the current node

    while (cursor) {
        // Check whether the avilable blocks of current node are sufficient
        if (cursor->allocated_size==0 && cursor->bank_blocks>=required_blocks) {
            // Starting allocation
            uint32_t allocated_address = cursor->address;

            // Check redundant blocks for the next new node
            if (cursor->bank_blocks > required_blocks) {
                struct heap_node* new_node = (struct heap_node*)malloc(sizeof(struct heap_node));
                new_node->address = cursor->address + required_blocks * BANK_BLOCK_SIZE;
                new_node->bank_blocks = cursor->bank_blocks - required_blocks;
                new_node->allocated_size = 0;
                // Add the new node to list
                new_node->next = cursor->next;
                cursor->next = new_node;
            }
            // Update the current node
            cursor->bank_blocks = required_blocks;
            cursor->allocated_size = size;

            return allocated_address;
        } else {
            // Keep iterating
            cursor = cursor->next;
        }
    }

    // No blocks to allocate
    return 0;
}

int vm_free(uint32_t address) {
    struct heap_node* cursor = &head;    // The current node
    struct heap_node* prev_node = NULL;  // The node before current node

    while (cursor) {
        // Check whether the address input is exactly an allocated address
        if (cursor->allocated_size > 0 && address == cursor->address) {
            cursor->allocated_size = 0;  // Starting free

            // Concatenate the later consecutive redundant blocks
            if (cursor->next && cursor->next->allocated_size == 0) {
                struct heap_node* next_node = cursor->next;
                cursor->bank_blocks += next_node->bank_blocks;
                cursor->next = next_node->next;
                free(next_node);
            }

            // Concatenate the former consecutive redundant blocks
            if (prev_node && prev_node->allocated_size == 0) {
                prev_node->bank_blocks += cursor->bank_blocks;
                prev_node->next = cursor->next;
                free(cursor);
            }
            return 1;  // Successfully freed
        } else {
            // Keep iterating
            prev_node = cursor;
            cursor = cursor->next;
        }
    }

    return 0; // Invalid free
}