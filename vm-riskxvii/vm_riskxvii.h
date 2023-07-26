#ifndef VM_RISKXVII_H
#define VM_RISKXVII_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INSTRUCT_BYTES 4
#define INST_MEM_SIZE 1024
#define DATA_MEM_SIZE 1024
#define INST_MEM_END 0x3ff
#define DATA_MEM_START 0x0400
#define DATA_MEM_END 0x7ff
#define VR_START 0x0800
#define VR_END 0x8ff
#define HEAP_START 0xb700
#define HEAP_END 0xd700
#define REG_NUM 32
#define WORD_BITS 32
#define VR_WRITE_CHAR 0x0800
#define VR_WRITE_SINT 0x0804
#define VR_WRITE_UINT 0x0808
#define VR_HALT 0x080C
#define VR_READ_CHAR 0x0812
#define VR_READ_SINT 0x0816
#define VR_DUMP_PC 0x0820
#define VR_DUMP_REG 0x0824
#define VR_DUMP_WORD 0x0828
#define VR_MALLOC 0x0830
#define VR_FREE 0x0834
#define VIRTUAL_ROUTINE_END 0x8ff
#define HEAP_BANK_NUM 128
#define BANK_BLOCK_SIZE 64


enum Opcode {
    R_TYPE = 0b0110011,
    // There are three kinds of opcodes for the I_type instruction with different purposes
    I_TYPE_ONE = 0b0010011,
    I_TYPE_TWO = 0b0000011,
    I_TYPE_THREE = 0b1100111,
    S_TYPE = 0b0100011,
    SB_TYPE = 0b1100011,
    U_TYPE = 0b0110111,
    UJ_TYPE = 0b1101111
};  // The opcode for different instructions

struct blob {
    unsigned char inst_mem[INST_MEM_SIZE];
    unsigned char data_mem[DATA_MEM_SIZE];
};  // The vm instruction memory and data memory

union instruction {
    uint32_t raw_instruct;

    struct {
        unsigned opcode : 7;
        unsigned rd : 5;
        unsigned func3 : 3;
        unsigned rs1 : 5;
        unsigned rs2 : 5;
        unsigned func7 : 7;
    } R_type;

    struct {
        unsigned opcode : 7;
        unsigned rd : 5;
        unsigned func3 : 3;
        unsigned rs1 : 5;
        unsigned imm : 12;
    } I_type;

    struct {
        unsigned opcode : 7;
        unsigned imm4_0 : 5;
        unsigned func3 : 3;
        unsigned rs1 : 5;
        unsigned rs2 : 5;
        unsigned imm11_5 : 7;
    } S_type;

    struct {
        unsigned opcode : 7;
        unsigned imm11 : 1;
        unsigned imm4_1 : 4;
        unsigned func3 : 3;
        unsigned rs1 : 5;
        unsigned rs2 : 5;
        unsigned imm10_5 : 6;
        unsigned imm12 : 1;
    } SB_type;

    struct {
        unsigned opcode : 7;
        unsigned rd : 5;
        unsigned imm31_12 : 20;
    } U_type;

    struct {
        unsigned opcode : 7;
        unsigned rd : 5;
        unsigned imm19_12 : 8;
        unsigned imm11 : 1;
        unsigned imm10_1 : 10;
        unsigned imm20 : 1;
    } UJ_type;
};  // Instructions have fixed size 32 bits

struct heap_node {
    uint32_t address;
    uint32_t bank_blocks;
    uint32_t allocated_size;
    struct heap_node *next;
}; // The liked list node to record the allocated information about specific heap address

/**
 * Load instruction and data memory to vm by reading the memory image file
 * @param filename The image file to read
 * @param vm_memory The vm blob including instruction and data memory
*/
void read_memory_image(const char* filename, struct blob* vm_memory);

/**
 * Fetch the next instruction from the vm memory
 * @parm vm_memory The vm memory blob
 * @return union instruct The next instruction
 */
union instruction fetch_instruct(struct blob* vm_memory);

/**
 * Execute the instruction
 * @param instruct The instruction to execute
 * @param vm_memory The vm memory blob
*/
void execute_instruct(union instruction instruct, struct blob* vm_memory);

/**
 * Start running the virtual machine
 * @param vm_memory The vm memory blob
*/
void running_vm(struct blob* vm_memory);

/**
 * Increment the PC after executing the instruction
*/
void increment_pc();

/**
 * Handle the R type instruction, including add, sub, xor, or, and, slt, srt, sra, slt, and sltu
 * @param instruct The instruction
*/
void handle_R_instruct(union instruction instruct);

