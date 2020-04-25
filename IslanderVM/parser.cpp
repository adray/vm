#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <streambuf>
#include <stack>
#include "vm.h"

enum vm_token
{
    VM_TOKEN_NONE = 0x0,
    VM_TOKEN_ADD = 0x1,
    VM_TOKEN_SUB = 0x2,
    VM_TOKEN_MUL = 0x3,
    VM_TOKEN_DIV = 0x4,
    VM_TOKEN_MOV = 0x5,
    VM_TOKEN_LEA = 0x6,
    VM_TOKEN_LOAD1 = 0x10,
    VM_TOKEN_LOAD2 = 0x11,
    VM_TOKEN_LOAD3 = 0x12,
    VM_TOKEN_LOAD4 = 0x13,
    VM_TOKEN_STORE1 = 0x20,
    VM_TOKEN_STORE2 = 0x21,
    VM_TOKEN_STORE3 = 0x22,
    VM_TOKEN_STORE4 = 0x23,
    VM_TOKEN_END = 0x30,
    VM_TOKEN_STRUCTURE = 0x31,
    VM_TOKEN_FIELD = 0x32,
    VM_TOKEN_DECLARE = 0x33,
    VM_TOKEN_PROC = 0x34,
    VM_TOKEN_RETURN = 0x35,
    VM_TOKEN_CALL = 0x36,
    VM_TOKEN_CMP = 0x40,
    VM_TOKEN_JMP = 0x50,
    VM_TOKEN_JMPLT = 0x51,
    VM_TOKEN_JMPGT = 0x52,
    VM_TOKEN_JMPEQ = 0x53,
    VM_TOKEN_LOCATION1 = 0x90,
    VM_TOKEN_LOCATION2 = 0x91,
    VM_TOKEN_LOCATION3 = 0x92,
    VM_TOKEN_LOCATION4 = 0x93,
    VM_TOKEN_LITERAL = 0x100,
    VM_TOKEN_SEPERATOR = 0x101,
    VM_TOKEN_COLON = 0x102,
    VM_TOKEN_DOT = 0x103,
    VM_TOKEN_ADDRESS = 0x104,
    VM_TOKEN_BRACKET_OPEN = 0x105,
    VM_TOKEN_BRACKET_CLOSE = 0x106,
    VM_TOKEN_EOF = 0x1000
};

int find_vm_label_const(vm_label& label, const std::vector<vm_label>& labels)
{
    int labelId = -1;
    for (int i = 0; i < labels.size(); i++)
    {
        if (strcmp(label.name, labels[i].name) == 0)
        {
            labelId = i;
            label.labelId = labels[i].labelId;
            break;
        }
    }

    return labelId;
}

int find_vm_decl_const(vm_decl_name& decl, const std::vector<vm_decl_name>& decls)
{
    int declId = -1;
    for (int i = 0; i < decls.size(); i++)
    {
        if (strcmp(decl.name, decls[i].name) == 0)
        {
            declId = i;
            decl.typeId = decls[i].typeId;
            break;
        }
    }

    return declId;
}

