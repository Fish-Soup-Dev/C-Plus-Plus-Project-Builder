#define VERSION "0.6.5"

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
    {
        std::cout << color(Gray) << objectFile << " built in " << std::fixed << std::setprecision(3) << duration.count() << "s";
        // if called from a worker thread, append thread id
        if (std::this_thread::get_id() != g_mainThreadId)
            std::cout << " [thread " << std::this_thread::get_id() << "]";
        std::cout << color(Default) << std::endl;
    }
    else
    {
        std::cout << color(Red) << objectFile << " Failed.";
        if (std::this_thread::get_id() != g_mainThreadId)
            std::cout << " [thread " << std::this_thread::get_id() << "]";
        std::cout << color(Default) << std::endl;
        return false;
    }

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

    if (result == EXIT_SUCCESS)
        std::cout << color(Gray) << main << " built in " << std::fixed << std::setprecision(3) << duration.count() << "s" << color(Default) << std::endl;
    else
    {
        std::cout << color(Red) << main << " Failed." << color(Default) << std::endl;
        return false;
    }

    return true;
}

int archiveStatic
(
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
        std::cout << color(Gray) << main << " packed in " << std::fixed << std::setprecision(3) << duration.count() << "s" << color(Default) << std::endl;
    else
    {
        std::cout << color(Red) << main << " Failed." << color(Default) << std::endl;
        return false;
    }

    return true;
}

void clean
(
    std::string binPath, 
    std::string objPath
)
{
    std::filesystem::remove_all(binPath);
    std::cout << color(Gray) << "deleted " << binPath << color(Default) << std::endl;

    std::filesystem::remove_all(objPath);
    std::cout << color(Gray) << "deleted " << objPath << color(Default) << std::endl;

    std::cout << color(Green) << "Project cleaned" << color(Default) << std::endl;
}

