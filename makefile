CXX = g++
#CXX=aarch64-linux-gnu-g++

BUILD ?= DEBUG
CFLAGS = -std=c++20 -static -pthread
LIBS =
DEFS = 

SRCS = $(wildcard src/*.cpp) $(wildcard src/templates/*.cpp)
SLIBS = $(wildcard lib/*.a)
INCDIR = ./include
SLIBDIR = ./lib
OBJDIR = ./obj
BINDIR = ./bin


OBJS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS)))

ifeq ($(OS),Windows_NT)
    MAIN = $(BINDIR)/cpb.exe
else
    MAIN = $(BINDIR)/cpb
endif

ifeq ($(BUILD),DEBUG)
	CFLAGS += -g -Wall -DDEBUG
else ifeq ($(BUILD),RELEASE)
	CFLAGS += -O2 -DNDEBUG
endif

all: $(MAIN)

$(MAIN): $(OBJS)
ifeq ($(OS),Windows_NT)
	@if not exist "$(BINDIR)" mkdir "$(BINDIR)"
else
	@mkdir -p $(BINDIR)
endif
	$(CXX) $(CFLAGS) -o $@ $^ $(addprefix -I, $(INCDIR)) $(addprefix -L, $(SLIBDIR)) $(SLIBS) $(addprefix -D, $(DEFS)) $(LIBS)

$(OBJDIR)/%.o: src/%.cpp
ifeq ($(OS),Windows_NT)
	@if not exist "$(@D)" mkdir "$(@D)"
else
	@mkdir -p $(@D)
endif
	$(CXX) $(CFLAGS) -c -o $@ $< $(addprefix -D, $(DEFS)) $(addprefix -I, $(INCDIR))

$(OBJDIR)/%.o: src/templates/%.cpp
ifeq ($(OS),Windows_NT)
	@if not exist "$(@D)" mkdir "$(@D)"
else
	@mkdir -p $(@D)
endif
	$(CXX) $(CFLAGS) -c -o $@ $< $(addprefix -D, $(DEFS)) $(addprefix -I, $(INCDIR))

clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(OBJDIR)" rmdir /s /q "$(OBJDIR)"
	@if exist "$(BINDIR)" rmdir /s /q "$(BINDIR)"
else
	@rm -rf $(OBJDIR) $(BINDIR)
endif

run:
	$(MAIN)

debug: 
	@$(MAKE) BUILD=DEBUG

release:
	@$(MAKE) BUILD=RELEASE