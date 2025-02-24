#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <ctime>
#include <map>
#include <iomanip>
#include <windows.h>
#include <unistd.h>

#include "toml11/toml.hpp"

#include "color.hpp"

#define VERSION "0.3.0"

std::chrono::system_clock::time_point fileLastWriteTime(const std::wstring& filePath)
{
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error opening file: " << GetLastError() << std::endl;
        return std::chrono::system_clock::time_point::min();  // Return a minimum time point
    }

    FILETIME creationTime, lastAccessTime, lastWriteTime;

    if (GetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime))
    {
        // Convert FILETIME to SYSTEMTIME
        SYSTEMTIME lastWriteSysTime;
        FileTimeToSystemTime(&lastWriteTime, &lastWriteSysTime);

        // Convert SYSTEMTIME to system_clock::time_point
        std::tm tm = {};
        tm.tm_year = lastWriteSysTime.wYear - 1900;
        tm.tm_mon = lastWriteSysTime.wMonth - 1;
        tm.tm_mday = lastWriteSysTime.wDay;
        tm.tm_hour = lastWriteSysTime.wHour;
        tm.tm_min = lastWriteSysTime.wMinute;
        tm.tm_sec = lastWriteSysTime.wSecond;

        std::time_t time = std::mktime(&tm);
        CloseHandle(hFile);
        return std::chrono::system_clock::from_time_t(time);
    }
    else
    {
        std::cerr << "Error getting file times: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return std::chrono::system_clock::time_point::min();  // Return a minimum time point
    }
}

std::filesystem::path findBuildFile(std::filesystem::path currentPath)
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

int compileObject(
    const std::string cc, 
    const std::vector<std::string> cflags, 
    const std::vector<std::string> cdefs, 
    const std::string objectFile, 
    const std::string sourceFile, 
    const std::string includePath
)
{
    std::string command = cc;

    for (const auto& cflag : cflags)
        command.append(" " + cflag);

    command.append(" -c -o " + objectFile + " " + sourceFile);

    for (const auto& cdef : cdefs)
        command.append(" " + cdef);

    command.append(" -I" + includePath);

    auto start = std::chrono::high_resolution_clock::now();

    int result = std::system(command.c_str());

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;

    if (result == EXIT_SUCCESS)
        std::cout << color(Gray) << objectFile << " built in " << std::fixed << std::setprecision(3) << duration.count() << "s" << color(Defult) << std::endl;
    else
    {
        std::cout << color(Red) << objectFile << " Failed." << color(Defult) << std::endl;
        return false;
    }

    return true;
}

int compileBinarry(
    const std::string cc, 
    const std::vector<std::string> cflags, 
    const std::vector<std::string> cdefs, 
    const std::vector<std::string> objFiles, 
    const std::vector<std::string> libFiles, 
    const std::vector<std::string> libs, 
    const std::string main,
    const std::string includePath,
    const std::string libPath
)
{
    std::string command = cc;

    for (const auto& cflag : cflags)
        command.append(" " + cflag);

    command.append(" -o " + main);

    for (const auto& obj : objFiles)
        command.append(" " + obj);

    command.append(" -I" + includePath);
    command.append(" -L" + libPath);

    for (const auto& lib : libFiles)
        command.append(" " + lib);

    for (const auto& cdef : cdefs)
        command.append(" " + cdef);

    for (const auto& lib : libs)
        command.append(" " + lib);

    auto start = std::chrono::high_resolution_clock::now();
    
    int result = std::system(command.c_str());

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;

    if (result == EXIT_SUCCESS)
        std::cout << color(Gray) << main << " built in " << std::fixed << std::setprecision(3) << duration.count() << "s" << color(Defult) << std::endl;
    else
    {
        std::cout << color(Red) << main << " Failed." << color(Defult) << std::endl;
        return false;
    }

    return true;
}

int archiveStatic(
    const std::vector<std::string> objFiles, 
    const std::string main
)
{
    std::string command = "ar rcs ";

    command.append(main);

    for (const auto& obj : objFiles)
        command.append(" " + obj);

    auto start = std::chrono::high_resolution_clock::now();
    
    int result = std::system(command.c_str());

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;

    if (result == EXIT_SUCCESS)
        std::cout << color(Gray) << main << " packed in " << std::fixed << std::setprecision(3) << duration.count() << "s" << color(Defult) << std::endl;
    else
    {
        std::cout << color(Red) << main << " Failed." << color(Defult) << std::endl;
        return false;
    }

    return true;
}