void run
(
    const std::string option, 
    const std::string name, 
    const std::string type
)
{
    if (type != "program")
    {
        std::cerr << color(Red) << "Error can only run programs" << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    #ifdef _WIN32
        std::string command = ".\\bin\\" + option + "\\" + name + ".exe";
    #elif __linux__
        std::string command = "./bin/" + option + "/" + name;
    #endif
    
    if (std::system(command.c_str()))
    {
        std::cerr << color(Red) << "Program returned an error" << color(Default)  << std::endl;
    }
}

void build
(
    const std::string option, 
    const std::string name,
    const std::string type, 
    const std::string cc, 
    const std::vector<std::string> ldflags,
    const std::vector<std::string> libs,
    std::string binPath,
    std::string objPath,
    std::vector<std::string> cdefs,
    std::vector<std::string> cflags,
    const std::string srcPath,
    const std::string includePath,
    const std::string libPath,
    size_t maxThreads
)
{
    if (option == "release")
    {
        binPath += "/release";
        objPath += "/release";
    }
    else
    {
        binPath += "/debug";
        objPath += "/debug";
    }

    if (!std::filesystem::exists(binPath))
    {
        std::cout << color(Gray) << binPath << " directory not found. Creating..." << color(Default) << std::endl;
        std::filesystem::create_directories(binPath);
    }
    
    std::map<std::string, std::chrono::_V2::system_clock::time_point> objTime;

    if (!std::filesystem::exists(objPath))
    {
        std::cout << color(Gray) << objPath << " directory not found. Creating..." << color(Default) << std::endl;
        std::filesystem::create_directories(objPath);
    }
    else // get when obj files where edited
    {
        for (const auto& entry : std::filesystem::directory_iterator(objPath))
        {
            if (!entry.is_directory() && entry.path().extension() == ".o")
            {
                objTime[entry.path().stem().string()] = fileLastWriteTime(entry.path().string());
            }
        }
    }

    std::map<std::string, std::chrono::_V2::system_clock::time_point> srcTime;

    std::vector<std::string> cppFiles;
    std::vector<std::string> objFiles;

    if (!std::filesystem::exists(srcPath))
    {
        std::cout << color(Red) << srcPath << " directory not found" << color(Default) << std::endl;
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
                srcTime[entry.path().stem().string()] = fileLastWriteTime(entry.path().string());
            }
        }
    }

    if (!std::filesystem::exists(libPath))
    {
        std::cout << color(Red) << libPath << " directory not found" << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> libFiles;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(libPath))
        if (!entry.is_directory() && entry.path().extension() == ".a")
            libFiles.push_back(libPath + "/" + entry.path().filename().string());

    std::string main = binPath + "/" + name;

    #ifdef _WIN32
        if (type == "program")
            main += ".exe";
        else if (type == "shared")
            main += ".dll";
        else
            main += ".lib";

        std::string main2 = binPath + "/" + name + "dll.lib";
    #elif __linux__
        if (type == "shared")
            main += ".so";
        else if (type != "program")
            main += ".a";

        std::string main2 = binPath + "/" + name + "so.a";
    #endif

    if (type == "shared")
        cflags.push_back("-Wl,--out-implib," + main2);

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
        std::cout << color(Red) << "No C++ source files found" << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    bool anyFilesBuilt = false;

    auto buildStart = std::chrono::high_resolution_clock::now();

    if (!filesToRecompile.empty())
    {
        std::cout << "Starting build " << type << " " << option << "..." << std::endl;

        if (maxThreads > 0)
        {
            std::vector<std::thread> workers;
            std::atomic<bool> failed(false);

            for (size_t i = 0; i < filesToRecompile.size(); i++)
            {
                // copy parameters for thread
                std::string srcFile = filesToRecompile[i];
                std::string objFile = filesToRecompile2[i];
                auto cc_copy = cc;
                auto cflags_copy = cflags;
                auto cdefs_copy = cdefs;
                auto include_copy = includePath;

                workers.emplace_back([cc_copy, cflags_copy, cdefs_copy, objFile, srcFile, include_copy, &failed]() {
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
                std::cout << color(Red) << "Compile Object Error" << color(Default) << std::endl;
                exit(EXIT_FAILURE);
            }

            anyFilesBuilt = true;
        }
        else
        {
            for (size_t i = 0; i < filesToRecompile.size(); i++)
            {
                if (!compileObject(cc, cflags, cdefs, filesToRecompile2[i], filesToRecompile[i], includePath))
                {
                    std::cout << color(Red) << "Compile Object Error" << color(Default) << std::endl;
                    exit(EXIT_FAILURE);
                }
                anyFilesBuilt = true;
            }
        }
    }
    
    if (anyFilesBuilt || !std::filesystem::exists(main))
    {
        if (type == "lib")
        {
            if (!archiveStatic(objFiles, main))
            {
                std::cout << color(Red) << "Error" << color(Default) << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!compileBinary(cc, cflags, cdefs, objFiles, libFiles, libs, main, includePath, libPath))
            {
                std::cout << color(Red) << "Compile Binary Error" << color(Default) << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        anyFilesBuilt = true;

        auto buildEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> buildDuration = buildEnd - buildStart;
        std::cout << color(Green) << main << " done in " << buildDuration.count() << "s" << color(Default) << std::endl;
    }

    if (!anyFilesBuilt)
    {
        std::cout << color(Gray) << "No new changes detected" << color(Default) << std::endl;
    }
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
        if (arg == "-r" || arg == "--release")
        {
            release = 1;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-d" || arg == "--debug")
        {
            release = 0;
            anyFlagSeen = true;
            continue;
        }

        // thread flag
        if (arg == "-t" || arg == "--threads")
        {
            threads = true;
            anyFlagSeen = true;
            continue;
        }

        // arch flags
        if (arg == "-x64" || arg == "--x64")
        {
            arch = 0;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-x32" || arg == "--x32")
        {
            arch = 1;
            anyFlagSeen = true;
            continue;
        }
        if (arg == "-arm64" || arg == "--arm64")
        {
            arch = 2;
            anyFlagSeen = true;
            continue;
        }
    }

    // If flags were provided but the selected option is not build(4) or run(5), error out.
    if (anyFlagSeen && optionIndex != 4 && optionIndex != 5)
    {
        std::cerr << color(Red) << "Release/arch flags are only valid with 'build' or 'run'." << color(Default) << std::endl;
        exit(EXIT_FAILURE);
    }

    return std::make_tuple(optionIndex, threads, release, arch);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << color(Red) << "No option given. Try help for how to use" << color(Default) << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> options = {"help", "version", "clean", "build", "run", "new"};

    // record main thread id so workers can detect they're in a thread
    g_mainThreadId = std::this_thread::get_id();

    int arch = 0; // 0 = x64, 1 = x32, 2 = arm64
    int release = 0; // 0 = debug, 1 = release
    bool threads = false;
    int option = 0;

    std::tie(option, threads, release, arch) = parseArguments(argc, argv, options);

    //check for buid file and exit if not found
    BuildFileLoader buildFile;

    if (!buildFile.isFound())
    {
        std::cerr << "No Build File" << std::endl;
        return EXIT_FAILURE;
    }

    switch (option)
    {
        case 1:
            std::cout << "help - help menu" << std::endl;
            std::cout << "version - app version" << std::endl;
            std::cout << "clean - removes all temp folders and files in project" << std::endl;
            std::cout << "build - builds project" << std::endl;
            std::cout << "run - runs project" << std::endl;
            std::cout << "new - makes files for a new project" << std::endl;
            break;

        case 2:
            #ifdef _WIN32
                std::cout << VERSION << " Windows build" << std::endl;
            #elif __linux__
                std::cout << VERSION << " Linux build" << std::endl;
            #endif
            break;

        case 3:
            clean(buildFile.getBinPath(), buildFile.getObjPath());
            break;

        case 4:
        {
            size_t maxThreadsLocal = 0;
            if (threads)
            {
                unsigned int hc = std::thread::hardware_concurrency();
                if (hc == 0)
                {
                    long n = sysconf(_SC_NPROCESSORS_ONLN);
                    if (n > 0)
                        hc = static_cast<unsigned int>(n);
                }
                if (hc == 0)
                    hc = 1;
                maxThreadsLocal = static_cast<size_t>(hc);
            }

            build(release ? "release" : "debug",
                buildFile.getName(),
                buildFile.getType(),
                buildFile.getCompiler(),
                buildFile.getLdFlags(),
                buildFile.getLibs(),
                buildFile.getBinPath(),
                buildFile.getObjPath(),
                release ? buildFile.getDefsRelease() : buildFile.getDefsDebug(),
                release ? buildFile.getFlagsRelease(): buildFile.getFlagsDebug(),
                buildFile.getSrcPath(),
                buildFile.getIncludePath(),
                buildFile.getLibPath(),
                maxThreadsLocal );
            break;
        }

        case 5:
            run(release ? "release" : "debug", 
                buildFile.getName(), 
                buildFile.getType());
            break;

        case 6:
            MakeProject();
            break;

        default:
            std::cerr << color(Red) << "Unknown option. Try (help)" << color(Default) << std::endl;
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}