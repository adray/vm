#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <streambuf>
#include "vm.h"

enum vm_token
{
    VM_TOKEN_NONE = 0x0,
    VM_TOKEN_ADD = 0x1,
    VM_TOKEN_SUB = 0x2,
    VM_TOKEN_MUL = 0x3,
    VM_TOKEN_DIV = 0x4,
    VM_TOKEN_LOAD1 = 0x10,
    VM_TOKEN_LOAD2 = 0x11,
    VM_TOKEN_LOAD3 = 0x12,
    VM_TOKEN_LOAD4 = 0x13,
    VM_TOKEN_STORE1 = 0x20,
    VM_TOKEN_STORE2 = 0x21,
    VM_TOKEN_STORE3 = 0x22,
    VM_TOKEN_STORE4 = 0x23,
    VM_TOKEN_LOCATION1 = 0x90,
    VM_TOKEN_LOCATION2 = 0x91,
    VM_TOKEN_LOCATION3 = 0x92,
    VM_TOKEN_LOCATION4 = 0x93,
    VM_TOKEN_LITERAL = 0x100,
    VM_TOKEN_SEPERATOR = 0x101,
    VM_TOKEN_EOF = 0x1000
};

void get_next_token(const std::string& line, int* pos, vm_token* token)
{
    if (line[*pos] == ',')
    {
        *token = VM_TOKEN_SEPERATOR;
        (*pos)++;
        return;
    }

    for (; *pos < line.size(); (*pos)++)
    {
        if (line[*pos] != ' ')
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
        else if (line[*pos] == ';')
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
    else
    {
        *token = VM_TOKEN_LITERAL;
    }
}

bool read_vm_store(const std::string& input, int* pos, int* start, vm_token* token, vm_operation* op)
{
    vm_token store = *token;

    *start = *pos;
    get_next_token(input, pos, token);
    if (*token == VM_TOKEN_LITERAL)
    {
        op->code = (vm_code)store; // kinda hack
        const char* ptr = input.c_str();
        op->arg1 = strtol(ptr + (*start), 0, 10);
        op->arg2 = 0;
        return true;
    }
    else
    {
        // error
        return false;
    }
}

bool read_vm_binary_op(const std::string& input, int* pos, int* start, vm_token* token, vm_operation* op, vm_code code)
{
    vm_token store = *token;
    op->code = code;

    *start = *pos;
    get_next_token(input, pos, token);
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
    else
    {
        // error
        return false;
    }

    return true;
}

bool read_vm_add(const std::string& input, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, pos, start, token, op, VM_ADD);
}

bool read_vm_sub(const std::string& input, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, pos, start, token, op, VM_SUB);
}

bool read_vm_div(const std::string& input, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, pos, start, token, op, VM_DIV);
}

bool read_vm_mul(const std::string& input, int* pos, int* start, vm_token* token, vm_operation* op)
{
    return read_vm_binary_op(input, pos, start, token, op, VM_MUL);
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
    std::vector<vm_operation> operations;

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
        if (token == VM_TOKEN_LOAD1 ||
            token == VM_TOKEN_LOAD2 ||
            token == VM_TOKEN_LOAD3 ||
            token == VM_TOKEN_LOAD4)
        {

        }
        else if (token == VM_TOKEN_STORE1 ||
            token == VM_TOKEN_STORE2 ||
            token == VM_TOKEN_STORE3 ||
            token == VM_TOKEN_STORE4)
        {
            if (read_vm_store(str, &pos, &start, &token, &op))
            {
                operations.push_back(op);
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
            if (read_vm_add(str, &pos, &start, &token, &op))
            {
                operations.push_back(op);
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
            if (read_vm_sub(str, &pos, &start, &token, &op))
            {
                operations.push_back(op);
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
            if (read_vm_div(str, &pos, &start, &token, &op))
            {
                operations.push_back(op);
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
            if (read_vm_mul(str, &pos, &start, &token, &op))
            {
                operations.push_back(op);
            }
            else
            {
                success = false;
                set_error(errorText, "Error: Unexpected token parsing MUL", errorLength, str, start, pos);
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
        vm_execute(operations, options);
    }

    return success;
}
