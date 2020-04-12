#include "vm.h"
#include <Windows.h>
#include <iostream>

#define VM_ALIGN_16(x) ((x + 0xf) & ~(0xf))

struct vm_stack_local
{
    int offset;
    vm_structure type;
};

enum vm_register
{
    VM_REGISTER_EAX = 0x0,
    VM_REGISTER_ECX = 0x1,
    VM_REGISTER_EDX = 0x2,
    VM_REGISTER_EBX = 0x3,
    VM_REGISTER_ESP = 0x4,
    VM_REGISTER_EBP = 0x5
};

void vm_cpuid(unsigned char* program, int& count)
{
    program[count++] = 0xf;
    program[count++] = 0xa2;
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

void vm_div_reg(unsigned char* program, int& count, char reg)
{
    program[count++] = 0xf7;
    program[count++] = 0xf8 | ((reg & 0x7) << 0);
}

void vm_generate(const std::vector<vm_operation>& operations, const std::vector<vm_structure>& structures, const vm_options& options, unsigned char* program, int &count)
{
    vm_push_reg(program, count, VM_REGISTER_EBP);
    vm_mov_reg_to_reg_x64(program, count, VM_REGISTER_EBP, VM_REGISTER_ESP);

    std::vector<vm_stack_local> locals;
    int stacksize = 0;
    for (auto& operation : operations)
    {
        if (operation.code == VM_DECLARE)
        {
            const vm_structure& structure = structures[operation.arg1];
            int size = 0;
            for (int i = 0; i < structure.fieldcount; i++)
            {
                size += structure.fields[i].size;
            }

            vm_stack_local local;
            local.offset = stacksize;
            local.type = structure;
            locals.push_back(local);

            stacksize += VM_ALIGN_16(size); // aligning
        }
    }

    vm_sub_imm_to_reg_x64(program, count, VM_REGISTER_ESP, stacksize); // grow stack

    for (auto& operation : operations)
    {
        if (operation.code == VM_STORE_1)
        {
            if (operation.arg2 == 1)
            {
                // move memory -> register
                auto& local = locals[(operation.arg1 >> 16) & 0xfff];
                if (local.type.fields[operation.arg1 & 0xfff].size == 4)
                {
                    vm_mov_memory_to_reg(program, count, VM_REGISTER_EAX, VM_REGISTER_ESP, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset);
                }
            }
            else
            {
                // mov imm -> register (EAX)
                vm_mov_imm_to_reg(program, count, operation.arg1, VM_REGISTER_EAX);
            }
        }
        else if (operation.code == VM_STORE_2)
        {
            if (operation.arg2 == 1)
            {
                // move memory -> register

            }
            else
            {
                // mov imm -> register (ECX)
                vm_mov_imm_to_reg(program, count, operation.arg1, VM_REGISTER_ECX);
            }
        }
        else if (operation.code == VM_STORE_3)
        {
            if (operation.arg2 == 1)
            {
                // move memory -> register

            }
            else
            {
                // mov imm -> register (EDX)
                vm_mov_imm_to_reg(program, count, operation.arg1, VM_REGISTER_EDX);
            }
        }
        else if (operation.code == VM_STORE_4)
        {
            if (operation.arg2 == 1)
            {
                // move memory -> register

            }
            else
            {
                // mov imm -> register (EBX)
                vm_mov_imm_to_reg(program, count, operation.arg1, VM_REGISTER_EBX);
            }
        }
        else if (operation.code == VM_MOV)
        {
            if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
            {
                // TODO: mov register -> memory
                auto& local = locals[(operation.arg1 >> 16) & 0xfff];
                if (local.type.fields[operation.arg1 & 0xfff].size == 4)
                {
                    vm_mov_reg_to_memory(program, count, VM_REGISTER_ESP, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset, operation.arg2);
                }
            }
        }
        else if (operation.code == VM_ADD)
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
            else if ((operation.flags & (VM_OPERATION_FLAGS_FIELD1 | VM_OPERATION_FLAGS_FIELD2)) == (VM_OPERATION_FLAGS_FIELD1 | VM_OPERATION_FLAGS_FIELD2))
            {
                // TODO: add memory -> memory
            }
            else if ((operation.flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1)
            {
                // TODO: add register -> memory
                auto& local = locals[(operation.arg1 >> 16) & 0xfff];
                if (local.type.fields[operation.arg1 & 0xfff].size == 4)
                {
                    vm_add_reg_to_memory(program, count, VM_REGISTER_ESP, stacksize - local.offset - local.type.fields[operation.arg1 & 0xfff].offset /*TODO*/, operation.arg2);
                }
            }
            else if ((operation.flags & VM_OPERATION_FLAGS_FIELD2) == VM_OPERATION_FLAGS_FIELD2)
            {
                // TODO: add memory -> register
            }
        }
        else if (operation.code == VM_SUB)
        {
            // sub register -> register
            vm_sub_reg_to_reg(program, count, operation.arg1, operation.arg2);
        }
        else if (operation.code == VM_MUL)
        {
            // mul register -> register
            vm_mul_reg_to_reg(program, count, operation.arg1, operation.arg2);
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
    }

    vm_mov_reg_to_reg_x64(program, count, VM_REGISTER_ESP, VM_REGISTER_EBP);
    vm_pop_reg(program, count, VM_REGISTER_EBP);
    vm_return(program, count);
}

void vm_execute(const std::vector<vm_operation>& operations, const std::vector<vm_structure>& structures, const vm_options& options)
{
    unsigned char program[256];
    int count = 0;

    vm_generate(operations, structures, options, program, count);

    void* data = VirtualAlloc(0, count, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    memcpy(data, program, count);
    unsigned long oldProtect;
    if (VirtualProtect(data, count, PAGE_EXECUTE, &oldProtect))
    {
        FlushInstructionCache(0, data, count);

        int(*fn)(void) = (int(*)(void)) data;
        int result = fn();
        std::cout << result << std::endl;
    }

    VirtualFree(data, count, MEM_RELEASE | MEM_DECOMMIT);
}
