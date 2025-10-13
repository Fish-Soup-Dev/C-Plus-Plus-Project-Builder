#ifndef TEMPLATE_GENORATOR_H
#define TEMPLATE_GENORATOR_H

#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <string>
#include <unistd.h>

#include "color.hpp"

std::string emptyMainFile()
{
	return 
R"(#include <iostream>

int main(int argc, char *argv[])
{
    std::cout << "it works" << std::endl;
    return 0;
})";
}

std::string emptyDllHeadderFile(std::string name)
{
	return
R"(#ifndef )" + name + R"(_HPP
#define )" + name + R"(_HPP

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef BUILD_DLL
    #define )" + name + R"( __declspec(dllexport)
#else
    #define )" + name + R"( __declspec(dllimport)
#endif

// functions here

#ifdef __cplusplus
    }
#endif

// or here

#endif)";
}

std::string emptyDllMainFile(std::string name)
{
	return
R"(#include ")" + name + R"(.h"
)";
}

std::string emptyLibHeadderFile(std::string name)
{
	return
R"(#ifndef )" + name + R"(_HPP
#define )" + name + R"(_HPP

#endif)";
}

std::string emptyLibMainFile(std::string name)
{
	return
R"(#include ")" + name + R"(.h"
)";
}

std::string basicMakefile(std::string name, std::string version)
{
    return
R"(CXX = g++
BUILD ?= DEBUG
CFLAGS = )" + version + R"(
LIBS =
DEFS = 

SRCS = $(wildcard src/*.cpp)
SLIBS = $(wildcard lib/*.a)
INCDIR = ./include
SLIBDIR = ./lib
OBJDIR = ./obj
BINDIR = ./bin

OBJS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS)))

ifeq ($(OS),Windows_NT)
    MAIN = $(BINDIR)/)" + name + R"(.exe
else
    MAIN = $(BINDIR)/)" + name + R"(
endif

ifeq ($(BUILD),DEBUG)
	CFLAGS += -g -Wall -DDEBUG
else ifeq ($(BUILD),RELEASE)
	CFLAGS += -O2 -DNDEBUG
endif

all: $(MAIN)

$(MAIN): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CFLAGS) -o $@ $^ $(addprefix -I, $(INCDIR)) $(addprefix -L, $(SLIBDIR)) $(SLIBS) $(addprefix -D, $(DEFS)) $(LIBS)

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -c -o $@ $< $(addprefix -D, $(DEFS)) $(addprefix -I, $(INCDIR))

clean:
	rm -rf $(OBJDIR) $(MAIN)

run:
	$(MAIN)

debug: 
	@$(MAKE) BUILD=DEBUG

release:
	@$(MAKE) BUILD=RELEASE)";
}

std::string dllMakefile(std::string name, std::string version)
{
    return
R"(CXX = g++
BUILD ?= DEBUG
MAIN = bin/)" + name + R"(.dll
OTHER = bin/)" + name + R"(dll.lib
CFLAGS = )" + version + R"( -shared
LIBS =
DEFS = BUILD_DLL

SRCS = $(wildcard src/*.cpp)
SLIBS = $(wildcard lib/*.a)
INCDIR = ./include
SLIBDIR = ./lib
OBJDIR = ./obj
BINDIR = ./bin

OBJS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS)))

ifeq ($(BUILD),DEBUG)
	CFLAGS += -g -Wall -DDEBUG
else ifeq ($(BUILD),RELEASE)
	CFLAGS += -O2 -DNDEBUG
endif

all: $(MAIN)

$(MAIN): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CFLAGS) -Wl,--out-implib,$(OTHER) -o $@ $^ $(addprefix -I, $(INCDIR)) $(addprefix -L, $(SLIBDIR)) $(SLIBS) $(addprefix -D, $(DEFS)) $(LIBS)

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -c -o $@ $< $(addprefix -D, $(DEFS)) $(addprefix -I, $(INCDIR))

clean:
	rm -rf $(OBJDIR) $(MAIN)

debug: 
	@$(MAKE) BUILD=DEBUG

release:
	@$(MAKE) BUILD=RELEASE)";
}

std::string basicBuildfile(std::string name, std::string version)
{
    return
R"([project]
name = ")" + name + R"("
type = "executable"

[compiler]
cc = "g++"
ldflags = []
libs = []

[compiler.release]
cflags = [")" + version + R"(", "-O2", "-static"]
cdefs = ["-DNDEBUG"]

[compiler.debug]
cflags = [")" + version + R"(", "-g", "-Wall"]
cdefs = ["-DDEBUG"]

[paths]
src = "./src"
include = "./include"
lib = "./lib"
bin = "./bin"
obj = "./obj"

)";
}

std::string dllBuildfile(std::string name, std::string version)
{
    return
R"([project]
name = ")" + name + R"("
type = "dll"

[compiler]
cc = "g++"
ldflags = []
libs = []

[compiler.release]
cflags = [")" + version + R"(", "-shared", "-O2"]
cdefs = ["-DNDEBUG", "-DBUILD_DLL"]

[compiler.debug]
cflags = [")" + version + R"(", "-shared", "-g", "-Wall"]
cdefs = ["-DDEBUG", "-DBUILD_DLL"]

[paths]
src = "./src"
include = "./include"
lib = "./lib"
bin = "./bin"
obj = "./obj"

)";
}

std::string libBuildfile(std::string name, std::string version)
{
    return
R"([project]
name = ")" + name + R"("
type = "lib"

[compiler]
cc = "g++"
ldflags = []
libs = []

[compiler.release]
cflags = [")" + version + R"(", "-O2"]
cdefs = ["-DNDEBUG"]

[compiler.debug]
cflags = [")" + version + R"(", "-g", "-Wall"]
cdefs = ["-DDEBUG"]

[paths]
src = "./src"
include = "./include"
lib = "./lib"
bin = "./bin"
obj = "./obj"

)";
}

void MakeProgramFiles(std::string name, int type, int mType, int version)
{
    std::string project_path = "./" + name;

    std::cout << color(Green) << "Maing template at " << project_path << std::endl;

    std::filesystem::create_directory(project_path);
    std::filesystem::create_directory(project_path + "/src");
    std::filesystem::create_directory(project_path + "/lib");
    std::filesystem::create_directory(project_path + "/include");

    if (type == 0)
    {
        std::ofstream mainFile(project_path + "/src/main.cpp");
        mainFile << emptyMainFile();
        mainFile.close();
    }
    else if (type == 1)
    {
        std::string nameupper = name;
        std::transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);

        std::ofstream headderDllFile(project_path + "/include/" + name + ".h");
        headderDllFile << emptyDllHeadderFile(nameupper);
        headderDllFile.close();

        std::ofstream mainDllFile(project_path + "/src/" + name +".cpp");
        mainDllFile << emptyDllMainFile(name);
        mainDllFile.close();
    }
    else
    {
        std::string nameupper = name;
        std::transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);

        std::ofstream headderLibFile(project_path + "/include/" + name + ".h");
        headderLibFile << emptyLibHeadderFile(nameupper);
        headderLibFile.close();

        std::ofstream mainLibFile(project_path + "/src/" + name +".cpp");
        mainLibFile << emptyLibMainFile(name);
        mainLibFile.close();
    }

    std::string versionName;

    switch (version)
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

    if (mType == 0)
    {
        std::ofstream makeFile(project_path + "/makefile");

        if (type == 0)
            makeFile << basicMakefile(name, versionName);
        else if (type == 1)
            makeFile << dllMakefile(name, versionName);

        makeFile.close();
    }
    else
    {
        std::ofstream buildFile(project_path + "/build.toml");

        if (type == 0)
            buildFile << basicBuildfile(name, versionName);
        else if (type == 1)
            buildFile << dllBuildfile(name, versionName);
        else
            buildFile << libBuildfile(name, versionName);

        buildFile.close();
    }

    std::cout << color(Green) << "Done." << color(Defult) << std::endl;
}

void genorate()
{
    std::cout << color(Blue) << "?" << color(Defult) << " Project Name >> ";
    std::string name;
    std::getline(std::cin, name);

    if (name.empty())
    {
        std::cout << color(Red) << "Error no name given" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << color(Gray) << "Types [bin, dll, lib]" << color(Defult) << std::endl;
    std::cout << color(Blue) << "?" << color(Defult) << " Project Type >> ";
    std::string result;
    std::getline(std::cin, result);

    if (result.empty())
    {
        std::cout << color(Red) << "Error no type given" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    int type;

    if (result == "bin")
        type = 0;
    else if (result == "dll")
        type = 1;
    else    
        type = 2;
    
    std::cout << color(Gray) << "Build systems [make, cpb]" << color(Defult) << std::endl;
    std::cout << color(Blue) << "?" << color(Defult) << " Project Build Type >> ";
    std::string build;
    std::getline(std::cin, build);

    if (build.empty())
    {
        std::cout << color(Red) << "Error no build type given" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    int bType = build == "make" ? 0 : 1;

    std::cout << color(Gray) << "Options [89, 03, 11, 14, 17, 20]" << color(Defult) << std::endl;
    std::cout << color(Blue) << "?" << color(Defult) << " Project C++ Version >> ";
    std::string version;
    std::getline(std::cin, version);

    if (version.empty())
    {
        std::cout << color(Red) << "Error no version given" << color(Defult) << std::endl;
        exit(EXIT_FAILURE);
    }

    int cpp_version = std::stoi(version);

    MakeProgramFiles(name, type, bType, cpp_version);
}

#endif