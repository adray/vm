#include "vm.h"
#include "emit.h"
#include <Windows.h>
#include <iostream>

#define VM_ALIGN_16(x) ((x + 0xf) & ~(0xf))

struct vm_stack_local
{
    int reg;
    int offset;
    bool reference;
    vm_structure type;
};

struct vm_label_target
{
    int labelId;
    bool initialized;
    int position;
};

struct vm_jump
{
    vm_code code;
    int labelId;
    int position;
};

struct vm_method_local
{
    int position;
    vm_scope* scope;
};

void vm_cpuid(const vm_instruction_table& table, unsigned char* program, int& count)
{
    vm_emit(table.cpuid, program, count, 0, 0, 0, 0);
}

void vm_reserve(unsigned char* program, int& count)
{
    program[count++] = 0x90; // nop
}

void vm_reserve2(unsigned char* program, int& count)
{
    program[count++] = 0x90; // nop
    program[count++] = 0x90; // nop
}

void vm_reserve3(unsigned char* program, int& count)
{
    program[count++] = 0x90; // nop
    program[count++] = 0x90; // nop
    program[count++] = 0x90; // nop
}

void vm_mov_reg_to_memory_x64(unsigned char* program, int &count, char dst, int dst_offset, char src)
{
    program[count++] = 0x48;
    program[count++] = 0x89;

    if (dst == VM_REGISTER_ESP)
    {
        program[count++] = ((src & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24;
        program[count++] = (unsigned char)(dst_offset & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((src & 0x7) << 3) | (0x2 << 6) | (dst & 0x7);
        program[count++] = (unsigned char)(dst_offset & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 24) & 0xff);
    }
}

void vm_mov_memory_to_reg_x64(unsigned char* program, int &count, char dst, char src, int src_offset)
{
    program[count++] = 0x48;
    program[count++] = 0x8b;

    if (src == VM_REGISTER_ESP)
    {
        program[count++] = ((dst & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24;
        program[count++] = (unsigned char)(src_offset & 0xff);
        program[count++] = (unsigned char)((src_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((dst & 0x7) << 3) | (0x2 << 6) | (src & 0x7);
        program[count++] = (unsigned char)(src_offset & 0xff);
        program[count++] = (unsigned char)((src_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 24) & 0xff);
    }
}

void vm_mov_reg_to_reg_x64(unsigned char* program, int &count, char dst, char src)
{
    program[count++] = 0x48;
    program[count++] = 0x89;
    program[count++] = 0xC0 | ((src & 0x7) << 0x3) | ((dst & 0x7) << 0);
}

void vm_cdq(unsigned char* program, int &count)
{
    program[count++] = 0x99;
}

void vm_return(const vm_instruction_table& table, unsigned char* program, int &count)
{
    vm_emit(table.ret, program, count, 0, 0, 0, 0);
}

void vm_push_reg(unsigned char* program, int &count, char reg)
{
    program[count++] = 0x50 | (reg & 0x7);
}

void vm_pop_reg(unsigned char* program, int &count, char reg)
{
    program[count++] = 0x58 | (reg & 0x7);
}

void vm_op_reg_to_reg(unsigned char op, unsigned char* program, int &count, char dst, char src)
{
    program[count++] = op;
    program[count++] = 0xC0 | ((src & 0x7) << 0x3) | ((dst & 0x7) << 0);
}

void vm_mov_reg_to_reg(unsigned char* program, int &count, char dst, char src)
{
    vm_op_reg_to_reg(0x89, program, count, dst, src);
}

void vm_sub_reg_to_reg(unsigned char* program, int& count, char dst, char src)
{
    vm_op_reg_to_reg(0x29, program, count, dst, src);
}

void vm_op_reg_to_memory(unsigned char op, unsigned char* program, int& count, char dst, int dst_offset, char src)
{
    program[count++] = op;

    if (dst == VM_REGISTER_ESP)
    {
        program[count++] = ((src & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24;
        program[count++] = (unsigned char)(dst_offset & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((src & 0x7) << 3) | (0x2 << 6) | (dst & 0x7);
        program[count++] = (unsigned char)(dst_offset & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 24) & 0xff);
    }
}

void vm_sub_reg_to_memory(unsigned char* program, int& count, char dst, int dst_offset, char src)
{
    vm_op_reg_to_memory(0x29, program, count, dst, dst_offset, src);
}

void vm_op_memory_to_reg(unsigned char op, unsigned char* program, int &count, char dst, char src, int src_offset)
{
    program[count++] = op;

    if (src == VM_REGISTER_ESP)
    {
        program[count++] = ((dst & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24;
        program[count++] = (unsigned char)(src_offset & 0xff);
        program[count++] = (unsigned char)((src_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((dst & 0x7) << 3) | (0x2 << 6) | (src & 0x7);
        program[count++] = (unsigned char)(src_offset & 0xff);
        program[count++] = (unsigned char)((src_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 24) & 0xff);
    }
}

void vm_sub_memory_to_reg(unsigned char* program, int &count, char dst, char src, int src_offset)
{
    vm_op_memory_to_reg(0x2B, program, count, dst, src, src_offset);
}

void vm_add_reg_to_reg_x64(unsigned char* program, int& count, char dst, char src)
{
    program[count++] = 0x48;
    program[count++] = 0x1;
    program[count++] = 0xC0 | ((src & 0x7) << 0x3) | ((dst & 0x7) << 0);
}

// TODO
//void vm_add_reg_to_memory_x64(unsigned char* program, int& count, char dst, int dst_offset, char src)
//{
//    program[count++] = 0x48;
//    program[count++] = 0x0;
//
//    if (dst == VM_REGISTER_ESP)
//    {
//        program[count++] = ((src & 0x7) << 3) | 0x4 | (0x2 << 6);
//        program[count++] = 0x24;
//        program[count++] = (unsigned char)(dst_offset & 0xff);
//        program[count++] = (unsigned char)((dst_offset >> 8) & 0xff);
//        program[count++] = (unsigned char)((dst_offset >> 16) & 0xff);
//        program[count++] = (unsigned char)((dst_offset >> 24) & 0xff);
//    }
//    else
//    {
//        program[count++] = ((src & 0x7) << 3) | (0x2 << 6) | (dst & 0x7);
//        program[count++] = (unsigned char)(dst_offset & 0xff);
//        program[count++] = (unsigned char)((dst_offset >> 8) & 0xff);
//        program[count++] = (unsigned char)((dst_offset >> 16) & 0xff);
//        program[count++] = (unsigned char)((dst_offset >> 24) & 0xff);
//    }
//}

void vm_sub_imm_to_reg_x64(unsigned char* program, int& count, char reg, int imm)
{
    program[count++] = 0x48;
    program[count++] = 0x81;
    program[count++] = 0xE8 | (reg & 0x7);
    program[count++] = (unsigned char)(imm & 0xff);
    program[count++] = (unsigned char)((imm >> 8) & 0xff);
    program[count++] = (unsigned char)((imm >> 16) & 0xff);
    program[count++] = (unsigned char)((imm >> 24) & 0xff);
}

void vm_mul_reg_to_reg(unsigned char* program, int& count, char dst, char src)
{
    program[count++] = 0xf;
    program[count++] = 0xaf;
    program[count++] = 0xC0 | ((dst & 0x7) << 0x3) | ((src & 0x7) << 0);
}

void vm_mul_memory_to_reg(unsigned char* program, int &count, char dst, char src, int src_offset)
{
    program[count++] = 0xf;
    program[count++] = 0xaf;

    if (src == VM_REGISTER_ESP)
    {
        program[count++] = ((dst & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24;
        program[count++] = (unsigned char)(src_offset & 0xff);
        program[count++] = (unsigned char)((src_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((dst & 0x7) << 3) | (0x2 << 6) | (src & 0x7);
        program[count++] = (unsigned char)(src_offset & 0xff);
        program[count++] = (unsigned char)((src_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 24) & 0xff);
    }
}

void vm_div_reg(const vm_instruction_table& table, unsigned char* program, int& count, char reg)
{
    vm_emit(table.idiv, program, count, reg, 0, 0, 0);
}

void vm_store(const vm_operation& operation, const std::vector<vm_stack_local>& locals, const vm_instruction_table& table, unsigned char* program, int &count, int stacksize, int reg)
{
    if (operation.arg2 == 1)
    {
        // move memory -> register
        auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        {
            vm_instruction ins;
            if (vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_OFFSET, &ins))
            {
                vm_emit(ins, program, count, reg, local.reg, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xfff].size, 0);
            }
        }
    }
    else
    {
        // mov imm -> register (EAX)
        vm_instruction ins;
        if (vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_IMMEDIATE, &ins))
        {
            vm_emit(ins, program, count, reg, 0, 0, operation.arg1);
        }
    }
}

void vm_mov(const vm_operation& operation, const std::vector<vm_stack_local>& locals, const vm_instruction_table& table, unsigned char* program, int &count, int stacksize)
{
    if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
    {
        // mov register -> memory
        auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        {
            vm_instruction ins;
            if (vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER | VM_INSTRUCTION_CODE_OFFSET, &ins))
            {
                vm_emit(ins, program, count, local.reg, operation.arg2, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xfff].size, 0);
            }
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
    {
        // mov memory -> register
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.type.fields[operation.arg2 & 0xfff].size == 4)
        {
            vm_instruction ins;
            if (vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_OFFSET, &ins))
            {
                vm_emit(ins, program, count, operation.arg1, local.reg, stacksize - local.offset - local.type.fields[operation.arg2 & 0xfff].offset - local.type.fields[operation.arg2 & 0xfff].size, 0);
            }
        }
    }
    else if (operation.flags == VM_OPERATION_FLAGS_REFERENCE1)
    {
        // mov register -> memory
        vm_instruction ins;
        if (vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, operation.arg2, 0, 0);
        }
    }
    else if (operation.flags == VM_OPERATION_FLAGS_REFERENCE2)
    {
        // mov memory -> register
        vm_instruction ins;
        if (vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_DST_REGISTER, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, operation.arg2, 0, 0);
        }
    }
    else if (operation.flags == VM_OPERATION_FLAGS_NONE)
    {
        // mov register -> register
        vm_instruction ins;
        if (vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_REGISTER, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, operation.arg2, 0, 0);
        }
    }
}

void vm_add(const vm_operation& operation, const std::vector<vm_stack_local>& locals, const vm_instruction_table& table, unsigned char* program, int &count, int stacksize, const vm_options& options)
{
    if (operation.flags == VM_OPERATION_FLAGS_NONE)
    {
        if (options.x64)
        {
            vm_add_reg_to_reg_x64(program, count, operation.arg1, operation.arg2); // add register -> register
        }
        else
        {
            vm_instruction ins;
            if (vm_find_ins(table.add, sizeof(table.add), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_REGISTER, &ins))
            {
                vm_emit(ins, program, count, operation.arg1, operation.arg2, 0, 0);
            }
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
    {
        // add register -> memory
        auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        {
            vm_instruction ins;
            if (vm_find_ins(table.add, sizeof(table.add), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER | VM_INSTRUCTION_CODE_OFFSET, &ins))
            {
                vm_emit(ins, program, count, local.reg, operation.arg2, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xffff].size, 0);
            }
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
    {
        // add memory -> register
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.type.fields[operation.arg2 & 0xfff].size == 4)
        {
            vm_instruction ins;
            if (vm_find_ins(table.add, sizeof(table.add), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_OFFSET, &ins))
            {
                vm_emit(ins, program, count, operation.arg1, local.reg, stacksize - local.offset - local.type.fields[operation.arg2 & 0xfff].offset - local.type.fields[operation.arg2 & 0xffff].size, 0);
            }
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_REFERENCE1) == VM_OPERATION_FLAGS_REFERENCE1)
    {
        vm_instruction ins;
        if (vm_find_ins(table.add, sizeof(table.add), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER | VM_INSTRUCTION_CODE_OFFSET, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, operation.arg2, 0, 0);
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_REFERENCE2) == VM_OPERATION_FLAGS_REFERENCE2)
    {
        vm_instruction ins;
        if (vm_find_ins(table.add, sizeof(table.add), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_OFFSET, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, operation.arg2, 0, 0);
        }
    }
}

void vm_sub(const vm_operation& operation, const std::vector<vm_stack_local>& locals, unsigned char* program, int &count, int stacksize, const vm_options& options)
{
    if (operation.flags == VM_OPERATION_FLAGS_NONE)
    {
        //if (options.x64)
        //{
        //    vm_sub_reg_to_reg_x64(program, count, operation.arg1, operation.arg2); // add register -> register
        //}
        //else
        {
            vm_sub_reg_to_reg(program, count, operation.arg1, operation.arg2); // add register -> register
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
    {
        // add register -> memory
        auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        {
            vm_sub_reg_to_memory(program, count, local.reg, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xffff].size, operation.arg2);
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
    {
        // add memory -> register
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.type.fields[operation.arg2 & 0xfff].size == 4)
        {
            vm_sub_memory_to_reg(program, count, operation.arg1, local.reg, stacksize - local.offset - local.type.fields[operation.arg2 & 0xfff].offset - local.type.fields[operation.arg2 & 0xffff].size);
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_REFERENCE1) == VM_OPERATION_FLAGS_REFERENCE1)
    {
        vm_sub_reg_to_memory(program, count, operation.arg1, 0, operation.arg2);
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_REFERENCE2) == VM_OPERATION_FLAGS_REFERENCE2)
    {
        vm_sub_memory_to_reg(program, count, operation.arg1, operation.arg2, 0);
    }
}

void vm_mul(const vm_operation& operation, const std::vector<vm_stack_local>& locals, unsigned char* program, int &count, int stacksize)
{
    if (operation.flags == VM_OPERATION_FLAGS_NONE)
    {
        // mul register -> register
        vm_mul_reg_to_reg(program, count, operation.arg1, operation.arg2);
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
    {
        // TODO: this does not appear to be a valid option
        // add register -> memory
        //auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        //if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        //{
        //    vm_mul_reg_to_memory(program, count, VM_REGISTER_ESP, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xffff].size, operation.arg2);
        //}
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
    {
        // add memory -> register
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.type.fields[operation.arg2 & 0xfff].size == 4)
        {
            vm_mul_memory_to_reg(program, count, operation.arg1, local.reg, stacksize - local.offset - local.type.fields[operation.arg2 & 0xfff].offset - local.type.fields[operation.arg2 & 0xffff].size);
        }
    }
}

void vm_cmp(const vm_operation& operation, const std::vector<vm_stack_local>& locals, const vm_instruction_table& table, unsigned char* program, int &count, int stacksize)
{
    if (operation.flags == VM_OPERATION_FLAGS_NONE)
    {
        // cmp register/register
        vm_instruction ins;
        if (vm_find_ins(table.cmp, sizeof(table.cmp), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_REGISTER, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, operation.arg2, 0, 0);
        }
    }
    else if ((operation.flags & (VM_OPERATION_FLAGS_FIELD1 | VM_OPERATION_FLAGS_FIELD2)) == (VM_OPERATION_FLAGS_FIELD1 | VM_OPERATION_FLAGS_FIELD2))
    {
        // Not supported
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
    {
        // cmp register/memory
        auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        {
            vm_instruction ins;
            if (vm_find_ins(table.cmp, sizeof(table.cmp), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_SRC_REGISTER | VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_OFFSET, &ins))
            {
                vm_emit(ins, program, count, local.reg, operation.arg2, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xffff].size, 0);
            }
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
    {
        // cmp memory/register
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.type.fields[operation.arg2 & 0xfff].size == 4)
        {
            vm_instruction ins;
            if (vm_find_ins(table.cmp, sizeof(table.cmp), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_OFFSET, &ins))
            {
                vm_emit(ins, program, count, operation.arg1, local.reg, stacksize - local.offset - local.type.fields[operation.arg2 & 0xfff].offset - local.type.fields[operation.arg2 & 0xffff].size, 0);
            }
        }
    }
}

void vm_call(unsigned char* program, int &count, int offset)
{
    offset -= 5;

    program[count++] = 0xE8;
    program[count++] = (unsigned char)(offset & 0xff);
    program[count++] = (unsigned char)((offset >> 8) & 0xff);
    program[count++] = (unsigned char)((offset >> 16) & 0xff);
    program[count++] = (unsigned char)((offset >> 24) & 0xff);
}

void vm_load_effective_address(const vm_operation& operation, const std::vector<vm_stack_local>& locals, const vm_instruction_table& table, unsigned char* program, int &count, int stacksize)
{
    if (operation.flags == VM_OPERATION_FLAGS_FIELD2)
    {
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.reference)
        {
            const int pointerSize = 8;
            vm_mov_memory_to_reg_x64(program, count, operation.arg1, local.reg, stacksize - local.offset - pointerSize);
            //vm_load_effective_address(program, count, operation.arg1, operation.arg1, -local.type.fields[operation.arg2 & 0xfff].offset/* - local.type.fields[operation.arg2 & 0xffff].size*/);
            vm_emit(table.lae, program, count, operation.arg1, operation.arg1, -local.type.fields[operation.arg2 & 0xfff].offset/* - local.type.fields[operation.arg2 & 0xffff].size*/, 0);
        }
        else
        {
            // error
        }
    }
}

void vm_inc(const vm_operation& operation, const std::vector<vm_stack_local>& locals, const vm_instruction_table& table, unsigned char* program, int &count, int stacksize)
{
    if (operation.flags == VM_OPERATION_FLAGS_REFERENCE1)
    {
        vm_instruction ins;
        if (vm_find_ins(table.inc, sizeof(table.inc), VM_INSTRUCTION_UNARY, VM_INSTRUCTION_CODE_DST_MEMORY, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, 0, 0, 0);
        }
    }
    else
    {
        vm_instruction ins;
        if (vm_find_ins(table.inc, sizeof(table.inc), VM_INSTRUCTION_UNARY, VM_INSTRUCTION_CODE_DST_REGISTER, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, 0, 0, 0);
        }
    }
}

void vm_dec(const vm_operation& operation, const std::vector<vm_stack_local>& locals, const vm_instruction_table& table, unsigned char* program, int &count, int stacksize)
{
    if (operation.flags == VM_OPERATION_FLAGS_REFERENCE1)
    {
        vm_instruction ins;
        if (vm_find_ins(table.dec, sizeof(table.dec), VM_INSTRUCTION_UNARY, VM_INSTRUCTION_CODE_DST_MEMORY, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, 0, 0, 0);
        }
    }
    else
    {
        vm_instruction ins;
        if (vm_find_ins(table.dec, sizeof(table.dec), VM_INSTRUCTION_UNARY, VM_INSTRUCTION_CODE_DST_REGISTER, &ins))
        {
            vm_emit(ins, program, count, operation.arg1, 0, 0, 0);
        }
    }
}

void vm_generate(const vm_scope& scope, const vm_scope& global, const vm_options& options, const vm_instruction_table& table, unsigned char* program, int &count, int& start)
{
    std::vector<vm_label_target> labels;
    std::vector<vm_stack_local> locals;
    std::vector<vm_stack_local> parameters;
    std::vector<vm_jump> jumps;
    std::vector<vm_method_local> methods;

    for (auto& child_scope : scope.children)
    {
        if (child_scope->operations.size() > 0) // check the scope contains executable operations
        {
            int childStart = count; // this is the entry point to the child

            vm_method_local methodLocal;
            methodLocal.position = childStart;
            methodLocal.scope = child_scope;
            methods.push_back(methodLocal);

            vm_generate(*child_scope, scope, options, table, program, count, childStart);
        }
    }

    for (auto& accessible_scope : global.children)
    {
        if (accessible_scope->operations.size() > 0) // check the scope contains executable operations
        {
            int childStart = count; // this is the entry point to the child

            vm_method_local methodLocal;
            methodLocal.position = childStart;
            methodLocal.scope = accessible_scope;
            methods.push_back(methodLocal);
        }
    }

    start = count; // set this after children are done, and before count is incremented

    int stacksize = 0;
    for (auto& declare : scope.decls)
    {
        const vm_structure& structure = global.structures[declare.typeId];
        int size = 0;
        for (int i = 0; i < structure.fieldcount; i++)
        {
            size += structure.fields[i].size;
        }

        if ((declare.flags & VM_DECL_FLAGS_PARAMETER) == VM_DECL_FLAGS_PARAMETER)
        {
            vm_stack_local local;
            local.offset = stacksize;
            local.type = structure;
            local.reg = VM_REGISTER_ESP;
            local.reference = (declare.flags & VM_DECL_FLAGS_REFERENCE) == VM_DECL_FLAGS_REFERENCE;
            locals.push_back(local);
            parameters.push_back(local);
            
            if (local.reference)
            {
                size = 8; // 64 bit pointer
            }

            stacksize += VM_ALIGN_16(size); // aligning
        }
        else
        {
            vm_stack_local local;
            local.offset = stacksize;
            local.type = structure;
            local.reg = VM_REGISTER_ESP;
            local.reference = false;
            locals.push_back(local);

            stacksize += VM_ALIGN_16(size); // aligning
        }
    }

    vm_push_reg(program, count, VM_REGISTER_EBP);
    vm_mov_reg_to_reg_x64(program, count, VM_REGISTER_EBP, VM_REGISTER_ESP);
    vm_sub_imm_to_reg_x64(program, count, VM_REGISTER_ESP, stacksize); // grow stack

    int parameterCount = 0;
    for (auto& parameter : parameters)
    {
        const vm_structure& structure = parameter.type;
        int size = 0;
        for (int i = 0; i < structure.fieldcount; i++)
        {
            size += structure.fields[i].size;
        }
        
        const int returnInstructionPtrSize = 8;
        vm_mov_memory_to_reg_x64(program, count, VM_REGISTER_EAX, VM_REGISTER_EBP, (parameterCount + 1) * 8 + returnInstructionPtrSize);

        if (parameter.reference)
        {
            // copy the pointer to callee frame
            int offset = stacksize - parameter.offset - 8;
            vm_mov_reg_to_memory_x64(program, count, VM_REGISTER_ESP, offset, VM_REGISTER_EAX);
        }
        else
        {
            vm_instruction mov_reg_to_mem;
            vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_OFFSET, &mov_reg_to_mem);
            
            vm_instruction mov_mem_to_reg;
            vm_find_ins(table.mov, sizeof(table.mov), VM_INSTRUCTION_BINARY, VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER | VM_INSTRUCTION_CODE_OFFSET, &mov_mem_to_reg);

            // copy each of the fields for the structs to the local variables (parameters)
            // via mov, this is somewhat inefficent
            for (int i = 0; i < structure.fieldcount; i++)
            {
                //vm_mov_memory_to_reg(program, count, VM_REGISTER_EBX, VM_REGISTER_EAX, -structure.fields[i].offset);
                vm_emit(mov_reg_to_mem, program, count, VM_REGISTER_EBX, VM_REGISTER_EAX, -structure.fields[i].offset, 0);

                int fieldOffset = structure.fields[i].offset;
                int fieldSize = structure.fields[i].size;
                int offset = stacksize - parameter.offset - fieldOffset - fieldSize;
                //vm_mov_reg_to_memory(program, count, VM_REGISTER_ESP, offset, VM_REGISTER_EBX);
                
                vm_emit(mov_mem_to_reg, program, count, VM_REGISTER_ESP, VM_REGISTER_EBX, offset, 0);
            }
        }

        parameterCount++;
    }

    for (auto& operation : scope.operations)
    {
        if (operation.code == VM_LABEL)
        {
            vm_label_target label;
            label.labelId = operation.arg1;
            label.initialized = false;
            label.position = 0;
            labels.push_back(label);
        }
    }
    
    for (auto& operation : scope.operations)
    {
        if (operation.code == VM_STORE_1)
        {
            vm_store(operation, locals, table, program, count, stacksize, VM_REGISTER_EAX);
        }
        else if (operation.code == VM_STORE_2)
        {
            vm_store(operation, locals, table, program, count, stacksize, VM_REGISTER_ECX);
        }
        else if (operation.code == VM_STORE_3)
        {
            vm_store(operation, locals, table, program, count, stacksize, VM_REGISTER_EDX);
        }
        else if (operation.code == VM_STORE_4)
        {
            vm_store(operation, locals, table, program, count, stacksize, VM_REGISTER_EBX);
        }
        else if (operation.code == VM_MOV)
        {
            vm_mov(operation, locals, table, program, count, stacksize);
        }
        else if (operation.code == VM_LEA)
        {
            vm_load_effective_address(operation, locals, table, program, count, stacksize);
        }
        else if (operation.code == VM_INC)
        {
            vm_inc(operation, locals, table, program, count, stacksize);
        }
        else if (operation.code == VM_DEC)
        {
            vm_dec(operation, locals, table, program, count, stacksize);
        }
        else if (operation.code == VM_ADD)
        {
            vm_add(operation, locals, table, program, count, stacksize, options);
        }
        else if (operation.code == VM_SUB)
        {
            // sub register -> register
            //vm_sub_reg_to_reg(program, count, operation.arg1, operation.arg2);
            vm_sub(operation, locals, program, count, stacksize, options);
        }
        else if (operation.code == VM_MUL)
        {
            vm_mul(operation, locals, program, count, stacksize);
        }
        else if (operation.code == VM_DIV)
        {
            if (operation.arg1 != VM_REGISTER_EAX)
            {
                vm_push_reg(program, count, VM_REGISTER_EAX); // push EAX (if not EAX = arg1)
            }
            vm_push_reg(program, count, VM_REGISTER_EDX); // push EDX
            vm_mov_reg_to_reg(program, count, VM_REGISTER_EAX, operation.arg1); // mov arg1 ->  EAX
            vm_cdq(program, count); // cdq
            vm_div_reg(table, program, count, operation.arg2); // EAX = EDX:EAX / reg
            vm_mov_reg_to_reg(program, count, operation.arg1, VM_REGISTER_EAX); // mov EAX -> arg1
            vm_pop_reg(program, count, VM_REGISTER_EDX); // pop to EDX
            if (operation.arg1 != VM_REGISTER_EAX)
            {
                vm_pop_reg(program, count, VM_REGISTER_EAX); // pop to EAX
            }
        }
        else if (operation.code == VM_CMP)
        {
            vm_cmp(operation, locals, table, program, count, stacksize);
        }
        else if (operation.code == VM_LABEL)
        {
            auto& label = labels.at(operation.arg1);
            label.initialized = true;
            label.position = count;
        }
        else if (operation.code == VM_JMP)
        {
            auto& label = labels.at(operation.arg1);
            vm_jump jump;
            jump.labelId = label.labelId;
            jump.position = count;
            jump.code = VM_JMP;
            vm_reserve2(program, count);
            jumps.push_back(jump);
        }
        else if (operation.code == VM_JMPLT)
        {
            auto& label = labels.at(operation.arg1);
            vm_jump jump;
            jump.labelId = label.labelId;
            jump.position = count;
            jump.code = VM_JMPLT;
            vm_reserve2(program, count);
            jumps.push_back(jump);
        }
        else if (operation.code == VM_JMPGT)
        {
            auto& label = labels.at(operation.arg1);
            vm_jump jump;
            jump.labelId = label.labelId;
            jump.position = count;
            jump.code = VM_JMPGT;
            vm_reserve2(program, count);
            jumps.push_back(jump);
        }
        else if (operation.code == VM_JMPEQ)
        {
            auto& label = labels.at(operation.arg1);
            vm_jump jump;
            jump.labelId = label.labelId;
            jump.position = count;
            jump.code = VM_JMPEQ;
            vm_reserve2(program, count);
            jumps.push_back(jump);
        }
        else if (operation.code == VM_CALL)
        {
            // what should happen is we should call LEA on each of the parameters to a register
            // then call push to plop them onto the stack (this is for a cstdcall, rather than a fastcall)
            // then call the method
            // after the method is called reclaim the stack space

            const int size = 8; // 64-bit memory address
            
            auto& method = global.methods.at(operation.arg1);
            int methodstacksize = 0;
            for (int i = method.paramcount-1; i >= 0; i--)
            {
                int typeId = method.parameters[i].typeId;
                auto& structure = global.structures.at(typeId);

                int localId = (operation.arg2 >> (i * size)) & 0xff;
                vm_stack_local& local = locals.at(localId);

                int offset = 0;
                if (local.reference)
                {
                    offset = stacksize + methodstacksize - local.offset - 8;
                    vm_mov_memory_to_reg_x64(program, count, VM_REGISTER_EAX, local.reg, offset);
                }
                else
                {
                    offset = stacksize + methodstacksize - local.offset - structure.fields[0].offset - structure.fields[0].size;
                    //vm_load_effective_address(program, count, VM_REGISTER_EAX, VM_REGISTER_ESP, offset);
                    vm_emit(table.lae, program, count, VM_REGISTER_EAX, VM_REGISTER_ESP, offset, 0);
                }

                vm_push_reg(program, count, VM_REGISTER_EAX);

                methodstacksize += size;
            }

            for (int i = 0; i < methods.size(); i++)
            {
                if (method.scope == methods.at(i).scope)
                {
                    vm_call(program, count, methods.at(i).position - count);
                    break;
                }
            }

            vm_sub_imm_to_reg_x64(program, count, VM_REGISTER_ESP, -methodstacksize); // restore stack (should be a add once implemented)
        }
    }

    vm_mov_reg_to_reg_x64(program, count, VM_REGISTER_ESP, VM_REGISTER_EBP);
    vm_pop_reg(program, count, VM_REGISTER_EBP);
    vm_return(table, program, count);

    for (auto& jump : jumps)
    {
        auto& label = labels.at(jump.labelId);
        if (jump.code == VM_JMP)
        {
            program[jump.position] = 0xEB;
            program[jump.position + 1] = label.position - jump.position - 2;
        }
        else if (jump.code == VM_JMPLT)
        {
            program[jump.position] = 0x70 | 0x2;
            program[jump.position + 1] = label.position - jump.position - 2;
        }
        else if (jump.code == VM_JMPGT)
        {
            program[jump.position] = 0x70 | 0x7;
            program[jump.position + 1] = label.position - jump.position - 2;
        }
        else if (jump.code == VM_JMPEQ)
        {
            program[jump.position] = 0x70 | 0x4;
            program[jump.position + 1] = label.position - jump.position - 2;
        }
    }
}

void vm_execute(const vm_scope& scope, const vm_options& options)
{
    unsigned char program[512];
    int count = 0;
    int entryPoint = 0;
    vm_instruction_table table;

    vm_generate(scope, scope, options, table, program, count, entryPoint);

    void* data = VirtualAlloc(0, count, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    memcpy(data, program, count);
    unsigned long oldProtect;
    if (VirtualProtect(data, count, PAGE_EXECUTE, &oldProtect))
    {
        FlushInstructionCache(0, data, count);

        int(*fn)(void) = (int(*)(void))((unsigned char*)data + entryPoint);
        int result = fn();
        std::cout << result << std::endl;
    }

    VirtualFree(data, count, MEM_RELEASE | MEM_DECOMMIT);
}
