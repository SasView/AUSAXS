#include <utility/Utility.h>
#include <algorithm>
#include <filesystem>

#include <utility/Settings.h>

bool utility::approx(double v1, double v2, double abs, double eps) {
    if (v1-abs > v2*(1+eps)) {return false;}
    if (v1+abs < v2*(1-eps)) {return false;}
    return true;
}

std::string utility::remove_spaces(std::string s) {
    std::string::iterator end_pos = std::remove(s.begin(), s.end(), ' ');
    s.erase(end_pos, s.end());
    return s;
}

void utility::print_warning(std::string text) {
    std::cout << "\033[1;31m" << text << "\033[0m" << std::endl;
}

void utility::create_directories(std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    if (!p.has_extension()) {
        path += "." + setting::figures::format;
    }
}

bool utility::equal(double a, double b, double c) {
    return a == b && b == c;
}

std::string utility::remove_extension(std::string path) {
    return std::filesystem::path(path).replace_extension("");
}

std::string utility::stem_append(std::string path, std::string s) {
    std::filesystem::path p(path);
    return p.parent_path().string() + "/" + p.stem().string() + s + p.extension().string();
}

std::string utility::stem(std::string path) {
    return std::filesystem::path(path).stem();    
}

template<>
std::string utility::extract_number<std::string>(std::string s) {
    unsigned int start = 0;
    while (!std::isdigit(s[start]) && start != s.size()) {start++;}
    unsigned int end = start;
    while ((std::isdigit(s[end]) || s[end] == '.') && end != s.size()) {end++;}
    while (end > 0 && s[end-1] == '.') {end--;}
    return s.substr(start, end-start);
}

std::vector<std::string> utility::split(std::string str, char delimiter) {
    std::string token;
    std::stringstream ss(str);
    std::vector<std::string> tokens;
    while(std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> utility::split(std::string str, std::string delimiters) {
    std::vector<std::string> tokens;

    auto is_delimiter = [&delimiters] (char c) {
        return delimiters.find(c) != std::string::npos;
    };

    // skip leading delimiters
    unsigned int start = 0;
    for (; start < str.size(); start++) {
        if (!is_delimiter(str[start])) {
            break;
        }
    }

    // iterate through the rest of the string
    for (unsigned int i = start; i < str.size(); i++) {
        if (!is_delimiter(str[i])) {
            continue;
        }

        // add token to vector
        tokens.push_back(str.substr(start, i-start));
        start = ++i;

        // skip consecutive delimiters
        for (; start < str.size(); start++) {
            if (!is_delimiter(str[start])) {
                break;
            }
        }
        i = start;
    }

    // add last token to vector
    if (start < str.size()) {
        tokens.push_back(str.substr(start));
    }
    return tokens;
}

std::string utility::join(std::vector<std::string> v, std::string separator) {
    std::string s;
    for (unsigned int i = 0; i < v.size(); i++) {
        s += v[i];
        if (i != v.size()-1) {
            s += separator;
        }
    }
    return s;
}

std::string utility::remove_all(std::string s, std::string remove) {
    std::string new_s;
    for (auto c : s) {
        if (remove.find(c) == std::string::npos) {
            new_s += c;
        }
    }
    return new_s;
}

std::string utility::uid() {
    static unsigned int i = 0;
    return std::to_string(i++);
}

std::string utility::uid(std::string s) {return s + uid();}