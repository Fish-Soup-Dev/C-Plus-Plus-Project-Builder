#ifndef BUILDFILELOADER_HPP
#define BUILDFILELOADER_HPP

#include <filesystem>

#include "toml11/toml.hpp"

class BuildFileLoader
{
public:
    BuildFileLoader();
    ~BuildFileLoader();

    bool isFound();

    std::string getBinPath();
    std::string getObjPath();
    std::string getName();
    std::string getType();
    std::string getCompiler();
    std::string getSrcPath();
    std::string getIncludePath();
    std::string getLibPath();

    std::vector<std::string> getLdFlags();
    std::vector<std::string> getLibs();
    std::vector<std::string> getDefsRelease();
    std::vector<std::string> getDefsDebug();
    std::vector<std::string> getFlagsRelease();
    std::vector<std::string> getFlagsDebug();

private:
    bool m_isFile;
    toml::value m_data;

    std::filesystem::path findBuildFile(const std::filesystem::path currentPath)
    {
        std::filesystem::path buildFile;

        for (const auto& entry : std::filesystem::directory_iterator(currentPath))
        {
            if (!entry.is_directory()  && entry.path().filename().string() == "build.toml")
            {
                buildFile = entry.path();
                break;
            }
        }

        return buildFile;
    }
};

#endif