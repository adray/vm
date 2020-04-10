#pragma once

struct vm_options;

bool load_file_and_execute(const char* filename, const vm_options& options, char* errorText, int errorLength);