void read_vm_identifier(const std::string& input, int start, char* name, int maxLength)
{
    char* end = name + maxLength;
    bool eatingWhitespace = true;
    for (; start < input.length() && name < end - 1; start++)
    {
        if (input[start] == ' ')
        {
            if (eatingWhitespace)
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if (input[start] == '\n')
        {
            break;
        }

        if (input[start] == ';')
        {
            break;
        }

        if (input[start] == '.')
        {
            break;
        }

        if (input[start] == ',')
        {
            break;
        }

        if (input[start] == ':')
        {
            break;
        }

        if (input[start] == '(')
        {
            break;
        }

        if (input[start] == ')')
        {
            break;
        }

        if (input[start] == '&')
        {
            break;
        }

        *name = input[start];
        name++;
        eatingWhitespace = false;
    }

    *name = '\0';
}

void get_next_token(const std::string& line, int* pos, vm_token* token)
{
    for (; *pos < line.size(); (*pos)++)
    {
        if (line[*pos] != ' ' && line[*pos] != '\n')
        {
            break;
        }
    }

    if (line[*pos] == ';')
    {
        for (; *pos < line.size(); (*pos)++)
        {
            if (line[*pos] == '\n')
            {
                (*pos)++;
                break;
            }
        }
        return get_next_token(line, pos, token);
    }

    if (line[*pos] == ',')
    {
        *token = VM_TOKEN_SEPERATOR;
        (*pos)++;
        return;
    }

    if (line[*pos] == ':')
    {
        *token = VM_TOKEN_COLON;
        (*pos)++;
        return;
    }

    if (line[*pos] == '.')
    {
        *token = VM_TOKEN_DOT;
        (*pos)++;
        return;
    }

    if (line[*pos] == '(')
    {
        *token = VM_TOKEN_BRACKET_OPEN;
        (*pos)++;
        return;
    }

    if (line[*pos] == ')')
    {
        *token = VM_TOKEN_BRACKET_CLOSE;
        (*pos)++;
        return;
    }

    if (line[*pos] == '&')
    {
        *token = VM_TOKEN_ADDRESS;
        (*pos)++;
        return;
    }

    int data_pos = 0;
    char data[128];
    for (; *pos < line.size(); (*pos)++)
    {
        if (line[*pos] == ' ')
        {
            break;
        }
        else if (line[*pos] == ',')
        {
            break;
        }
        else if (line[*pos] == '.')
        {
            break;
        }
        else if (line[*pos] == ':')
        {
            break;
        }
        else if (line[*pos] == ';')
        {
            break;
        }
        else if (line[*pos] == '(')
        {
            break;
        }
        else if (line[*pos] == ')')
        {
            break;
        }
        else if (line[*pos] == '&')
        {
            break;
        }
        else if (line[*pos] == '\n')
        {
            (*pos)++;
            break;
        }

        data[data_pos] = line[*pos];
        data_pos++;
    }

    if (data_pos == 0 && line.size() == *pos)
    {
        *token = VM_TOKEN_EOF;
        return;
    }

    data[data_pos] = '\0';

    if (strcmp(data, "ADD") == 0)
    {
        *token = VM_TOKEN_ADD;
    }
    else if (strcmp(data, "SUB") == 0)
    {
        *token = VM_TOKEN_SUB;
    }
    else if (strcmp(data, "DIV") == 0)
    {
        *token = VM_TOKEN_DIV;
    }
    else if (strcmp(data, "MUL") == 0)
    {
        *token = VM_TOKEN_MUL;
    }
    else if (strcmp(data, "MOV") == 0)
    {
        *token = VM_TOKEN_MOV;
    }
    else if (strcmp(data, "LEA") == 0)
    {
        *token = VM_TOKEN_LEA;
    }
    else if (strcmp(data, "CMP") == 0)
    {
        *token = VM_TOKEN_CMP;
    }
    else if (strcmp(data, "STORE1") == 0)
    {
        *token = VM_TOKEN_STORE1;
    }
    else if (strcmp(data, "STORE2") == 0)
    {
        *token = VM_TOKEN_STORE2;
    }
    else if (strcmp(data, "STORE3") == 0)
    {
        *token = VM_TOKEN_STORE3;
    }
    else if (strcmp(data, "STORE4") == 0)
    {
        *token = VM_TOKEN_STORE4;
    }
    else if (strcmp(data, "S1") == 0)
    {
        *token = VM_TOKEN_LOCATION1;
    }
    else if (strcmp(data, "S2") == 0)
    {
        *token = VM_TOKEN_LOCATION2;
    }
    else if (strcmp(data, "S3") == 0)
    {
        *token = VM_TOKEN_LOCATION3;
    }
    else if (strcmp(data, "S4") == 0)
    {
        *token = VM_TOKEN_LOCATION4;
    }
    else if (strcmp(data, "STRUCTURE") == 0)
    {
        *token = VM_TOKEN_STRUCTURE;
    }
    else if (strcmp(data, "FIELD") == 0)
    {
        *token = VM_TOKEN_FIELD;
    }
    else if (strcmp(data, "END") == 0)
    {
        *token = VM_TOKEN_END;
    }
    else if (strcmp(data, "DECLARE") == 0)
    {
        *token = VM_TOKEN_DECLARE;
    }
    else if (strcmp(data, "PROC") == 0)
    {
        *token = VM_TOKEN_PROC;
    }
    else if (strcmp(data, "RETURN") == 0)
    {
        *token = VM_TOKEN_RETURN;
    }
    else if (strcmp(data, "CALL") == 0)
    {
        *token = VM_TOKEN_CALL;
    }
    else if (strcmp(data, "JUMP") == 0)
    {
        *token = VM_TOKEN_JMP;
    }
    else if (strcmp(data, "JUMPLT") == 0)
    {
        *token = VM_TOKEN_JMPLT;
    }
    else if (strcmp(data, "JUMPGT") == 0)
    {
        *token = VM_TOKEN_JMPGT;
    }
    else if (strcmp(data, "JUMPEQ") == 0)
    {
        *token = VM_TOKEN_JMPEQ;
    }
    else
    {
        *token = VM_TOKEN_LITERAL;
    }
}

bool read_vm_store(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure> structs, int* pos, int* start, vm_token* token, vm_operation* op)
{
    vm_token store = *token;

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token == VM_TOKEN_LITERAL)
    {
        op->code = (vm_code)store; // kinda hack

        vm_decl_name decl;
        read_vm_identifier(input, *start, decl.name, sizeof(decl.name));
        int declId = find_vm_decl_const(decl, decls);
        if (declId == -1) // standard literal
        {
            const char* ptr = input.c_str();
            op->arg1 = strtol(ptr + (*start), 0, 10);
            op->arg2 = 0;
            return true;
        }
        else
        {
            *start = *pos;
            get_next_token(input, pos, token);
            if (*token != VM_TOKEN_DOT)
            {
                // error
                return false;
            }

            *start = *pos;
            get_next_token(input, pos, token);
            if (*token != VM_TOKEN_LITERAL)
            {
                // error
                return false;
            }

            char fieldname[VM_FIELD_MAX_NAME];
            read_vm_identifier(input, *start, fieldname, sizeof(fieldname));

            int fieldOffset = -1;
            vm_structure type = structs[decl.typeId];
            for (int i = 0; i < type.fieldcount; i++)
            {
                if (strcmp(fieldname, type.fields[i].name) == 0)
                {
                    fieldOffset = i;
                    break;
                }
            }

            if (fieldOffset == -1)
            {
                // error
                return false;
            }

            op->arg1 = (declId << 16) | fieldOffset;
            op->arg2 = 1;
            return true;
        }
    }
    else
    {
        // error
        return false;
    }
}

