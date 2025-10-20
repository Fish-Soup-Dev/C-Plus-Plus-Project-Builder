#ifndef STATIC_LIB_TEMPLATE_HPP
#define STATIC_LIB_TEMPLATE_HPP

#include <string>

std::string inline staticLibHeadderTemplate(std::string name)
{
    return
R"(#ifndef )" + name + R"(_HPP
#define )" + name + R"(_HPP

#endif)";
}

std::string inline staticLibTemplate(std::string name)
{
    return
R"(#include ")" + name + R"(.h"
)";
}

#endif