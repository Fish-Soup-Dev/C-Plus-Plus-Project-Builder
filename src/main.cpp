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

#include "utils.hpp"
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

static std::thread::id g_mainThreadId;

enum class ProgramOption
{
    Help,
    Version,
    Clean,
    Build,
    Run,
    New,
    Class,
    BumpMinor,
    BumpMajor,
    Unknown
};

std::vector<std::string> options = {
    "help", 
    "version", 
    "clean", 
    "build", 
    "run", 
    "new", 
    "class", 
    "minor", 
    "major"
};

struct BuildOptions
{
    bool release = false;
    bool threads = false;
    int arch = 0; // 0 = x64, 1 = x32, 2 = arm64
    ProgramOption option = ProgramOption::Unknown;
};

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

    if (result != EXIT_SUCCESS)
    {
        utils::printLine(isThread ? "[Thread]" + objectFile + " Failed." : objectFile + " Failed.", utils::Red, true);
        DEBUG_PRINT("Command was: " + command);
        return false;
    }

    utils::printLine(isThread ? "[Thread]" + objectFile + " - " + std::to_string(duration.count()) + "s" : objectFile + " built in " + std::to_string(duration.count()) + "s", utils::Gray);
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
        utils::printLine(main + " Failed.", utils::Red, true);
        DEBUG_PRINT("Command was: " + command);
        return false;
    }

    utils::printLine(main + " - " + std::to_string(duration.count()) + "s", utils::Gray);
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
        utils::printLine(main + " Failed.", utils::Red, true);
        DEBUG_PRINT("Command was: " + command);
        return false;
    }

    utils::printLine(main + " - " + std::to_string(duration.count()) + "s", utils::Gray);
    DEBUG_PRINT("Command was: " + command);
    return true;
}

void run(BuildOptions& opts, BuildFileLoader& file)
{
    if (!file.isFound())
    {
        throw std::runtime_error("No build.toml file found in current or parent directories.");
    }

    if (file.getType() != "program")
    {
        throw std::runtime_error("Project type must be 'program' to run.");
    }

    std::string profile = opts.release ? "release" : "debug";
    std::filesystem::path binPath = std::filesystem::path("bin") / profile / file.getName();

    #ifdef _WIN32
        binPath.replace_extension(".exe");
        std::string command = binPath.string();
    #elif __linux__
        std::string command = "./" + binPath.string();
    #endif

    if (std::system(command.c_str()))
    {
        throw std::runtime_error("Program returned an error");
    }

    DEBUG_PRINT("Command was: " + command);
}

void helpMenu()
{
    utils::printLine("help - help menu", utils::Default);
    utils::printLine("version - app version", utils::Default);
    utils::printLine("clean - removes all temp folders and files in project", utils::Default);
    utils::printLine("build - builds project (if debug then bumps patch version)", utils::Default);
    utils::printLine("run - runs project", utils::Default);
    utils::printLine("new - makes files for a new project", utils::Default);
    utils::printLine("minor - bumps minor version", utils::Default);
    utils::printLine("major - bumps major version", utils::Default);
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
        throw std::runtime_error("No build.toml file found in current or parent directories.");
    }

    std::filesystem::remove_all(file.getBinPath());
    std::filesystem::remove_all(file.getObjPath());

    utils::printLine("Removed " + file.getBinPath() + " and " + file.getObjPath(), utils::Green);
}

