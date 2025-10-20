#ifndef SHARED_LIB_TEMPLATE_HPP
#define SHARED_LIB_TEMPLATE_HPP

#include <string>

std::string inline sharedLibHeadderTemplate(std::string name)
{
    return
R"(#ifndef )" + name + R"(_HPP
#define )" + name + R"(_HPP

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef BUILD_DLL
    #define )" + name + R"( __declspec(dllexport)
#else
    #define )" + name + R"( __declspec(dllimport)
#endif

// functions here

#ifdef __cplusplus
    }
#endif

// or here

#endif)";
}

std::string inline sharedLibTemplate(std::string name)
{
    return
R"(#include ")" + name + R"(.h"
)";
}

#endif