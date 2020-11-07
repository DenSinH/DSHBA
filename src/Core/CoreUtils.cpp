#include "include/CoreUtils.h"

#include "log.h"

#include <fstream>

size_t LoadFileTo(char* buffer, const std::string& file_name, size_t max_length) {
    std::ifstream infile(file_name, std::ios::binary);

    if (infile.fail()) {
        log_fatal("Failed to open file %s", file_name.c_str());
    }

    infile.seekg(0, std::ios::end);
    size_t length = infile.tellg();
    infile.seekg(0, std::ios::beg);

    if (length > max_length) {
        log_fatal("Failed loading file %s, buffer overflow: %x > %x", file_name.c_str(), length, max_length);
    }

    infile.read(buffer, length);
    log_info("Loaded %x bytes from file %s", length, file_name.c_str());
    return length;
}