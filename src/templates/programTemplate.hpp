#ifndef PROGRAM_TEMPLATE_HPP
#define PROGRAM_TEMPLATE_HPP

#include <string>

std::string inline programTemplate()
{
    return 
R"(#include <iostream>

int main(int argc, char *argv[])
{
    std::cout << "it works" << std::endl;
    return 0;
})";
}

#endif