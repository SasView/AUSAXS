#pragma once

#include <string>

namespace utility {
    /**
     * @brief Check if two numbers are approximately equal. 
     * 
     * @param v1 First value.
     * @param v2 Second value. 
     * @param abs Absolute tolerance. 
     * @param eps Relative tolerance. 
     */
    bool approx(double v1, double v2, double abs = 1e-6, double eps = 0.01);

    /**
     * @brief Remove spaces from both ends of a string. 
     *        Note that the input string is modified. 
     */
    // std::string remove_spaces(std::string s) {
    //     std::string::iterator end_pos = std::remove(s.begin(), s.end(), ' ');
    //     s.erase(end_pos, s.end());
    //     return s;
    // }

    /**
     * @brief Create all parent directories of the path.
     */
    void create_directories(std::string& path);

    /**
     * @brief Print a warning message. The text will be red in the terminal. 
     */
    void print_warning(std::string text);
}