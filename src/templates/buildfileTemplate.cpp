#include "buildfileTemplate.hpp"

std::string buildfile_template
(
    const std::string projectName,
    const std::string projectType, 
    const std::string cppFlagsRelease,
    const std::string cppFlagsDebug,
    const std::string cppDefinesRelease,
    const std::string cppDefinesDebug
)
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