bool read_vm_field(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, int* declId, int* fieldOffset)
{
    vm_decl_name decl;
    read_vm_identifier(input, *start, decl.name, sizeof(decl.name));
    *declId = find_vm_decl_const(decl, decls);
    if (*declId == -1)
    {
        // error
        return false;
    }

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_DOT)
    {
        // error
        return false;
    }

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_LITERAL)
    {
        // error
        return false;
    }

    char fieldname[VM_FIELD_MAX_NAME];
    read_vm_identifier(input, *start, fieldname, sizeof(fieldname));

    *fieldOffset = -1;
    vm_structure type = structs[decl.typeId];
    for (int i = 0; i < type.fieldcount; i++)
    {
        if (strcmp(fieldname, type.fields[i].name) == 0)
        {
            *fieldOffset = i;
            break;
        }
    }

    if (*fieldOffset == -1)
    {
        // error
        return false;
    }

    return true;
}

bool read_vm_binary_op(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op, vm_code code, int flags1, int flags2)
{
    vm_token store = *token;
    op->code = code;
    op->flags = 0;

    *start = *pos;
    get_next_token(input, pos, token);
    
    if ((flags1 & *token) == VM_TOKEN_NONE)
    {
        // error
        return false;
    }

    if (*token == VM_TOKEN_ADDRESS)
    {
        op->flags |= VM_OPERATION_FLAGS_REFERENCE1;

        *start = *pos;
        get_next_token(input, pos, token);
    }

    if (*token == VM_TOKEN_LOCATION1)
    {
        op->arg1 = 0;
    }
    else if (*token == VM_TOKEN_LOCATION2)
    {
        op->arg1 = 1;
    }
    else if (*token == VM_TOKEN_LOCATION3)
    {
        op->arg1 = 2;
    }
    else if (*token == VM_TOKEN_LOCATION4)
    {
        op->arg1 = 3;
    }
    else if (*token == VM_TOKEN_LITERAL)
    {
        int delcId;
        int field;
        bool res = read_vm_field(input, decls, structs, pos, start, token, &delcId, &field);
        if (!res)
        {
            // error
            return false;
        }

        op->flags |= VM_OPERATION_FLAGS_FIELD1;
        op->arg1 = (delcId << 16) | field;
    }
    else
    {
        // error
        return false;
    }

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_SEPERATOR)
    {
        // error
        return false;
    }

    *start = *pos;
    get_next_token(input, pos, token);

    if ((flags2 & *token) == VM_TOKEN_NONE)
    {
        // error
        return false;
    }

    if (*token == VM_TOKEN_ADDRESS)
    {
        op->flags |= VM_OPERATION_FLAGS_REFERENCE2;

        *start = *pos;
        get_next_token(input, pos, token);
    }

    if (*token == VM_TOKEN_LOCATION1)
    {
        op->arg2 = 0;
    }
    else if (*token == VM_TOKEN_LOCATION2)
    {
        op->arg2 = 1;
    }
    else if (*token == VM_TOKEN_LOCATION3)
    {
        op->arg2 = 2;
    }
    else if (*token == VM_TOKEN_LOCATION4)
    {
        op->arg2 = 3;
    }
    else if (*token == VM_TOKEN_LITERAL)
    {
        if (((op->flags & VM_OPERATION_FLAGS_FIELD1) == VM_OPERATION_FLAGS_FIELD1) ||
            ((op->flags & VM_OPERATION_FLAGS_REFERENCE1) == VM_OPERATION_FLAGS_REFERENCE1))
        {
            // error - cannot move between two memory locations
            return false;
        }

        int declId;
        int field;
        bool res = read_vm_field(input, decls, structs, pos, start, token, &declId, &field);
        if (!res)
        {
            // error
            return false;
        }

        op->flags |= VM_OPERATION_FLAGS_FIELD2;
        op->arg2 = (declId << 16) | field;
    }
    else
    {
        // error
        return false;
    }

    return true;
}