void build(BuildOptions& opts, BuildFileLoader& file)
{
    if (!file.isFound())
    {
        throw std::runtime_error("No build.toml file found in current or parent directories.");
    }

    size_t maxThreads = 0;
    if (opts.threads)
    {
        unsigned int hc = std::thread::hardware_concurrency();
        if (hc == 0)
        {
            hc = 1;
        }
        maxThreads = static_cast<size_t>(hc);
    }

    std::string optionString = opts.release ? "/release" : "/debug";
    std::string binPath = file.getBinPath() + optionString;
    std::string objPath = file.getObjPath() + optionString;
    std::string srcPath = file.getSrcPath();
    std::string libPath = file.getLibPath();
    std::string projectMain = binPath + "/" + file.getName();
    std::string projectLib = binPath + "/" + file.getName();
    std::string projectType = file.getType();

    std::vector<std::string> cflags = opts.release ? file.getFlagsRelease() : file.getFlagsDebug();
    std::vector<std::string> cdefs = opts.release ? file.getDefsRelease() : file.getDefsDebug();

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

    utils::printLine("Started build " + projectType + " " + (opts.release ? "release" : "debug") + "...", utils::Green);

    if (!std::filesystem::exists(binPath))
    {
        utils::printLine("Creating " + binPath + " directory...", utils::Gray);
        std::filesystem::create_directories(binPath);
    }

    // create list of oject files and get time created
    std::map<std::string, std::chrono::_V2::system_clock::time_point> objTime;

    if (!std::filesystem::exists(objPath))
    {
        utils::printLine("Creating " + objPath + " directory...", utils::Gray);
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
        throw std::runtime_error(srcPath + " directory not found");
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
        throw std::runtime_error("No C++ source files found");
    }

    // load lib files
    if (!std::filesystem::exists(libPath))
    {
        utils::printLine("Creating " + libPath + " directory...", utils::Gray);
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

    if (maxThreads > 0 && sourceFiles.size() > 0) 
    {
        std::vector<std::thread> workers;
        std::atomic<bool> failed(false);
        std::atomic<size_t> nextFileIndex(0); // Shared counter for all threads

        // Prepare shared data for threads
        auto cc = file.getCompiler();
        auto include = file.getIncludePath();

        // Spawn a fixed pool of workers
        for (size_t t = 0; t < maxThreads; ++t)
        {
            workers.emplace_back([&]() 
            {
                while (true)
                {
                    // Atomically grab the next available file index
                    size_t i = nextFileIndex.fetch_add(1);
                    
                    // Stop if we ran out of files or another thread failed
                    if (i >= sourceFiles.size() || failed.load())
                        break;

                    if (!compileObject(cc, cflags, cdefs, objectFiles[i], sourceFiles[i], include))
                    {
                        failed.store(true);
                        break;
                    }
                }
            });
        }

        // Wait for all workers to complete their queues
        for (auto &t : workers)
        {
            t.join();
        }

        if (failed.load())
        {
            throw std::runtime_error("Compilation failed in one of the threads.");
        }

        anyFilesBuilt = true;
    }
    else if (sourceFiles.size() > 0) // single thread work
    {
        for (size_t i = 0; i < sourceFiles.size(); i++)
        {
            if (!compileObject(file.getCompiler(), cflags, cdefs, objectFiles[i], sourceFiles[i], file.getIncludePath()))
            {
                throw std::runtime_error("Failed to compile object file: " + sourceFiles[i]);
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
                throw std::runtime_error("Failed to archive static library");
            }
        }
        else
        {
            if (!compileBinary(file.getCompiler(), cflags, cdefs, objFiles, libFiles, libList, projectMain, file.getIncludePath(), libPath))
            {
                throw std::runtime_error("Failed to compile binary");
            }
        }

        anyFilesBuilt = true;

        auto buildEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> buildDuration = buildEnd - buildStart;
        utils::printLine("Created " + projectMain + " in " + std::to_string(buildDuration.count()) + "s", utils::Green);
    }

    if (!anyFilesBuilt)
    {
        utils::printLine("No new changes detected", utils::Gray);
    }
}

BuildOptions parseArguments(int count, char* argumentArray[], const std::vector<std::string>& options)
{
    BuildOptions opts;

    bool anyFlagSeen = false;

    for (int i = 1; i < count; ++i)
    {
        std::string arg = argumentArray[i];

        auto it = std::find(options.begin(), options.end(), arg);
        if (it != options.end())
        {   
            opts.option = static_cast<ProgramOption>(std::distance(options.begin(), it));
            continue;
        }
        if (arg == "-r" || arg == "-release")
        {
            opts.release = true;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-d" || arg == "-debug")
        {
            opts.release = false;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-t" || arg == "-threads")
        {
            opts.threads = true;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-x64")
        {
            opts.arch = 0;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-x32")
        {
            opts.arch = 1;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-arm64")
        {
            opts.arch = 2;
            anyFlagSeen = true;
            continue;
        }
    }

    if (anyFlagSeen && opts.option != ProgramOption::Build && opts.option != ProgramOption::Run)
    {
        utils::printError("Release/arch flags are only valid with 'build' or 'run'.", utils::Red, true);
        exit(EXIT_FAILURE);
    }

    return opts;
}

class Application
{
public:
    Application(int argc, char* argv[]) : argc(argc), argv(argv)
    {
        if (argc < 2)
        {
            throw std::runtime_error("No option provided. Use 'help' for usage.");
        }
    }

    void execute()
    {
        g_mainThreadId = std::this_thread::get_id();

        BuildOptions buildOpts = parseArguments(argc, argv, options);
        BuildFileLoader buildFile;

        switch (buildOpts.option)
        {
        case ProgramOption::Help:
            helpMenu();
            break;
        case ProgramOption::Version:
            appVersion();
            break;
        case ProgramOption::Clean:
            cleanProject(buildFile);
            break;
        case ProgramOption::Build:
            if (!buildOpts.release)
            {
                buildFile.bumpPatchVersion();
                utils::printLine("Bumped patch version " + buildFile.getVersion(), utils::Gray);
            }
            build(buildOpts, buildFile);
            break;
        case ProgramOption::Run:
            run(buildOpts, buildFile);
            break;
        case ProgramOption::New:
            makeProject();
            break;
        case ProgramOption::Class:
            makeClass();
            break;
        case ProgramOption::BumpMinor:
            buildFile.bumpMinorVersion();
            utils::printLine("Bumped minor version " + buildFile.getVersion(), utils::Green);
            break;
        case ProgramOption::BumpMajor:
            buildFile.bumpMajorVersion();
            utils::printLine("Bumped major version " + buildFile.getVersion(), utils::Green);
            break;
        default:
            utils::printLine("Unknown option. Try help for how to use", utils::Red, true);
            break;
        }
    }

private:
    int argc;
    char** argv;
};

int main(int argc, char *argv[])
{
    try
    {
        Application app(argc, argv);
        app.execute();
    }
    catch (const std::exception& e)
    {
        utils::printError("Error: " + std::string(e.what()), utils::Red, true);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
