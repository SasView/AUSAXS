#pragma once

#include <md/utility/files/File.h>

namespace md {
    // Binary run input file
    struct TPRFile : public detail::File {
        TPRFile() = default;
        TPRFile(const std::string& name) : File(name, "tpr") {}
        TPRFile(const char* name) : TPRFile(std::string(name)) {}
        ~TPRFile() override = default;
    };
}