bool read_vm_binary_op(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op, vm_code code)
{
    return read_vm_binary_op(input, decls, structs, pos, start, token, op, code,
        VM_TOKEN_LOCATION1 | VM_TOKEN_LOCATION2 | VM_TOKEN_LOCATION3 | VM_TOKEN_LOCATION4 | VM_TOKEN_LITERAL | VM_TOKEN_ADDRESS,
        VM_TOKEN_LOCATION1 | VM_TOKEN_LOCATION2 | VM_TOKEN_LOCATION3 | VM_TOKEN_LOCATION4 | VM_TOKEN_LITERAL | VM_TOKEN_ADDRESS);
}

bool read_vm_add(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, decls, structs, pos, start, token, op, VM_ADD);
}

bool read_vm_sub(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, decls, structs, pos, start, token, op, VM_SUB);
}

bool read_vm_div(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, decls, structs, pos, start, token, op, VM_DIV);
}

bool read_vm_mul(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, decls, structs, pos, start, token, op, VM_MUL,
        VM_TOKEN_LOCATION1 | VM_TOKEN_LOCATION2 | VM_TOKEN_LOCATION3 | VM_TOKEN_LOCATION4 | VM_TOKEN_ADDRESS,
        VM_TOKEN_LOCATION1 | VM_TOKEN_LOCATION2 | VM_TOKEN_LOCATION3 | VM_TOKEN_LOCATION4 | VM_TOKEN_LITERAL | VM_TOKEN_ADDRESS);
}

bool read_vm_mov(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, decls, structs, pos, start, token, op, VM_MOV);
}

