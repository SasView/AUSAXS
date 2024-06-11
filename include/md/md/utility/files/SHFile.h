#pragma once

#include <md/utility/files/File.h>

namespace md {
    // Binary run input file
    struct SHFile : public detail::File {
        SHFile() = default;
        SHFile(const std::string& name) : File(name, "sh") {}
        SHFile(const char* name) : SHFile(std::string(name)) {}
        ~SHFile() override = default;
    };
}