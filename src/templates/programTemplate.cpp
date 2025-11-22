#include "programTemplate.hpp"

std::string programTemplate()
{
    return 
R"(#include <iostream>

int main(int argc, char *argv[])
{
    std::cout << "it works" << std::endl;
    return 0;
})";
}