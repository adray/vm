#pragma once
#include <vector>

#define VM_STRUCTURE_MAX_NAME 128
#define VM_STRUCTURE_MAX_FIELD 128

enum vm_code
{
    VM_NOP = 0x0,
    VM_ADD = 0x1,
    VM_SUB = 0x2,
    VM_MUL = 0x3,
    VM_DIV = 0x4,
    VM_LOAD_1 = 0x10,
    VM_LOAD_2 = 0x11,
    VM_LOAD_3 = 0x12,
    VM_LOAD_4 = 0x13,
    VM_STORE_1 = 0x20,
    VM_STORE_2 = 0x21,
    VM_STORE_3 = 0x22,
    VM_STORE_4 = 0x23
};


struct vm_operation
{
    vm_code code;
    int arg1;
    int arg2;
};

struct vm_field
{
    int size;
    int array_size;
};

struct vm_structure
{
    int fieldcount;
    char name[VM_STRUCTURE_MAX_NAME];
    vm_field fields[VM_STRUCTURE_MAX_FIELD];
};

struct vm_options
{
    bool x64;
};

void vm_execute(const std::vector<vm_operation>& operations, const vm_options& options);