bool read_vm_lea(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, decls, structs, pos, start, token, op, VM_LEA, 
        VM_TOKEN_LOCATION1 | VM_TOKEN_LOCATION2 | VM_TOKEN_LOCATION3 | VM_TOKEN_LOCATION4,
        VM_TOKEN_LITERAL);
}

bool read_vm_cmp(const std::string& input, const std::vector<vm_decl_name>& decls, const std::vector<vm_structure>& structs, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, decls, structs, pos, start, token, op, VM_CMP);
}

void read_vm_structure_name(const std::string& input, int start, char* name)
{
    read_vm_identifier(input, start, name, VM_STRUCTURE_MAX_NAME);
}

void read_vm_field_name(const std::string& input, int start, char* name)
{
    read_vm_identifier(input, start, name, VM_FIELD_MAX_NAME);
}

int add_vm_decl(const vm_decl_name& decl, std::vector<vm_decl_name>& decls)
{
    int declId = -1;
    for (int i = 0; i < decls.size(); i++)
    {
        if (strcmp(decl.name, decls[i].name) == 0)
        {
            declId = i;
            break;
        }
    }

    decls.push_back(decl);
    return decls.size() - 1;
}

bool read_vm_declare(const std::vector<vm_structure>& structs, std::vector<vm_decl_name>& decls, const std::string& input, int* pos, int* start, vm_token* token)
{
    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_LITERAL)
    {
        // error
        return false;
    }

    // the type of the field
    char type[VM_STRUCTURE_MAX_NAME];
    read_vm_identifier(input, *start, type, sizeof(type));
    int typeId = -1;
    for (int i = 0; i < structs.size(); i++)
    {
        if (strcmp(type, structs[i].name) == 0)
        {
            typeId = i;
            break;
        }
    }

    if (typeId == -1)
    {
        // error - unknown type
        return false;
    }

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_COLON)
    {
        // error
        return false;
    }
    
    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_LITERAL)
    {
        // error
        return false;
    }

    // the name of the field
    vm_decl_name decl;
    decl.typeId = typeId;
    read_vm_identifier(input, *start, decl.name, sizeof(decl.name));
    int declId = add_vm_decl(decl, decls);

    return true;
}

bool read_vm_struct(const std::string& input, int* pos, int* start, vm_token* token, vm_structure* structure)
{
    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_LITERAL)
    {
        // error
        return false;
    }

    read_vm_structure_name(input, *start, structure->name);

    structure->fieldcount = 0;

    while (*token != VM_TOKEN_EOF)
    {
        *start = *pos;
        get_next_token(input, pos, token);
        if (*token == VM_TOKEN_FIELD)
        {
            *start = *pos;
            get_next_token(input, pos, token);
            if (*token == VM_TOKEN_LITERAL)
            {
                const char* ptr = input.c_str();
                int size = strtol(ptr + (*start), 0, 10);
                
                if (structure->fieldcount == VM_STRUCTURE_MAX_FIELD)
                {
                    // error
                    return false;
                }

                *start = *pos;
                get_next_token(input, pos, token);
                if (*token != VM_TOKEN_COLON)
                {
                    // error
                    return false;
                }

                *start = *pos;
                get_next_token(input, pos, token);
                if (*token != VM_TOKEN_LITERAL)
                {
                    // error
                    return false;
                }

                vm_field* field = &structure->fields[structure->fieldcount];
                field->size = size;
                field->array_size = 0;
                if (structure->fieldcount == 0)
                {
                    field->offset = 0;
                }
                else
                {
                    vm_field* prev = &structure->fields[structure->fieldcount - 1];
                    field->offset = prev->offset + prev->size;
                }
                read_vm_field_name(input, *start, field->name);
                structure->fieldcount++;
            }
        }
        else if (*token == VM_TOKEN_END)
        {
            return true;
        }
        else
        {
            // error
            return false;
        }
    }

    // error
    return false;
}

