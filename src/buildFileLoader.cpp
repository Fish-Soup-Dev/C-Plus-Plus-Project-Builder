#include "buildFileLoader.hpp"

#include "utils.hpp"

#include <iostream>
#include <filesystem>

BuildFileLoader::BuildFileLoader() : m_isFile(true)
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    m_file = findBuildFile(currentPath);

    if (m_file.empty())
    {
        m_isFile = false;
    }
    else
    {
        m_data = toml::parse(m_file, toml::spec::v(1,1,0));
    }
}

BuildFileLoader::~BuildFileLoader()
{

}

bool BuildFileLoader::isFound()
{
    return m_isFile;
}

std::string BuildFileLoader::getBinPath()
{
    return toml::find<std::string>(m_data, "paths", "bin");
}

std::string BuildFileLoader::getObjPath()
{
    return toml::find<std::string>(m_data, "paths", "obj");
}

std::string BuildFileLoader::getName()
{
    std::string name = toml::find_or<std::string>(m_data, "project", "name", "");

    if (name.empty())
    {
        throw std::runtime_error("build.toml project name value missing");
    }

    return name;
}

std::string BuildFileLoader::getType()
{
    std::string type = toml::find_or<std::string>(m_data, "project", "type", "");

    if (type.empty())
    {
        throw std::runtime_error("build.toml type value missing");
    }

    if (type != "program" && type != "shared" && type != "static")
    {
        throw std::runtime_error("build.toml type is incorrect");
    }

    return type;
}

std::string BuildFileLoader::getCompiler()
{
    std::string cc = toml::find_or<std::string>(m_data, "compiler", "cc", "");

    if (cc.empty())
    {
        throw std::runtime_error("build.toml compiler value missing");
    }

    return cc;
}

std::string BuildFileLoader::getSrcPath()
{
    return toml::find<std::string>(m_data, "paths", "src");
}

std::string BuildFileLoader::getIncludePath()
{
    // if (!std::filesystem::exists(includePath))
    // {
    //     std::cout << color(Red) << includePath << " directory not found" << color(Default) << std::endl;
    //     exit(EXIT_FAILURE);
    // }

    return toml::find<std::string>(m_data, "paths", "include");
}

std::string BuildFileLoader::getLibPath()
{
    return toml::find<std::string>(m_data, "paths", "lib");
}

std::vector<std::string> BuildFileLoader::getLdFlags()
{
    return toml::find<std::vector<std::string>>(m_data, "compiler", "ldflags");
}

std::vector<std::string> BuildFileLoader::getLibs(bool isWindows)
{
    if (isWindows)
    {
        return toml::find<std::vector<std::string>>(m_data, "compiler", "windows", "libs");
    }
    else
    {
        return toml::find<std::vector<std::string>>(m_data, "compiler", "linux", "libs");
    }
}

std::vector<std::string> BuildFileLoader::getDefsRelease()
{
    return toml::find<std::vector<std::string>>(m_data, "compiler", "release", "cdefs");
}

std::vector<std::string> BuildFileLoader::getDefsDebug()
{
    return toml::find<std::vector<std::string>>(m_data, "compiler", "debug", "cdefs");
}

std::vector<std::string> BuildFileLoader::getFlagsRelease()
{
    return toml::find<std::vector<std::string>>(m_data, "compiler", "release", "cflags");
}

std::vector<std::string> BuildFileLoader::getFlagsDebug()
{
    return toml::find<std::vector<std::string>>(m_data, "compiler", "debug", "cflags");
}

std::string BuildFileLoader::getVersion()
{
    m_data = toml::parse(m_file, toml::spec::v(1,1,0));
    return toml::find<std::string>(m_data, "project", "version");
}

// version format is major.minor.patch
void BuildFileLoader::bumpPatchVersion()
{
    std::string version = getVersion();
    size_t firstDot = version.find('.');
    size_t secondDot = version.find('.', firstDot + 1);

    if (firstDot == std::string::npos || secondDot == std::string::npos)
    {
        throw std::runtime_error("Version format is incorrect");
    }

    int major = std::stoi(version.substr(0, firstDot));
    int minor = std::stoi(version.substr(firstDot + 1, secondDot - firstDot - 1));
    int patch = std::stoi(version.substr(secondDot + 1));

    patch++;

    version = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);

    // update the version in the toml data
    toml::find(m_data, "project")["version"] = version;

    // write the updated toml data back to the file
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path file = findBuildFile(currentPath);
    std::ofstream ofs(file);
    ofs << m_data;
    ofs.close();
}

void BuildFileLoader::bumpMinorVersion()
{
    std::string version = getVersion();
    size_t firstDot = version.find('.');
    size_t secondDot = version.find('.', firstDot + 1);

    if (firstDot == std::string::npos || secondDot == std::string::npos)
    {
        throw std::runtime_error("Version format is incorrect");
    }

    int major = std::stoi(version.substr(0, firstDot));
    int minor = std::stoi(version.substr(firstDot + 1, secondDot - firstDot - 1));
    int patch = std::stoi(version.substr(secondDot + 1));

    minor++;
    patch = 0;

    version = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);

    // update the version in the toml data
    toml::find(m_data, "project")["version"] = version;

    // write the updated toml data back to the file
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path file = findBuildFile(currentPath);
    std::ofstream ofs(file);
    ofs << m_data;
    ofs.close();
}

void BuildFileLoader::bumpMajorVersion()
{
    std::string version = getVersion();
    size_t firstDot = version.find('.');
    size_t secondDot = version.find('.', firstDot + 1);

    if (firstDot == std::string::npos || secondDot == std::string::npos)
    {
        throw std::runtime_error("Version format is incorrect");
    }

    int major = std::stoi(version.substr(0, firstDot));
    int minor = std::stoi(version.substr(firstDot + 1, secondDot - firstDot - 1));
    int patch = std::stoi(version.substr(secondDot + 1));

    major++;
    minor = 0;
    patch = 0;

    version = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);

    // update the version in the toml data
    toml::find(m_data, "project")["version"] = version;

    // write the updated toml data back to the file
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path file = findBuildFile(currentPath);
    std::ofstream ofs(file);
    ofs << m_data;
    ofs.close();
}