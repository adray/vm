#include "vm.h"
#include <Windows.h>
#include <iostream>

#define VM_ALIGN_16(x) ((x + 0xf) & ~(0xf))

struct vm_stack_local
{
    int reg;
    int offset;
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

void vm_cpuid(unsigned char* program, int& count)
{
    program[count++] = 0xf;
    program[count++] = 0xa2;
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

void vm_mov_reg_to_memory(unsigned char* program, int &count, char dst, char src)
{
    program[count++] = 0x89;

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

void vm_mov_reg_to_memory(unsigned char* program, int &count, char dst, int dst_offset, char src)
{
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

void vm_mov_memory_to_reg(unsigned char* program, int &count, char dst, char src)
{
    program[count++] = 0x8b;

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

void vm_mov_memory_to_reg(unsigned char* program, int &count, char dst, char src, int src_offset)
{
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

void vm_mov_imm_to_reg(unsigned char* program, int &count, int imm, char reg)
{
    program[count++] = 0xb8 | reg;
    program[count++] = (unsigned char)(imm & 0xff);
    program[count++] = (unsigned char)((imm >> 8) & 0xff);
    program[count++] = (unsigned char)((imm >> 16) & 0xff);
    program[count++] = (unsigned char)((imm >> 24) & 0xff);
}

void vm_mov_reg_to_reg(unsigned char* program, int &count, char dst, char src)
{
    program[count++] = 0x89;
    program[count++] = 0xC0 | ((src & 0x7) << 0x3) | ((dst & 0x7) << 0);
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

void vm_return(unsigned char* program, int &count)
{
    program[count++] = 0xc3;
}

void vm_push_reg(unsigned char* program, int &count, char reg)
{
    program[count++] = 0x50 | (reg & 0x7);
}

void vm_pop_reg(unsigned char* program, int &count, char reg)
{
    program[count++] = 0x58 | (reg & 0x7);
}

void vm_add_reg_to_reg(unsigned char* program, int& count, char dst, char src)
{
    program[count++] = 0x1;
    program[count++] = 0xC0 | ((src & 0x7) << 0x3) | ((dst & 0x7) << 0);
}

void vm_add_reg_to_memory(unsigned char* program, int& count, char dst, int dst_offset, char src)
{
    program[count++] = 0x1;
    
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

void vm_add_memory_to_reg(unsigned char* program, int &count, char dst, char src, int src_offset)
{
    program[count++] = 0x3;

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

void vm_sub_reg_to_reg(unsigned char* program, int& count, char dst, char src)
{
    program[count++] = 0x29;
    program[count++] = 0xC0 | ((src & 0x7) << 0x3) | ((dst & 0x7) << 0);
}

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

void vm_div_reg(unsigned char* program, int& count, char reg)
{
    program[count++] = 0xf7;
    program[count++] = 0xf8 | ((reg & 0x7) << 0);
}

void vm_cmp_reg_with_reg(unsigned char* program, int& count, char reg1, char reg2)
{
    program[count++] = 0x39;
    program[count++] = 0xC0 | ((reg2 & 0x7) << 0x3) | ((reg1 & 0x7) << 0);
}

void vm_cmp_memory_with_reg(unsigned char* program, int& count, char dst, int dst_offset, char src)
{
    program[count++] = 0x39;

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

void vm_store(const vm_operation& operation, const std::vector<vm_stack_local>& locals, unsigned char* program, int &count, int stacksize, int reg)
{
    if (operation.arg2 == 1)
    {
        // move memory -> register
        auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        {
            vm_mov_memory_to_reg(program, count, reg, local.reg, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xfff].size);
        }
    }
    else
    {
        // mov imm -> register (EAX)
        vm_mov_imm_to_reg(program, count, operation.arg1, reg);
    }
}

void vm_mov(const vm_operation& operation, const std::vector<vm_stack_local>& locals, unsigned char* program, int &count, int stacksize)
{
    if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
    {
        // mov register -> memory
        auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        {
            vm_mov_reg_to_memory(program, count, local.reg, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xfff].size, operation.arg2);
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
    {
        // mov memory -> register
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.type.fields[operation.arg2 & 0xfff].size == 4)
        {
            vm_mov_memory_to_reg(program, count, operation.arg1, local.reg, stacksize - local.offset - local.type.fields[operation.arg2 & 0xfff].offset - local.type.fields[operation.arg2 & 0xfff].size);
        }
    }
    else if (operation.flags == VM_OPERATION_FLAGS_NONE)
    {
        // mov register -> register
        vm_mov_reg_to_reg(program, count, operation.arg1, operation.arg2);
    }
}

void vm_add(const vm_operation& operation, const std::vector<vm_stack_local>& locals, unsigned char* program, int &count, int stacksize, const vm_options& options)
{
    if (operation.flags == VM_OPERATION_FLAGS_NONE)
    {
        if (options.x64)
        {
            vm_add_reg_to_reg_x64(program, count, operation.arg1, operation.arg2); // add register -> register
        }
        else
        {
            vm_add_reg_to_reg(program, count, operation.arg1, operation.arg2); // add register -> register
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
    {
        // add register -> memory
        auto& local = locals[(operation.arg1 >> 16) & 0xfff];
        if (local.type.fields[operation.arg1 & 0xfff].size == 4)
        {
            vm_add_reg_to_memory(program, count, local.reg, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xffff].size, operation.arg2);
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
    {
        // add memory -> register
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.type.fields[operation.arg2 & 0xfff].size == 4)
        {
            vm_add_memory_to_reg(program, count, operation.arg1, local.reg, stacksize - local.offset - local.type.fields[operation.arg2 & 0xfff].offset - local.type.fields[operation.arg2 & 0xffff].size);
        }
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

void vm_cmp(const vm_operation& operation, const std::vector<vm_stack_local>& locals, unsigned char* program, int &count, int stacksize)
{
    if (operation.flags == VM_OPERATION_FLAGS_NONE)
    {
        // cmp register/register
        vm_cmp_reg_with_reg(program, count, operation.arg1, operation.arg2);
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
            vm_cmp_memory_with_reg(program, count, local.reg, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset - local.type.fields[operation.arg1 & 0xffff].size, operation.arg2);
        }
    }
    else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
    {
        // cmp memory/register
        auto& local = locals[(operation.arg2 >> 16) & 0xfff];
        if (local.type.fields[operation.arg2 & 0xfff].size == 4)
        {
            vm_cmp_memory_with_reg(program, count, local.reg, stacksize - local.offset - local.type.fields[operation.arg2 & 0xfff].offset - local.type.fields[operation.arg2 & 0xffff].size, operation.arg1);
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

void vm_load_effective_address(unsigned char* program, int& count, int dst, int src, int src_offset)
{
    program[count++] = 0x48;
    program[count++] = 0x8D;

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

void vm_generate(const vm_scope& scope, const vm_scope& global, const vm_options& options, unsigned char* program, int &count, int& start)
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

            vm_generate(*child_scope, scope, options, program, count, childStart);
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
            locals.push_back(local);
            parameters.push_back(local);
            
            stacksize += VM_ALIGN_16(size); // aligning
        }
        else
        {
            vm_stack_local local;
            local.offset = stacksize;
            local.type = structure;
            local.reg = VM_REGISTER_ESP;
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
        
        // copy each of the fields for the structs to the local variables (parameters)
        // via mov, this is somewhat inefficent
        for (int i = 0; i < structure.fieldcount; i++)
        {
            vm_mov_memory_to_reg(program, count, VM_REGISTER_EBX, VM_REGISTER_EAX, -structure.fields[i].offset);

            int fieldOffset = structure.fields[i].offset;
            int fieldSize = structure.fields[i].size;
            int offset = stacksize - parameter.offset - fieldOffset - fieldSize;
            vm_mov_reg_to_memory(program, count, VM_REGISTER_ESP, offset, VM_REGISTER_EBX);
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
            vm_store(operation, locals, program, count, stacksize, VM_REGISTER_EAX);
        }
        else if (operation.code == VM_STORE_2)
        {
            vm_store(operation, locals, program, count, stacksize, VM_REGISTER_ECX);
        }
        else if (operation.code == VM_STORE_3)
        {
            vm_store(operation, locals, program, count, stacksize, VM_REGISTER_EDX);
        }
        else if (operation.code == VM_STORE_4)
        {
            vm_store(operation, locals, program, count, stacksize, VM_REGISTER_EBX);
        }
        else if (operation.code == VM_MOV)
        {
            vm_mov(operation, locals, program, count, stacksize);
        }
        else if (operation.code == VM_ADD)
        {
            vm_add(operation, locals, program, count, stacksize, options);
        }
        else if (operation.code == VM_SUB)
        {
            // sub register -> register
            vm_sub_reg_to_reg(program, count, operation.arg1, operation.arg2);
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
            vm_div_reg(program, count, operation.arg2); // EAX = EDX:EAX / reg
            vm_mov_reg_to_reg(program, count, operation.arg1, VM_REGISTER_EAX); // mov EAX -> arg1
            vm_pop_reg(program, count, VM_REGISTER_EDX); // pop to EDX
            if (operation.arg1 != VM_REGISTER_EAX)
            {
                vm_pop_reg(program, count, VM_REGISTER_EAX); // pop to EAX
            }
        }
        else if (operation.code == VM_CMP)
        {
            vm_cmp(operation, locals, program, count, stacksize);
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

                int offset = stacksize + methodstacksize - local.offset - structure.fields[0].offset - structure.fields[0].size;

                vm_load_effective_address(program, count, VM_REGISTER_EAX, VM_REGISTER_ESP, offset);
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
    vm_return(program, count);

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

    vm_generate(scope, scope, options, program, count, entryPoint);

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