bool read_vm_label(std::vector<vm_label>& labels, const std::string& input, int* pos, int* start, vm_token* token, vm_operation* operation)
{
    int labelStart = *start;
    for (int i = *start; i < *pos; i++)
    {
        if (input.at(i) == '\n')
        {
            labelStart = i + 1;
        }
    }

    vm_label label;
    read_vm_identifier(input, labelStart, label.name, sizeof(label.name));

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_COLON)
    {
        // error
        return false;
    }

    int labelId = find_vm_label_const(label, labels);
    if (labelId == -1)
    {
        label.defined = 1;
        label.labelId = labels.size();
        labels.push_back(label);
    }
    else
    {
        labels.at(labelId).defined = 1;
    }

    operation->code = VM_LABEL;
    operation->arg1 = label.labelId;

    return true;
}

bool read_vm_jump(std::vector<vm_label>& labels, const std::string& input, int* pos, int* start, vm_token* token, vm_operation* operation)
{
    vm_code code;
    switch (*token)
    {
    case VM_TOKEN_JMP:
        code = VM_JMP;
        break;
    case VM_TOKEN_JMPLT:
        code = VM_JMPLT;
        break;
    case VM_TOKEN_JMPGT:
        code = VM_JMPGT;
        break;
    case VM_TOKEN_JMPEQ:
        code = VM_JMPEQ;
        break;
    }

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_LITERAL)
    {
        // error
        return false;
    }

    vm_label label;
    read_vm_identifier(input, *start, label.name, sizeof(label.name));
    int labelId = find_vm_label_const(label, labels);
    if (labelId == -1)
    {
        // label does not exist yet
        label.defined = 0;
        label.labelId = labels.size();
        labels.push_back(label);
    }

    operation->code = code;
    operation->arg1 = label.labelId;

    return true;
}

bool read_vm_proc(const std::vector<vm_structure>& structures, const std::string& input, int* pos, int* start, vm_token* token, vm_method* method)
{
    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_LITERAL)
    {
        // error
        return false;
    }

    read_vm_identifier(input, *start, method->name, sizeof(method->name));

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_BRACKET_OPEN)
    {
        // error
        return false;
    }

    method->paramcount = 0;

    *start = *pos;
    get_next_token(input, pos, token);

    bool cont = true;

    if (*token == VM_TOKEN_BRACKET_CLOSE)
    {
        cont = false;
    }

    while (cont)
    {
        if (*token != VM_TOKEN_LITERAL)
        {
            // error
            return false;
        }

        auto& parameter = method->parameters[method->paramcount];
        parameter.flags = VM_DECL_FLAGS_PARAMETER;

        // the type of the field
        char type[VM_STRUCTURE_MAX_NAME];
        read_vm_structure_name(input, *start, type);
        
        int isType = -1;
        for (int i = 0; i < structures.size(); i++)
        {
            auto& structure = structures.at(i);
            if (strcmp(structure.name, type) == 0)
            {
                isType = 1;
                parameter.typeId = i;
                break;
            }
        }

        if (isType == -1)
        {
            // error: no such type
            return false;
        }

        *start = *pos;
        get_next_token(input, pos, token);
        if (*token == VM_TOKEN_ADDRESS)
        {
            parameter.flags |= VM_DECL_FLAGS_REFERENCE;

            *start = *pos;
            get_next_token(input, pos, token);
        }
        
        if (*token != VM_TOKEN_COLON)
        {
            // error
            return false;
        }

        *start = *pos;
        get_next_token(input, pos, token);
        if (*token != VM_TOKEN_LITERAL)
        {
            // error
            return false;
        }
        
        read_vm_identifier(input, *start, parameter.name, sizeof(parameter.name));
        
        method->paramcount++;

        *start = *pos;
        get_next_token(input, pos, token);
        if (*token == VM_TOKEN_BRACKET_CLOSE)
        {
            cont = false;
        }
        else if (*token != VM_TOKEN_SEPERATOR)
        {
            // error
            return false;
        }
        else
        {
            // Next token
            *start = *pos;
            get_next_token(input, pos, token);
        }
    }

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_COLON)
    {
        // error
        return false;
    }

    return true;
}

