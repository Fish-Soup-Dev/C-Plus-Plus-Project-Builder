#define COLOR_HPP
#ifdef COLOR_HPP

#include <string>

enum Colors
{
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Defult = 0,
};

std::string color(enum Colors color)
{
    return "\033[" + std::to_string(color) + "m";
}

#endif