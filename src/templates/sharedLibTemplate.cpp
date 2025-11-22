#include "sharedLibTemplate.hpp"

std::string sharedLibHeaderTemplate(const std::string name)
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

std::string sharedLibTemplate(const std::string name)
{
    return
R"(#include ")" + name + R"(.h"
)";
}
