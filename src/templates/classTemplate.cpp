#include "classTemplate.hpp"

std::string classHeaderTemplate(const std::string classNameUpper, const std::string className)
{
    return
R"(#ifndef )" + classNameUpper + R"(_HPP
#define )" + classNameUpper + R"(_HPP

class )" + className + R"(
{
public:
    )" + className + R"(();

    ~)" + className + R"(();

    // more stuff here

private:
    // stuff go here

};
#endif)";
}

std::string classTemplate(const std::string className)
{
    return
R"(#include ")" + className + R"(.h"

)" + className + R"(::)" + className + R"(()
{

}

)" + className + R"(::~)" + className + R"(()
{

}

)";
}
