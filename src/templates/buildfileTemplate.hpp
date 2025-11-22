#ifndef BUILDFILE_TEMPLATE_HPP
#define BUILDFILE_TEMPLATE_HPP

#include <string>

std::string buildfile_template
(
    const std::string projectName,
    const std::string projectType, 
    const std::string cppFlagsRelease,
    const std::string cppFlagsDebug,
    const std::string cppDefinesRelease,
    const std::string cppDefinesDebug
);

#endif