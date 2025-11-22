#ifndef COLOR_HPP
#define COLOR_HPP

#include <string>

enum COLORS
{
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Gray = 90,
    Default = 0,
};

std::string inline color(enum COLORS color, bool bold = false)
{
    if (bold)
        return "\033[1;" + std::to_string(color) + "m";
    else
        return "\033[0;" + std::to_string(color) + "m";
}

#endif