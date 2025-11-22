#include "staticLibTemplate.hpp"

std::string staticLibHeaderTemplate(const std::string name)
{
    return
R"(#ifndef )" + name + R"(_HPP
#define )" + name + R"(_HPP

#endif)";
}

std::string staticLibTemplate(const std::string name)
{
    return
R"(#include ")" + name + R"(.h"
)";
}