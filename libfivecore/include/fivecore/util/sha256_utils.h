//
// Created by wufan on 2026/9/20.
//

#ifndef FIVECLOCK_SHA256_UTILS_H
#define FIVECLOCK_SHA256_UTILS_H
#include <string>

namespace fivecore::util
{
    std::string sha256_string(const std::string& input);
    std::string sha256_file(const std::string& file_path);
}

#endif //FIVECLOCK_SHA256_UTILS_H