/**
 * Handle the I type 1 instruction, including addi, xori, ori, andi, slti, and sltiu
 * @param instruct The instruction
*/
void handle_I1_instruct(union instruction instruct);

/**
 * Handle the I type 2 instruction, including lb, lh, lw, lbu, and lhu
 * @param instruct The instruction
*/
void handle_I2_instruct(union instruction instruct, struct blob* vm_memory);

/**
 * Handle the I type 3 instruction, jalr
 * @param instruct The instruction
 * 
*/
void handle_I3_instruct(union instruction instruct);

/**
 * Handle the S type instruction, including sb, sh, and sw
 * @param instruct The instruction
 * @param vm_memory The vm memory blob
*/
void handle_S_instruct(union instruction instruct, struct blob* vm_memory);

/**
 * Handle the SB type instruction, including beq, bne, blt, bltu, bge, and bgeu
 * @param instruct The instruction
*/
void handle_SB_instruct(union instruction instruct);

/**
 * Handle the U type instruction, lui
 * @param instruct The instruction
*/
void handle_U_instruct(union instruction instruct);

/**
 * Handle UJ type instruction, jal
 * @param instruct The instruction
*/
void handle_UJ_instruct(union instruction instruct);

/**
 * Print the instruction not implemented information
 * @param instruct The instruction
*/
void instruct_not_implement(union instruction instruct);

/**
 * Perform register dump and print the information
*/
void register_dump();

/**
 * Chech whether the address is within the vm scope
 * @param address The address to check
 * @return int, 1 valid, 0 invalid
*/
int is_valid_address(uint32_t address);

/**
 * Print the illegal operation information
 * @param instruct The current instruction to print
*/
void illegal_operation(union instruction instruct);

/**
 * Load a byte from specific address in vm
 * @param address The address of the byte to load
 * @param vm_memory The vm memory blob
 * @param instruct The current instruction
 * @return uin8_t The loaded byte
*/
uint8_t load_byte(uint32_t address, struct blob* vm_memory, union instruction instruct);

/**
 * Load a half word from specific address in vm
 * @param address The address of the half word to load
 * @param vm_memory The vm memory blob
 * @param instruct The current instruction
 * @return uin16_t The loaded half word
*/
uint16_t load_half_word(uint32_t address, struct blob* vm_memory, union instruction instruct);

/**
 * Load a word from specfic address in vm
 * @param address The address of the word
 * @param vm_memory The vm memory blob
 * @param instruct The current instruction
 * @return uin32_t The loaded word
*/
uint32_t load_word(uint32_t address, struct blob* vm_memory, union instruction instruct);

/**
 * Store a byte to specific address in vm
 * @param address The address to store byte
 * @param value, The value of the byte to store
 * @param vm_memory The vm memory blob
 * @param instruct The current instruction
*/
void store_byte(uint32_t address, uint8_t value, struct blob* vm_memory, union instruction instruct);

/**
 * Store a half word to specific address in vm
 * @param address The address to store the half word
 * @param value, The value of the half word
 * @param vm_memory The vm memory blob
 * @param instruct The current instruction
*/
void store_half_word(uint32_t address, uint16_t value, struct blob* vm_memory, union instruction instruct);

/**
 * Store a word to specified address in vm
 * @param address, The address to store the word
 * @param value, The value of the word
 * @param vm_memory The vm memory blob
 * @param instruct The current instruction
*/
void store_word(uint32_t address, uint32_t value, struct blob* vm_memory, union instruction instruct);

/**
 * Perform read related virtual routine
 * @param address The address related to virtual routine
 * @param vm_memory The vm memory blob
 * @retun unin32_t The reading result of the virtual routine
*/
uint32_t console_read_routine(uint32_t address, struct blob* vm_memory);

/**
 * Perform write related virtual routine
 * @param address The address related to virtual routine
 * @param vm_memory The vm memory blob
 * @retun unin32_t The writing result of the virtual routine
*/
int console_write_routine(uint32_t address, uint32_t value, struct blob* vm_memory, union instruction instruct);

/**
 * Initializes the heap management linked list with all 128 banks unallocated
*/
void init_heap();

/**
 * Malloc a chunk of memory on the heap banks with the specified size
 * @param size The size of the memory
 * @return The allocated memory address if successful, otherwise 0;
*/
uint32_t vm_malloc(uint32_t size);

/**
 * Free a chunk of memory on the heap starting at the value being stored
 * @param address The address on heap to free
 * @return the free result 1 if successful, otherwise 0
*/
int vm_free(uint32_t address);

#endif