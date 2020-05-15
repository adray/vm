#include "emit.h"


bool vm_find_ins(const vm_instruction* subtable, int size, vm_instruction_type type, int /*vm_instruction_code*/ code, vm_instruction* ins)
{
    int elements = size / sizeof(vm_instruction);

    bool success = false;
    for (int i = 0; i < elements; i++)
    {
        if (subtable[i].code == code &&
            subtable[i].type == type)
        {
            *ins = subtable[i];
            success = true;
            break;
        }
    }

    return success;
}

void vm_emit(const vm_instruction& ins, unsigned char* program, int &count)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;
}

void vm_emit_ur(const vm_instruction& ins, unsigned char* program, int &count, char reg)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    if (ins.subins > 0)
    {
        program[count++] = ins.ins;
        program[count++] = ins.subins | (reg & 0x7);
    }
    else
    {
        program[count++] = ins.ins | (reg & 0x7);
    }
}

void vm_emit_um(const vm_instruction& ins, unsigned char* program, int &count, char reg)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;
    
    if (reg == VM_REGISTER_ESP)
    {
        program[count++] = ((ins.subins & 0x7) << 3) | 0x4 | (0x0 << 6);
        program[count++] = 0x24;
    }
    else
    {
        program[count++] = ((ins.subins & 0x7) << 3) | (0x0 << 6) | (reg & 0x7);
    }
}

void vm_emit_umo(const vm_instruction& ins, unsigned char* program, int &count, char reg, int offset)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (reg == VM_REGISTER_ESP)
    {
        program[count++] = ((0x0 & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24;
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((0x0 & 0x7) << 3) | (0x2 << 6) | (reg & 0x7);
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
}

void vm_emit_bri(const vm_instruction& ins, unsigned char* program, int &count, char reg, int imm)
{
    program[count++] = ins.ins | reg;
    program[count++] = (unsigned char)(imm & 0xff);
    program[count++] = (unsigned char)((imm >> 8) & 0xff);
    program[count++] = (unsigned char)((imm >> 16) & 0xff);
    program[count++] = (unsigned char)((imm >> 24) & 0xff);
}

void vm_emit_brr(const vm_instruction& ins, unsigned char* program, int &count, char dst, char src)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    program[count++] = ins.subins | ((src & 0x7) << 0x3) | ((dst & 0x7) << 0);
}

void vm_emit_brm(const vm_instruction& ins, unsigned char* program, int &count, char dst, char src)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (src == VM_REGISTER_ESP)
    {
        program[count++] = ((dst & 0x7) << 3) | 0x4 | (0x0 << 6);
        program[count++] = 0x24;
    }
    else
    {
        program[count++] = ((dst & 0x7) << 3) | (0x0 << 6) | (src & 0x7);
    }
}

void vm_emit_bmr(const vm_instruction& ins, unsigned char* program, int &count, char dst, char src)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (dst == VM_REGISTER_ESP)
    {
        program[count++] = ((src & 0x7) << 3) | 0x4 | (0x0 << 6);
        program[count++] = 0x24;
    }
    else
    {
        program[count++] = ((src & 0x7) << 3) | (0x0 << 6) | (dst & 0x7);
    }
}

void vm_emit_brmo(const vm_instruction& ins, unsigned char* program, int &count, char dst, char src, int offset)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (src == VM_REGISTER_ESP)
    {
        program[count++] = ((dst & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24;
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((dst & 0x7) << 3) | (0x2 << 6) | (src & 0x7);
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
}

void vm_emit_bmro(const vm_instruction& ins, unsigned char* program, int &count, char dst, char src, int offset)
{
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (dst == VM_REGISTER_ESP)
    {
        program[count++] = ((src & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24;
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((src & 0x7) << 3) | (0x2 << 6) | (dst & 0x7);
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
}

void vm_emit(const vm_instruction& ins, unsigned char* program, int& count, char dst, char src, int offset, int imm)
{
    switch (ins.type)
    {
    case VM_INSTRUCTION_NONE:
        vm_emit(ins, program, count);
        break;
    case VM_INSTRUCTION_UNARY:
        if (ins.code == CODE_UR)
        {
            vm_emit_ur(ins, program, count, dst);
        }
        else if (ins.code == CODE_UM)
        {
            vm_emit_um(ins, program, count, dst);
        }
        else if (ins.code == CODE_UMO)
        {
            vm_emit_umo(ins, program, count, dst, offset);
        }
        break;
    case VM_INSTRUCTION_BINARY:
        if (ins.code == CODE_BRR)
        {
            vm_emit_brr(ins, program, count, dst, src);
        }
        else if (ins.code == CODE_BMR)
        {
            vm_emit_bmr(ins, program, count, dst, src);
        }
        else if (ins.code == CODE_BRM)
        {
            vm_emit_brm(ins, program, count, dst, src);
        }
        else if (ins.code == CODE_BMRO)
        {
            vm_emit_bmro(ins, program, count, dst, src, offset);
        }
        else if (ins.code == CODE_BRMO)
        {
            vm_emit_brmo(ins, program, count, dst, src, offset);
        }
        else if (ins.code == CODE_BRI)
        {
            vm_emit_bri(ins, program, count, dst, imm);
        }
        break;
    }
}
