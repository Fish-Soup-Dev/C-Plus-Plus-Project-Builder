#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <iostream>

namespace utils
{
    enum COLORS
    {
        Red = 31,
        Green = 32,
        Yellow = 33,
        Blue = 34,
        Gray = 90,
        Default = 0,
    };

    std::string inline setTextColor(enum COLORS color, bool bold = false)
    {
        if (bold)
            return "\033[1;" + std::to_string(color) + "m";
        else
            return "\033[0;" + std::to_string(color) + "m";
    }

    void inline printLine(const std::string& text, enum COLORS color, bool bold = false)
    {
        std::cout << setTextColor(color, bold) << text << setTextColor(Default) << std::endl;
    }

    void inline printError(const std::string& text, enum COLORS color, bool bold = false)
    {
        std::cerr << setTextColor(color, bold) << text << setTextColor(Default) << std::endl;
    }
}

#endif