void clean()
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path file = findBuildFile(currentPath);

    if (file.empty())
    {
        std::cout << color(Red) << "No build.toml found" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    auto data = toml::parse(file, toml::spec::v(1,1,0));

    std::string binPath = toml::find<std::string>(data, "paths", "bin");
    std::string objPath = toml::find<std::string>(data, "paths", "obj");

    std::filesystem::remove_all(binPath);
    std::filesystem::remove_all(objPath);

    std::cout << color(Green) << "Project cleaned" << color(Defult) << std::endl;
}

void build(std::string option)
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path file = findBuildFile(currentPath);

    if (file.empty())
    {
        std::cout << color(Red) << "No build.toml found" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    auto data = toml::parse(file, toml::spec::v(1,1,0));

    std::string name = toml::find_or<std::string>(data, "project", "name", "");

    if (name.empty())
    {
        std::cout << color(Red) << "build.toml name value missing" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string type = toml::find_or<std::string>(data, "project", "type", "");

    if (type.empty())
    {
        std::cout << color(Red) << "build.toml name value missing" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (type != "executable" && type != "dll" && type != "lib")
    {
        std::cout << color(Red) << "build.toml type is incorect" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string cc = toml::find_or<std::string>(data, "compiler", "cc", "");

    if (cc.empty())
    {
        std::cout << color(Red) << "build.toml name value missing" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    // libs and flags

    std::vector<std::string> ldflags = toml::find<std::vector<std::string>>(data, "compiler", "ldflags");
    std::vector<std::string> libs = toml::find<std::vector<std::string>>(data, "compiler", "libs");

    // object and bin paths

    std::string binPath = toml::find<std::string>(data, "paths", "bin");
    std::string objPath = toml::find<std::string>(data, "paths", "obj");

    std::vector<std::string> cdefs;
    std::vector<std::string> cflags;

    if (option == "release")
    {
        cdefs = toml::find<std::vector<std::string>>(data, "compiler", "release", "cdefs");
        cflags = toml::find<std::vector<std::string>>(data, "compiler", "release", "cflags");
        binPath += "/RELEASE";
        objPath += "/RELEASE";
    }
    else // debug
    {
        cdefs = toml::find<std::vector<std::string>>(data, "compiler", "debug", "cdefs");
        cflags = toml::find<std::vector<std::string>>(data, "compiler", "debug", "cflags");
        binPath += "/DEBUG";
        objPath += "/DEBUG";
    }

    if (!std::filesystem::exists(binPath))
    {
        std::cout << color(Gray) << binPath << " directory not found. Creating..." << color(Defult) << std::endl;
        std::filesystem::create_directories(binPath);
    }
    
    std::map<std::string, std::chrono::_V2::system_clock::time_point> objTime;

    if (!std::filesystem::exists(objPath))
    {
        std::cout << color(Gray) << objPath << " directory not found. Creating..." << color(Defult) << std::endl;
        std::filesystem::create_directories(objPath);
    }
    else // get when obj files where edited
    {
        for (const auto& entry : std::filesystem::directory_iterator(objPath))
        {
            if (!entry.is_directory() && entry.path().extension() == ".o")
            {
                objTime[entry.path().stem().string()] = fileLastWriteTime(entry.path());
            }
        }
    }

    // source path

    std::string srcPath = toml::find<std::string>(data, "paths", "src");

    std::map<std::string, std::chrono::_V2::system_clock::time_point> srcTime;

    std::vector<std::string> cppFiles;
    std::vector<std::string> objFiles;

    if (!std::filesystem::exists(srcPath))
    {
        std::cout << color(Red) << srcPath << " directory not found" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(srcPath))
        {
            if (!entry.is_directory() && entry.path().extension() == ".cpp")
            {
                cppFiles.push_back(entry.path().string());
                objFiles.push_back(objPath + "/" + entry.path().stem().string() + ".o");
                srcTime[entry.path().stem().string()] = fileLastWriteTime(entry.path());
            }
        }
    }

    // include path

    std::string includePath = toml::find<std::string>(data, "paths", "include");

    if (!std::filesystem::exists(includePath))
    {
        std::cout << color(Red) << includePath << " directory not found" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    // lib path and files

    std::string libPath = toml::find<std::string>(data, "paths", "lib");

    if (!std::filesystem::exists(libPath))
    {
        std::cout << color(Red) << libPath << " directory not found" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> libFiles;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(libPath))
        if (!entry.is_directory() && entry.path().extension() == ".a")
            libFiles.push_back(libPath + "/" + entry.path().filename().string());

    std::string main = binPath + "/" + name;

    if (type == "executable")
        main += ".exe";
    else if (type == "dll")
        main += ".dll";
    else
        main += ".lib";

    std::string main2 = binPath + "/" + name + "dll.lib";

    if (type == "dll")
        cflags.push_back("-Wl,--out-implib,"+ main2);

    std::vector<std::string> filesToRecompile;
    std::vector<std::string> filesToRecompile2;

    if (!cppFiles.empty())
    {
        for (size_t i = 0; i < cppFiles.size(); i++)
        {
            std::string fileName = std::filesystem::path(cppFiles[i]).stem().string();
            auto cppTimePoint = srcTime[fileName];
            bool objFound = false;

            // Check if the corresponding object file exists
            auto objIt = objTime.find(fileName);
            if (objIt != objTime.end())
            {
                auto objTimePoint = objIt->second;
                if (cppTimePoint > objTimePoint)
                {
                    filesToRecompile.push_back(cppFiles[i]);
                    filesToRecompile2.push_back(objFiles[i]);
                }
                objFound = true;
            }

            if (!objFound)
            {
                filesToRecompile.push_back(cppFiles[i]);
                filesToRecompile2.push_back(objFiles[i]);
            }
        }
    }
    else
    {
        std::cout << color(Red) << "No C++ source files found" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    bool anyFilesBuilt = false;

    auto buildStart = std::chrono::high_resolution_clock::now();

    if (!filesToRecompile.empty())
    {
        std::cout << "Starting build " << type << " " << option << "..." << std::endl;

        for (size_t i = 0; i < filesToRecompile.size(); i++)
        {
            if (!compileObject(cc, cflags, cdefs, filesToRecompile2[i], filesToRecompile[i], includePath))
            {
                std::cout << color(Red) << "Error" << color(Defult) << std::endl;
                exit(EXIT_FAILURE);
            }
            anyFilesBuilt = true;
        }
    }
    
    if (anyFilesBuilt || !std::filesystem::exists(main))
    {
        if (type == "lib")
        {
            if (!archiveStatic(objFiles, main))
            {
                std::cout << color(Red) << "Error" << color(Defult) << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!compileBinarry(cc, cflags, cdefs, objFiles, libFiles, libs, main, includePath, libPath))
            {
                std::cout << color(Red) << "Error" << color(Defult) << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        anyFilesBuilt = true;

        auto buildEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> buildDuration = buildEnd - buildStart;
        std::cout << color(Green) << main << " done in " << buildDuration.count() << "s" << color(Defult) << std::endl;
    }

    if (!anyFilesBuilt)
    {
        std::cout << color(Gray) << "No new changes detected" << color(Defult) << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        build("debug");
        return EXIT_SUCCESS;
    }

    int opt;
    while ((opt = getopt(argc, argv, "hvcb:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                std::cout << " -v for version \n -b to specfiy a build option (release, debug) \n -c for cleaning obj and bin folders \n -h for help" << std::endl;
                break;
            case 'v':
                std::cout << VERSION << std::endl;
                break;
            case 'c':
                clean();
                break;
            case 'b': {
                std::string buildType = optarg;

                if (buildType != "release" && buildType != "debug")
                {
                    std::cerr << color(Red) << "Unknown build arg " << optarg << color(Defult) << std::endl;
                    return EXIT_FAILURE;
                }

                build(optarg);
                break;
            }
            default:
                std::cerr << color(Red) << "Unknown option. Try -h" << color(Defult) << std::endl;
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}