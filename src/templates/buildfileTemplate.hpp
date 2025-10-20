#ifndef BUILDFILE_TEMPLATE_HPP
#define BUILDFILE_TEMPLATE_HPP

#include <string>

std::string inline buildfile_template(
    std::string projectName,
    std::string projectType, 
    std::string cppFlagsRelease,
    std::string cppFlagsDebug,
    std::string cppDefinesRelease,
    std::string cppDefinesDebug)
{
    return
R"([project]
name = ")" + projectName + R"("
type = ")" + projectType + R"("

[compiler]
cc = "g++"
ldflags = []
libs = []

[compiler.release]
cflags = [)" + cppFlagsRelease + R"(]
cdefs = [)" + cppDefinesRelease + R"(]

[compiler.debug]
cflags = [)" + cppFlagsDebug + R"(]
cdefs = [)" + cppDefinesDebug + R"(]

[paths]
src = "./src"
include = "./include"
lib = "./lib"
bin = "./bin"
obj = "./obj"

)";
}

#endif