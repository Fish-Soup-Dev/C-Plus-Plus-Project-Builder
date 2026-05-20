#include "templateGenerator.hpp"

#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>

#include "utils.hpp"

#include "templates/buildfileTemplate.hpp"
#include "templates/programTemplate.hpp"
#include "templates/sharedLibTemplate.hpp"
#include "templates/staticLibTemplate.hpp"
#include "templates/classTemplate.hpp"

void makeProject()
{
    // --------------------------------
    utils::printLine("? Project name ", utils::Gray);
    std::string name;
    std::getline(std::cin, name);

    if (name.empty())
    {
        throw std::runtime_error("Error no name given");
    }

    // --------------------------------

    utils::printLine("? Build Options (1) program, (2) shared, (3) static ", utils::Gray);
    std::string type;
    std::getline(std::cin, type);

    if (type.empty())
    {
        throw std::runtime_error("Error no type given");
    }

    // --------------------------------

    utils::printLine("? C++ Version [98, 03, 11, 14, 17, 20, 23, 26] ", utils::Gray);
    std::string version;
    std::getline(std::cin, version);

    if (version.empty())
    {
        throw std::runtime_error("Error no version given");
    }

    // --------------------------------

    std::string versionName;

    switch (std::stoi(version))
    {
    case 98:
        versionName = "-std=c++98";
        break;
    case 3:
        versionName = "-std=c++03";
        break;
    case 11:
        versionName = "-std=c++11";
        break;
    case 14:
        versionName = "-std=c++14";
        break;
    case 17:
        versionName = "-std=c++17";
        break;
    case 20:
        versionName = "-std=c++20";
        break;
    case 23:
        versionName = "-std=c++23";
        break;
    case 26:
        versionName = "-std=c++26";
        break;
    }

    std::string project_path = "./" + name;

    utils::printLine("Making template at " + project_path, utils::Blue);

    std::filesystem::create_directory(project_path);
    std::filesystem::create_directory(project_path + "/src");
    std::filesystem::create_directory(project_path + "/lib");
    std::filesystem::create_directory(project_path + "/include");

    std::string typeName, cflagsR, cflagsD, cdefsR, cdefsD;

    switch (std::stoi(type))
    {
    case 1: // program
        {
            std::ofstream mainFile(project_path + "/src/main.cpp");
            mainFile << programTemplate();
            mainFile.close();

            typeName = "program";
            cflagsR = "\"-O2\", \"" + versionName + "\"";
            cflagsD = "\"-g\", \"-Wall\", \"" + versionName + "\"";
            cdefsR = "\"-DNDEBUG\"";
            cdefsD = "\"-DDEBUG\"";
        }
        break;

    case 2: // shared
        {
            std::string nameupper = name;
            std::transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);

            std::ofstream headerDllFile(project_path + "/include/" + name + ".h");
            headerDllFile << sharedLibHeaderTemplate(nameupper);
            headerDllFile.close();

            std::ofstream mainDllFile(project_path + "/src/" + name +".cpp");
            mainDllFile << sharedLibTemplate(name);
            mainDllFile.close();

            typeName = "shared";
            cflagsR = "\"-O2\", \"-shared\", \"" + versionName + "\"";
            cflagsD = "\"-g\", \"-Wall\", \"-shared\", \"" + versionName + "\"";
            cdefsR = "\"-DNDEBUG\", \"-DBUILD_DLL\"";
            cdefsD = "\"-DDEBUG\", \"-DBUILD_DLL\"";
        }
        break;

    case 3: // static
        {
            std::string nameupper = name;
            std::transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);

            std::ofstream headerLibFile(project_path + "/include/" + name + ".h");
            headerLibFile << staticLibHeaderTemplate(nameupper);
            headerLibFile.close();

            std::ofstream mainLibFile(project_path + "/src/" + name +".cpp");
            mainLibFile << staticLibTemplate(name);
            mainLibFile.close();

            typeName = "static";
            cflagsR = "\"-O2\", \"" + versionName + "\"";
            cflagsD = "\"-g\", \"-Wall\", \"" + versionName + "\"";
            cdefsR = "\"-DNDEBUG\"";
            cdefsD = "\"-DDEBUG\"";
        }
        break;

    default:
        break;
    }

    std::ofstream buildFile(project_path + "/build.toml");
    buildFile << buildfile_template(name, typeName, cflagsR, cflagsD, cdefsR, cdefsD);
    buildFile.close();

    utils::printLine("Done.", utils::Green);
}

void makeClass()
{
    // --------------------------------
    utils::printLine("? Class name ", utils::Gray);
    std::string name;
    std::getline(std::cin, name);

    if (name.empty())
    {
        throw std::runtime_error("Error no name given");
    }

    std::string nameupper = name;
    std::transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);

    std::ofstream headderClassFile("./src/" + name + ".hpp");
    headderClassFile << classHeaderTemplate(nameupper, name);
    headderClassFile.close();

    std::ofstream classFile("./src/" + name + ".cpp");
    classFile << classTemplate(name);
    classFile.close();

    utils::printLine("Done.", utils::Green);
}
