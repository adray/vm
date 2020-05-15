#pragma once

enum vm_register
{
    VM_REGISTER_EAX = 0x0,
    VM_REGISTER_ECX = 0x1,
    VM_REGISTER_EDX = 0x2,
    VM_REGISTER_EBX = 0x3,
    VM_REGISTER_ESP = 0x4,
    VM_REGISTER_EBP = 0x5,
    VM_REGISTER_R0 = 0x8,
    VM_REGISTER_R1 = 0x9,
    VM_REGISTER_R2 = 0x10,
    VM_REGISTER_R3 = 0x11,
    VM_REGISTER_R4 = 0x12,
    VM_REGISTER_R5 = 0x13,
    VM_REGISTER_R6 = 0x14,
    VM_REGISTER_R7 = 0x15
};

enum vm_instruction_type
{
    VM_INSTRUCTION_NONE = 0x0,
    VM_INSTRUCTION_UNARY = 0x1,
    VM_INSTRUCTION_BINARY = 0x2
};

enum vm_instruction_code
{
    VM_INSTRUCTION_CODE_NONE = 0x0,
    VM_INSTRUCTION_CODE_DST_REGISTER = 0x1,
    VM_INSTRUCTION_CODE_DST_MEMORY = 0x2,
    VM_INSTRUCTION_CODE_IMMEDIATE = 0x4,
    VM_INSTRUCTION_CODE_SRC_REGISTER = 0x8,
    VM_INSTRUCTION_CODE_SRC_MEMORY = 0x10,
    VM_INSTRUCTION_CODE_OFFSET = 0x20
};

#define CODE_NONE (VM_INSTRUCTION_CODE_NONE)
#define CODE_UR (VM_INSTRUCTION_CODE_DST_REGISTER)
#define CODE_UM (VM_INSTRUCTION_CODE_DST_MEMORY)
#define CODE_UMO (VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_OFFSET)
#define CODE_UI (VM_INSTRUCTION_CODE_IMMEDIATE)
#define CODE_BRR (VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_REGISTER)
#define CODE_BRM (VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_MEMORY)
#define CODE_BMR (VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER)
#define CODE_BMRO (VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER | VM_INSTRUCTION_CODE_OFFSET)
#define CODE_BRMO (VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_OFFSET)
#define CODE_BRI (VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_IMMEDIATE)

struct vm_instruction
{
    unsigned char rex;
    unsigned char ins;
    unsigned char subins;
    int type;
    int code;
};

#define INS(rex, ins, subins, type, code) \
    { (unsigned char)rex, (unsigned char)ins, (unsigned char)subins, type, code }

struct vm_instruction_table
{
    vm_instruction cpuid =
        INS(0xF, 0xA2, 0x0, VM_INSTRUCTION_NONE, CODE_NONE); // CPUID
    vm_instruction cdq[1] = {
        INS(0x0, 0x99, 0x0, VM_INSTRUCTION_NONE, CODE_NONE), // CDQ
    };
    vm_instruction ret =
        INS(0x0, 0xC3, 0x0, VM_INSTRUCTION_NONE, CODE_NONE); // RETURN
    vm_instruction push[1] = {
        INS(0x0, 0x50, 0x0, VM_INSTRUCTION_UNARY, CODE_UR), // PUSH
    };
    vm_instruction pop[1] = {
        INS(0x0, 0x58, 0x0, VM_INSTRUCTION_UNARY, CODE_UR), // POP
    };
    vm_instruction mov[6] = {
        INS(0x0, 0x89, 0x0, VM_INSTRUCTION_BINARY, CODE_BMR), // MOV
        INS(0x0, 0x8B, 0x0, VM_INSTRUCTION_BINARY, CODE_BRM), // MOV
        INS(0x0, 0xB8, 0x0, VM_INSTRUCTION_BINARY, CODE_BRI), // MOV
        INS(0x0, 0x89, 0xC0, VM_INSTRUCTION_BINARY, CODE_BRR), // MOV
        INS(0x0, 0x89, 0x0, VM_INSTRUCTION_BINARY, CODE_BMRO), // MOV
        INS(0x0, 0x8b, 0x0, VM_INSTRUCTION_BINARY, CODE_BRMO), // MOV
    };
    vm_instruction add[3] = {
        INS(0x0, 0x1, 0xC0, VM_INSTRUCTION_BINARY, CODE_BRR), // ADD
        INS(0x0, 0x1, 0x0, VM_INSTRUCTION_BINARY, CODE_BMRO), // ADD
        INS(0x0, 0x3, 0x0, VM_INSTRUCTION_BINARY, CODE_BRMO), // ADD
    };
    vm_instruction sub[1] = {
        INS(0x0, 0x29, 0xC0, VM_INSTRUCTION_BINARY, CODE_BRR), // SUB
    };
    vm_instruction inc[2] = {
        INS(0x40, 0xFF, 0xC0, VM_INSTRUCTION_UNARY, CODE_UR), // INC
        INS(0x40, 0xFF, 0x0, VM_INSTRUCTION_UNARY, CODE_UM), // INC
    };
    vm_instruction dec[2] = {
        INS(0x40, 0xFF, 0xC8, VM_INSTRUCTION_UNARY, CODE_UR), // DEC
        INS(0x40, 0xFF, 0x1, VM_INSTRUCTION_UNARY, CODE_UM), // DEC
    };
    vm_instruction nop =
        INS(0x00, 0x90, 0x0, VM_INSTRUCTION_NONE, CODE_NONE); // NOP
    vm_instruction cmp[3] = {
        INS(0x00, 0x39, 0xC0, VM_INSTRUCTION_BINARY, CODE_BRR), // CMP
        INS(0x00, 0x39, 0x0, VM_INSTRUCTION_BINARY, CODE_BRMO), // CMP
        INS(0x00, 0x39, 0x0, VM_INSTRUCTION_BINARY, CODE_BMRO) // CMP
    };
    vm_instruction lae =
        INS(0x48, 0x8D, 0x0, VM_INSTRUCTION_BINARY, CODE_BRMO); // LEA
    vm_instruction idiv =
        INS(0xf7, 0xf8, 0x0, VM_INSTRUCTION_UNARY, CODE_UR); // IDIV
};

bool vm_find_ins(const vm_instruction* subtable, int size, vm_instruction_type type, int /*vm_instruction_code*/ code, vm_instruction* ins);

void vm_emit(const vm_instruction& ins, unsigned char* program, int& count, char dst, char src, int offset, int imm);