bool vm_read_call(const std::vector<vm_method>& methods, const std::vector<vm_decl_name>& decls, const std::string& input, int* pos, int* start, vm_token* token, vm_operation* operation)
{
    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_LITERAL)
    {
        // error
        return false;
    }

    char methodname[VM_PROC_MAX_NAME];
    read_vm_identifier(input, *start, methodname, sizeof(methodname));

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token != VM_TOKEN_BRACKET_OPEN)
    {
        // error
        return false;
    }
    
    *start = *pos;
    get_next_token(input, pos, token);

    bool cont = true;

    if (*token == VM_TOKEN_BRACKET_CLOSE)
    {
        cont = false;
    }

    int packedArgs = 0;
    int parametercount = 0;

    while (cont)
    {
        if (*token != VM_TOKEN_LITERAL)
        {
            // error
            return false;
        }

        vm_decl_name parameter;
        read_vm_identifier(input, *start, parameter.name, sizeof(parameter.name));
        int id = find_vm_decl_const(parameter, decls);
        if (id == -1)
        {
            // error: undefined variable
            return false;
        }
        
        packedArgs |= (id & 0xff) << (parametercount * 8);
        parametercount++;

        *start = *pos;
        get_next_token(input, pos, token);
        if (*token == VM_TOKEN_BRACKET_CLOSE)
        {
            cont = false;
        }
        else if (*token != VM_TOKEN_SEPERATOR)
        {
            // error
            return false;
        }
        else
        {
            // Next token
            *start = *pos;
            get_next_token(input, pos, token);
        }
    }

    bool matched = false;
    vm_method matched_method;
    for (int i = 0; i < methods.size(); i++)
    {
        auto& method = methods.at(i);
        if (strcmp(method.name, methodname) == 0 && method.paramcount == parametercount)
        {
            // TODO: match the parameter types

            matched = true;
            matched_method = method;
            break;
        }
    }

    if (!matched)
    {
        // error: no such method
        return false;
    }

    operation->code = VM_CALL;
    operation->arg1 = matched_method.methodId;
    operation->arg2 = packedArgs;
    operation->flags = 0;

    return true;
}

void set_error(char* errorOutput, char* errorMsg, int maxErrorLength, const std::string& str, int pos, int end)
{
    std::stringstream stream;
    stream << errorMsg << " '" << str.substr(pos, end - pos) << "'";

    std::string result = stream.str();
    
    int len = result.size();
    if (len > maxErrorLength) len = maxErrorLength;
    memcpy(errorOutput, result.c_str(), len);
    errorOutput[len] = '\0';
}

