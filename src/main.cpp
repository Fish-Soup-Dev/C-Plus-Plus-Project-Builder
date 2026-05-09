#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <ctime>
#include <map>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <tuple>
#include <thread>
#include <atomic>

#include "color.hpp"
#include "templateGenerator.hpp"
#include "buildFileLoader.hpp"

#ifdef DEBUG
    #define DEBUG_PRINT(x) std::cout << "DEBUG: " << x << std::endl;
#else
    #define DEBUG_PRINT(x)
#endif

#ifndef VERSION
    #define VERSION "version not set"
#endif

// main thread id used to detect worker threads
static std::thread::id g_mainThreadId;

std::chrono::system_clock::time_point fileLastWriteTime(const std::string& filePath)
{
    namespace fs = std::filesystem;
    std::error_code ec;

    auto ftime = fs::last_write_time(fs::path(filePath), ec);

    if (ec)
    {
        std::cerr << "Error getting file time: " << ec.message() << std::endl;
        return std::chrono::system_clock::time_point::min();
    }

    return std::chrono::clock_cast<std::chrono::system_clock>(ftime);
}

int compileObject
(
    const std::string cc,
    const std::vector<std::string> cflags,
    const std::vector<std::string> cdefs,
    const std::string objectFile,
    const std::string sourceFile,
    const std::string includePath
)
{
    bool isThread = std::this_thread::get_id() != g_mainThreadId;

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

    if (isThread)
    {
        std::cout << color(Gray) << "[Thread] ";
    }

    if (result != EXIT_SUCCESS)
    {
        std::cout << color(Red, true) << objectFile << " Failed." << color(Default) << std::endl;
        DEBUG_PRINT("Command was: " + command);
        return false;
    }

    std::cout << color(Gray) << objectFile << " built in " << std::fixed << std::setprecision(3) << duration.count() << "s" << color(Default) << std::endl;
    DEBUG_PRINT("Command was: " + command);
    return true;
}

int compileBinary
(
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

    if (result != EXIT_SUCCESS)
    {
        std::cout << color(Red, true) << main << " Failed." << color(Default) << std::endl;
        DEBUG_PRINT("Command was: " + command);
        return false;
    }

    std::cout << color(Gray) << main << " built in " << std::fixed << std::setprecision(3) << duration.count() << "s" << color(Default) << std::endl;
    DEBUG_PRINT("Command was: " + command);
    return true;
}

int archiveStatic(const std::vector<std::string> objFiles, const std::string main)
{
    std::string command = "ar rcs ";

    command.append(main);

    for (const auto& obj : objFiles)
        command.append(" " + obj);

    auto start = std::chrono::high_resolution_clock::now();

    int result = std::system(command.c_str());

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;

    if (result != EXIT_SUCCESS)
    {
        std::cout << color(Red, true) << main << " Failed." << color(Default) << std::endl;
        DEBUG_PRINT("Command was: " + command);
        return false;
    }

    std::cout << color(Gray) << main << " built in " << std::fixed << std::setprecision(3) << duration.count() << "s" << color(Default) << std::endl;
    DEBUG_PRINT("Command was: " + command);
    return true;
}

