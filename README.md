# RISK-XVII-Virtual-Machine
## Introduction
A virtual machine for the RISK-XVII architecture implemented in C. It is capable of interpreting and executing binary programs written for RISK-XVII. It is a useful tool for testing, debugging, and executing RISK-XVII assembly code without needing actual hardware, aids in understanding how a CPU interprets and executes instructions, how different registers function, and how memory and CPU interact.

## RISK-XVII Architecture
RISK-XVII is a simplified version of RISC (Reduced Instruction Set Computer) architecture. It includes:

- 32-bit instructions and data.
- 32 general-purpose registers (x0-x31), with the x0 register hardwired to 0.
- A 32-bit program counter (PC) that points to the next instruction to be executed.
- An instruction memory of 1024 bytes (256 instructions) and a data memory of 1024 bytes.
- Instructions for arithmetic and logical operations, memory access (load and store), and control flow.

**The memory image binary file used to load the program and initial state of data memory is structured as follows:**

- The first 1024 bytes are the instruction memory, in which each 4-byte group represents a single RISK-XVII instruction in little-endian format.
- The next 1024 bytes are the data memory, initialized to the desired initial state.

This file always measures 2048 bytes in total, even if the actual instruction and data memory required is less than 1024 bytes each. Unused memory is filled with zeroes.



## Instruction Set Overview
The RISK-XVII architecture uses a reduced set of the RISC-V instruction set, with a subset of instructions for arithmetic, logical operations, control flow, and I/O.

#### Arithmetic Instructions
Arithmetic instructions include add, sub, addi, and subi, which perform addition and subtraction on registers or an immediate value and a register.

#### Logical Instructions
Logical instructions include and, or, xor, andi, ori, and xori, which perform bitwise operations on registers or an immediate value and a register.

#### Control Flow Instructions
Control flow instructions include beq, bne, blt, bge, bltu, and bgeu, which branch to a different instruction based on a comparison between two registers.

#### I/O Instructions
I/O is performed using memory-mapped I/O. The address 0x0800 is mapped to the console output, and writing a byte to this address will output it to the console. Similarly, the address 0x0804 is mapped to the console input, and reading from this address will read a byte from the console.

## Virtual Routines
The RISK-XVII virtual machine has several virtual routines for interfacing with system hardware. The following memory addresses are reserved for these routines:

#### Console Write Character (0x0800)
Writing a byte to address 0x0800 will output that byte as a character to the console. For example, writing 0x41 (ASCII 'A') will print 'A' to the console.

#### Console Write Signed Integer (0x0804)
Writing a 32-bit value to address 0x0804 will output that value as a signed integer to the console. For example, writing 0x0000002A will print '42' to the console.

#### Console Read Character (0x0808)
Reading a byte from address 0x0808 will read a character from the console as a byte. The result will be in ASCII format.

#### Console Read Signed Integer (0x080C)
Reading a 32-bit value from address 0x080C will read a signed integer from the console. The input should be a string of decimal digits possibly preceded by a '-' sign.

Note that these are blocking routines: the virtual machine will halt until the operation is complete. For example, if a read operation is performed but no input is available, the machine will wait until input is provided.

## How to Run
Use make to build and compile the program
```
$ make
```

Execute the virtual machine
```
$ ./vm_riskxvii <path_to_memory_image_binary>
```
E.g.
```
$ ./vm_riskxvii examples/hello_world/hello_world.mi
```

Compile and run the tests
```
$ make tests
$ make run_tests
```

Clean the compiled binaries and objects
```
$ make clean
```

## Project Structure
- vm-riskxvii/: This directory contains the source code for the virtual machine.
- ./examples/: This directory contains example RISK-XVII assembly programs and their corresponding .mi files
- ./tests/: This directory contains the testing .mi files

## Contributing
### Current contributors
- Renjie He

Contributions to the project are welcome. Please fork this repository, make your changes in a separate branch, and open a pull request.