bool load_file_and_execute(const char* filename, const vm_options& options, char* errorText, int errorLength)
{
    std::stack<vm_scope*> scope;

    vm_scope globalscope;
    vm_scope* localscope = &globalscope;
    localscope->flags = VM_SCOPE_FLAG_DECLARATION | VM_SCOPE_FLAG_LABEL | VM_SCOPE_FLAG_OPERATION | VM_SCOPE_FLAG_STRUCTURE | VM_SCOPE_FLAG_METHOD;
    scope.push(localscope);

    std::string str;
    std::ifstream stream;
    stream.open(filename, std::ios::in);
    if (!stream.fail() && !stream.eof())
    {
        str = std::move(std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()));
    }
    stream.close();

    bool success = true;
    vm_operation op;
    vm_token token;
    int pos = 0;
    int start;
    while (pos < str.length())
    {
        start = pos;

        get_next_token(str, &pos, &token);
        if (token == VM_TOKEN_STORE1 ||
            token == VM_TOKEN_STORE2 ||
            token == VM_TOKEN_STORE3 ||
            token == VM_TOKEN_STORE4)
        {
            if (read_vm_store(str, localscope->decls, globalscope.structures, &pos, &start, &token, &op))
            {
                localscope->operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing STORE", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_ADD)
        {
            if (read_vm_add(str, localscope->decls, globalscope.structures, &pos, &start, &token, &op))
            {
                localscope->operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing ADD", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_SUB)
        {
            if (read_vm_sub(str, localscope->decls, globalscope.structures, &pos, &start, &token, &op))
            {
                localscope->operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing SUB", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_DIV)
        {
            if (read_vm_div(str, localscope->decls, globalscope.structures, &pos, &start, &token, &op))
            {
                localscope->operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing DIV", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_MUL)
        {
            if (read_vm_mul(str, localscope->decls, globalscope.structures, &pos, &start, &token, &op))
            {
                localscope->operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing MUL", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_MOV)
        {
            if (read_vm_mov(str, localscope->decls, globalscope.structures, &pos, &start, &token, &op))
            {
                localscope->operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing MOV", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_LEA)
        {
            if (read_vm_lea(str, localscope->decls, globalscope.structures, &pos, &start, &token, &op))
            {
                localscope->operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing LEA", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_CMP)
        {
            if (read_vm_cmp(str, localscope->decls, globalscope.structures, &pos, &start, &token, &op))
            {
                localscope->operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing CMP", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_STRUCTURE)
        {
            if ((localscope->flags & VM_SCOPE_FLAG_STRUCTURE) == VM_SCOPE_FLAG_NONE)
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing STRUCTURE", errorLength, str, start, pos);
                break;
            }

            vm_structure structure;
            if (read_vm_struct(str, &pos, &start, &token, &structure))
            {
                localscope->structures.push_back(structure);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing STRUCTURE", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_DECLARE)
        {
            if ((localscope->flags & VM_SCOPE_FLAG_DECLARATION) == VM_SCOPE_FLAG_NONE)
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing DECLARE", errorLength, str, start, pos);
                break;
            }

            if (!read_vm_declare(globalscope.structures, localscope->decls, str, &pos, &start, &token))
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing DECLARE", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_PROC)
        {
            if ((localscope->flags & VM_SCOPE_FLAG_METHOD) == VM_SCOPE_FLAG_NONE)
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing PROC", errorLength, str, start, pos);
                break;
            }

            vm_method method;
            if (read_vm_proc(globalscope.structures, str, &pos, &start, &token, &method))
            {
                method.scope = new vm_scope(); // create a new scope
                method.methodId = localscope->methods.size();
                localscope->methods.push_back(method);

                // initialize the new scope
                vm_scope* procScope = method.scope;
                procScope->flags = VM_SCOPE_FLAG_LABEL | VM_SCOPE_FLAG_DECLARATION | VM_SCOPE_FLAG_OPERATION;
                localscope->children.push_back(procScope);
                scope.push(localscope);
                localscope = procScope;

                // push all the parameters into the localscope
                for (int i = 0; i < method.paramcount; i++)
                {
                    localscope->decls.push_back(method.parameters[i]);
                }
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing PROC", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_RETURN)
        {
            vm_operation operation;
            operation.code = VM_RETURN;
            operation.arg1 = 0;
            operation.arg2 = 0;
            operation.flags = 0;
            localscope->operations.push_back(operation);

            localscope = scope.top();
            scope.pop();
        }
        else if (token == VM_TOKEN_CALL)
        {
            vm_operation operation;
            if (vm_read_call(globalscope.methods, localscope->decls, str, &pos, &start, &token, &operation))
            {
                localscope->operations.push_back(operation);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing CALL", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_JMP ||
            token == VM_TOKEN_JMPEQ ||
            token == VM_TOKEN_JMPGT ||
            token == VM_TOKEN_JMPLT)
        {
            vm_operation operation;
            if (read_vm_jump(localscope->labels, str, &pos, &start, &token, &operation))
            {
                localscope->operations.push_back(operation);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing JUMP", errorLength, str, start, pos);
                break;
            }
        }
        else if (token == VM_TOKEN_LITERAL)
        {
            if ((localscope->flags & VM_SCOPE_FLAG_LABEL) == VM_SCOPE_FLAG_NONE)
            {
                success = false;
                set_error(errorText, "Error: Unexpected token", errorLength, str, start, pos);
                break;
            }

            vm_operation operation;
            if (read_vm_label(localscope->labels, str, &pos, &start, &token, &operation))
            {
                localscope->operations.push_back(operation);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token", errorLength, str, start, pos);
                break;
            }
        }
        else if (token != VM_TOKEN_EOF)
        {
            success = false;
            set_error(errorText, "Error: Unexpected token", errorLength, str, start, pos);
            break;
        }
    }

    if (success)
    {
        vm_execute(globalscope, options);
    }

    return success;
}
