#include "templateGenorator.h"

#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>

#include "color.hpp"

#include "templates/buildfileTemplate.hpp"
#include "templates/programTemplate.hpp"
#include "templates/sharedLibTemplate.hpp"
#include "templates/staticLibTemplate.hpp"

int MakeProject()
{
    // --------------------------------
    std::cout << "Project name" << std::endl;
    std::cout << color(Blue) << ">> " << color(Defult);
    std::string name;
    std::getline(std::cin, name);

    if (name.empty())
    {
        std::cout << color(Red) << "Error no name given" << color(Defult) << std::endl;
        return 1;
    }

    // --------------------------------

    std::cout << "Build Options (1) program (2) shared (3) static" << std::endl;
    
    std::cout << color(Blue) << ">> " << color(Defult);
    std::string type;
    std::getline(std::cin, type);

    if (type.empty())
    {
        std::cout << color(Red) << "Error no type given" << color(Defult) << std::endl;
        return 1;
    }

    // --------------------------------

    std::cout << "C++ Version [89, 03, 11, 14, 17, 20]" << std::endl;
    std::cout << color(Blue) << ">> " << color(Defult);
    std::string version;
    std::getline(std::cin, version);

    if (version.empty())
    {
        std::cout << color(Red) << "Error no version given" << color(Defult) << std::endl;
        return 1;
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
    }

    std::string project_path = "./" + name;

    std::cout << color(Green) << "Maing template at " << project_path << std::endl;

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
            cflagsR = "\"-O2\", \"-static\", \"" + versionName + "\"";
            cflagsD = "\"-g\", \"-wall\", \"" + versionName + "\"";
            cdefsR = "\"-DNDEBUG\"";
            cdefsD = "\"-DDEBUG\"";
        }
        break;

    case 2: // shared
        {
            std::string nameupper = name;
            std::transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);

            std::ofstream headderDllFile(project_path + "/include/" + name + ".h");
            headderDllFile << sharedLibHeadderTemplate(nameupper);
            headderDllFile.close();

            std::ofstream mainDllFile(project_path + "/src/" + name +".cpp");
            mainDllFile << sharedLibTemplate(name);
            mainDllFile.close();

            typeName = "shared";
            cflagsR = "\"-O2\", \"-shared\", \"" + versionName + "\"";
            cflagsD = "\"-g\", \"-wall\", \"-shared\", \"" + versionName + "\"";
            cdefsR = "\"-DNDEBUG\", \"-DBUILD_DLL\"";
            cdefsD = "\"-DDEBUG\", \"-DBUILD_DLL\"";
        }
        break;

    case 3: // static
        {
            std::string nameupper = name;
            std::transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);

            std::ofstream headderLibFile(project_path + "/include/" + name + ".h");
            headderLibFile << staticLibHeadderTemplate(nameupper);
            headderLibFile.close();

            std::ofstream mainLibFile(project_path + "/src/" + name +".cpp");
            mainLibFile << staticLibTemplate(name);
            mainLibFile.close();

            typeName = "static";
            cflagsR = "\"-O2\", \"" + versionName + "\"";
            cflagsD = "\"-g\", \"-wall\", \"" + versionName + "\"";
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

    std::cout << color(Green) << "Done." << color(Defult) << std::endl;

    return 0;
}