void run(bool option, BuildFileLoader& file)
{
    if (!file.isFound())
    {
        std::cerr << color(Red, true) << "No build.toml file found in current or parent directories." << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (file.getType() != "program")
    {
        std::cerr << color(Red) << "Error can only run programs" << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string optionString = option ? "relesse" : "debug";

    #ifdef _WIN32
        std::string command = ".\\bin\\" + optionString + "\\" + file.getName() + ".exe";
    #elif __linux__
        std::string command = "./bin/" + optionString + "/" + file.getName();
    #endif

    if (std::system(command.c_str()))
    {
        std::cerr << color(Red, true) << "Program returned an error" << color(Default)  << std::endl;
    }

    DEBUG_PRINT("Command was: " + command);
}

std::tuple<int, bool, int, int> parseArguments(int count, char *argumentArray[], std::vector<std::string> options)
{
    int optionIndex = 0; // 0 = none / unknown, otherwise j+1 (matches options vector)
    int release = 0;     // 0 = debug (default), 1 = release
    bool threads = false;
    int arch = 0;        // 0 = x64 (default), 1 = x32, 2 = arm64

    bool anyFlagSeen = false;

    for (int i = 1; i < count; ++i)
    {
        std::string arg = argumentArray[i];

        // check main options
        for (size_t j = 0; j < options.size(); ++j)
        {
            if (arg == options[j])
            {
                // keep first option seen (if you prefer last, remove the guard)
                if (optionIndex == 0)
                    optionIndex = static_cast<int>(j) + 1;
            }
        }

        // release/debug flags
        if (arg == "-r" || arg == "-release")
        {
            release = 1;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-d" || arg == "-debug")
        {
            release = 0;
            anyFlagSeen = true;
            continue;
        }

        // thread flag
        if (arg == "-t" || arg == "-threads")
        {
            threads = true;
            anyFlagSeen = true;
            continue;
        }

        // arch flags
        if (arg == "-x64")
        {
            arch = 0;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-x32")
        {
            arch = 1;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-arm64")
        {
            arch = 2;
            anyFlagSeen = true;
            continue;
        }
    }

    // If flags were provided but the selected option is not build(4) or run(5), error out.
    if (anyFlagSeen && optionIndex != 4 && optionIndex != 5)
    {
        std::cerr << color(Red, true) << "Release/arch flags are only valid with 'build' or 'run'." << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    return std::make_tuple(optionIndex, threads, release, arch);
}

void helpMenu()
{
    std::cout << "help - help menu" << std::endl;
    std::cout << "version - app version" << std::endl;
    std::cout << "clean - removes all temp folders and files in project" << std::endl;
    std::cout << "build - builds project (if debug then bumps patch version)" << std::endl;
    std::cout << "run - runs project" << std::endl;
    std::cout << "new - makes files for a new project" << std::endl;
    std::cout << "minor - bumps minor version" << std::endl;
    std::cout << "major - bumps major version" << std::endl;
}

void appVersion()
{
    #ifdef _WIN32
        std::cout << VERSION << " Windows build" << std::endl;
    #elif __linux__
        std::cout << VERSION << " Linux build" << std::endl;
    #endif
}

void cleanProject(BuildFileLoader& file)
{
    if (!file.isFound())
    {
        std::cerr << color(Red, true) << "No build.toml file found in current or parent directories." << color(Default) << std::endl;
        return;
    }

    std::filesystem::remove_all(file.getBinPath());
    std::cout << color(Gray) << "deleted " << file.getBinPath() << color(Default) << std::endl;

    std::filesystem::remove_all(file.getObjPath());
    std::cout << color(Gray) << "deleted " << file.getObjPath() << color(Default) << std::endl;

    std::cout << color(Green) << "Project cleaned" << color(Default) << std::endl;
}

void build(bool option, BuildFileLoader& file, bool thredding)
{
    if (!file.isFound())
    {
        std::cerr << color(Red, true) << "No build.toml file found in current or parent directories." << color(Default) << std::endl;
        return;
    }

    size_t maxThreads = 0;
    if (thredding)
    {
        unsigned int hc = std::thread::hardware_concurrency();
        if (hc == 0)
        {
            hc = 1;
        }
        maxThreads = static_cast<size_t>(hc);
    }

    std::string optionString = option ? "/release" : "/debug";
    std::string binPath = file.getBinPath() + optionString;
    std::string objPath = file.getObjPath() + optionString;
    std::string srcPath = file.getSrcPath();
    std::string libPath = file.getLibPath();
    std::string projectMain = binPath + "/" + file.getName();
    std::string projectLib = binPath + "/" + file.getName();
    std::string projectType = file.getType();

    std::vector<std::string> cflags = option ? file.getFlagsRelease() : file.getFlagsDebug();
    std::vector<std::string> cdefs = option ? file.getDefsRelease() : file.getDefsDebug();

    cdefs.push_back("-DVERSION=\\\"" + file.getVersion() + "\\\"");

    if (projectType == "shared")
    {
        cflags.push_back("-Wl,--out-implib," + projectLib);
    }

    // add exstension depending on type
    #ifdef _WIN32
        std::vector<std::string> libList = file.getLibs(true);

        if (projectType == "program")
            projectMain += ".exe";
        else if (projectType == "shared")
            projectMain += ".dll";
        else
            projectMain += ".lib";

        projectLib += "dll.lib";
    #elif __linux__
        std::vector<std::string> libList = file.getLibs(false);

        if (projectType == "shared")
            projectMain += ".so";
        else if (projectType != "program")
            projectMain += ".a";

        projectLib += "so.a";
    #endif

    if (!std::filesystem::exists(binPath))
    {
        std::cout << color(Gray) << binPath << " directory not found. Creating..." << color(Default) << std::endl;
        std::filesystem::create_directories(binPath);
    }

    // create list of oject files and get time created
    std::map<std::string, std::chrono::_V2::system_clock::time_point> objTime;

    if (!std::filesystem::exists(objPath))
    {
        std::cout << color(Gray) << objPath << " directory not found. Creating..." << color(Default) << std::endl;
        std::filesystem::create_directories(objPath);
    }
    else
    {
        for (const auto& entry : std::filesystem::directory_iterator(objPath))
        {
            if (!entry.is_directory() && entry.path().extension() == ".o")
            {
                objTime[entry.path().stem().string()] = fileLastWriteTime(entry.path().string());
            }
        }
    }

    // load source files and time last edited to compare with object files
    std::map<std::string, std::chrono::_V2::system_clock::time_point> srcTime;

    std::vector<std::string> cppFiles;
    std::vector<std::string> objFiles;

    if (!std::filesystem::exists(srcPath))
    {
        std::cout << color(Red, true) << srcPath << " directory not found" << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    // load paths and edit times of src files and create list of objects to be created?
    for (const auto& entry : std::filesystem::recursive_directory_iterator(srcPath))
    {
        if (!entry.is_directory() && (entry.path().extension() == ".cpp" || entry.path().extension() == ".c"))
        {
            cppFiles.push_back(entry.path().string());
            objFiles.push_back(objPath + "/" + entry.path().stem().string() + ".o");
            srcTime[entry.path().stem().string()] = fileLastWriteTime(entry.path().string());
        }
    }

    if (cppFiles.empty())
    {
        std::cout << color(Red, true) << "No C++ source files found" << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    // load lib files
    if (!std::filesystem::exists(libPath))
    {
        std::cout << color(Gray) << libPath << " directory not found. Creating..." << color(Default) << std::endl;
        std::filesystem::create_directories(libPath);
    }

    std::vector<std::string> libFiles;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(libPath))
    {
        if (!entry.is_directory() && entry.path().extension() == ".a")
        {
            libFiles.push_back(libPath + "/" + entry.path().filename().string());
        }
    }

    // not list of all files but list of files deemed to be recompiled
    std::vector<std::string> sourceFiles;
    std::vector<std::string> objectFiles;

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
                sourceFiles.push_back(cppFiles[i]);
                objectFiles.push_back(objFiles[i]);
            }
            objFound = true;
        }

        if (!objFound)
        {
            sourceFiles.push_back(cppFiles[i]);
            objectFiles.push_back(objFiles[i]);
        }
    }

    bool anyFilesBuilt = false;

    auto buildStart = std::chrono::high_resolution_clock::now();

    std::cout << "Starting build " << projectType << " " << (option ? "relesse" : "debug") << "..." << std::endl;

    if (maxThreads > 0 && sourceFiles.size() > 0) // multi thread work
    {
        std::vector<std::thread> workers;
        std::atomic<bool> failed(false);

        for (size_t i = 0; i < sourceFiles.size(); i++)
        {
            // copy parameters for thread
            std::string srcFile = sourceFiles[i];
            std::string objFile = objectFiles[i];
            auto cc_copy = file.getCompiler();
            auto cflags_copy = cflags;
            auto cdefs_copy = cdefs;
            auto include_copy = file.getIncludePath();

            workers.emplace_back([cc_copy, cflags_copy, cdefs_copy, objFile, srcFile, include_copy, &failed]()
            {
                if (failed.load())
                    return;

                if (!compileObject(cc_copy, cflags_copy, cdefs_copy, objFile, srcFile, include_copy))
                    failed.store(true);
            });

            // throttle threads: when we reach max, join the oldest one
            if (workers.size() >= maxThreads)
            {
                workers.front().join();
                workers.erase(workers.begin());
                if (failed.load())
                    break;
            }
        }

        // join remaining workers
        for (auto &t : workers)
            t.join();

        if (failed.load())
        {
            std::cout << color(Red, true) << "Compile Object Error" << color(Default) << std::endl;
            exit(EXIT_FAILURE);
        }

        anyFilesBuilt = true;
    }
    else if (sourceFiles.size() > 0) // single thread work
    {
        for (size_t i = 0; i < sourceFiles.size(); i++)
        {
            if (!compileObject(file.getCompiler(), cflags, cdefs, objectFiles[i], sourceFiles[i], file.getIncludePath()))
            {
                std::cout << color(Red, true) << "Compile Object Error" << color(Default) << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        anyFilesBuilt = true;
    }

    // if we built files or the binarry is missing remake it
    if (anyFilesBuilt || !std::filesystem::exists(projectMain))
    {
        if (projectType == "lib")
        {
            if (!archiveStatic(objFiles, projectMain))
            {
                std::cout << color(Red, true) << "Error" << color(Default) << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!compileBinary(file.getCompiler(), cflags, cdefs, objFiles, libFiles, libList, projectMain, file.getIncludePath(), libPath))
            {
                std::cout << color(Red, true) << "Compile Binary Error" << color(Default) << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        anyFilesBuilt = true;

        auto buildEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> buildDuration = buildEnd - buildStart;
        std::cout << color(Green) << projectMain << " done in " << buildDuration.count() << "s" << color(Default) << std::endl;
    }

    if (!anyFilesBuilt)
    {
        std::cout << color(Gray) << "No new changes detected" << color(Default) << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << color(Red, true) << "No option given. Try help for how to use" << color(Default) << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> options = {"help", "version", "clean", "build", "run", "new", "class", "bump-minor", "bump-major"};

    // record main thread id so workers can detect they're in a thread
    g_mainThreadId = std::this_thread::get_id();

    int arch = 0; // 0 = x64, 1 = x32, 2 = arm64
    int release = 0; // 0 = debug, 1 = release
    bool threads = false;
    int option = 0;

    std::tie(option, threads, release, arch) = parseArguments(argc, argv, options);

    //check for buid file and exit if not found
    BuildFileLoader buildFile;

    switch (option)
    {
    case 1:
        helpMenu();
        break;
    case 2:
        appVersion();
        break;
    case 3:
        cleanProject(buildFile);
        break;
    case 4:
        if (!release)
        {
            buildFile.bumpPatchVersion();
        }
        build(release ? true : false, buildFile, threads);
        break;
    case 5:
        run(release ? true : false, buildFile);
        break;
    case 6:
        makeProject();
        break;
    case 7:
        makeClass();
        break;
    case 8:
        buildFile.bumpMinorVersion();
        break;
    case 9:
        buildFile.bumpMajorVersion();
        break;
    default:
        std::cerr << color(Red, true) << "Unknown option. Try (help)" << color(Default) << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
