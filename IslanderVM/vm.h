#pragma once
#include <vector>

#define VM_STRUCTURE_MAX_NAME 128
#define VM_STRUCTURE_MAX_FIELD 128
#define VM_FIELD_MAX_NAME 128
#define VM_PROC_MAX_NAME 128
#define VM_DECL_NAME_MAX_SIZE 256
#define VM_LABEL_NAME_MAX_SIZE 256
#define VM_PROC_MAX_PARAMETERS 16

struct vm_scope;

enum vm_code
{
    VM_NOP = 0x0,
    VM_ADD = 0x1,
    VM_SUB = 0x2,
    VM_MUL = 0x3,
    VM_DIV = 0x4,
    VM_MOV = 0x5,
    VM_LOAD_1 = 0x10,
    VM_LOAD_2 = 0x11,
    VM_LOAD_3 = 0x12,
    VM_LOAD_4 = 0x13,
    VM_STORE_1 = 0x20,
    VM_STORE_2 = 0x21,
    VM_STORE_3 = 0x22,
    VM_STORE_4 = 0x23,
    VM_RETURN = 0x35,
    VM_CALL = 0x36,
    VM_CMP = 0x40,
    VM_JMP = 0x50,
    VM_JMPLT = 0x51,
    VM_JMPGT = 0x52,
    VM_JMPEQ = 0x53,
    VM_LABEL = 0x60
};

enum vm_operation_flags
{
    VM_OPERATION_FLAGS_NONE = 0x0,
    VM_OPERATION_FLAGS_FIELD1 = 0x1,
    VM_OPERATION_FLAGS_FIELD2 = 0x2
};

struct vm_operation
{
    vm_code code;
    int arg1;
    int arg2;
    int flags;
};

struct vm_field
{
    int size;
    int array_size;
    int offset;
    char name[VM_FIELD_MAX_NAME];
};

struct vm_structure
{
    int fieldcount;
    char name[VM_STRUCTURE_MAX_NAME];
    vm_field fields[VM_STRUCTURE_MAX_FIELD];
};

enum vm_decl_flags
{
    VM_DECL_FLAGS_NONE = 0,
    VM_DECL_FLAGS_PARAMETER = 1,
    VM_DECL_FLAGS_REFERENCE = 2
};

struct vm_decl_name
{
    int typeId;
    int flags;
    char name[VM_DECL_NAME_MAX_SIZE];
};

struct vm_label
{
    int labelId;
    int defined;
    char name[VM_DECL_NAME_MAX_SIZE];
};

struct vm_method
{
    int methodId;
    int paramcount;
    vm_scope* scope;
    char name[VM_PROC_MAX_NAME];
    vm_decl_name parameters[VM_PROC_MAX_PARAMETERS];
};

enum vm_scope_flags
{
    VM_SCOPE_FLAG_NONE = 0x0,
    VM_SCOPE_FLAG_DECLARATION = 0x1,
    VM_SCOPE_FLAG_LABEL = 0x2,
    VM_SCOPE_FLAG_OPERATION = 0x4,
    VM_SCOPE_FLAG_STRUCTURE = 0x8,
    VM_SCOPE_FLAG_METHOD = 0x10
};

struct vm_scope
{
    int flags;
    std::vector<vm_operation> operations;
    std::vector<vm_label> labels;
    std::vector<vm_decl_name> decls;
    std::vector<vm_structure> structures;
    std::vector<vm_method> methods;
    std::vector<vm_scope*> children;
};

struct vm_options
{
    bool x64;
};

void vm_execute(const vm_scope& scope, const vm_options& options);
