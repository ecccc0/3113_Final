# Source and target
TARGET := game
SRCS = main.cpp lib/cs3113.cpp lib/Entity.cpp lib/Map.cpp lib/Scene.cpp lib/ShaderProgram.cpp lib/Effects.cpp scenes/LevelOne.cpp scenes/LevelTwo.cpp scenes/CombatScene.cpp scenes/StartMenu.cpp
BINARY := $(TARGET)

# OS detection - Windows MinGW doesn't have uname, so we detect Windows differently
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
    ifeq ($(UNAME_S),Darwin)
        DETECTED_OS := macOS
    else
        DETECTED_OS := Linux
    endif
endif

# Default values
CXX = g++
CXXFLAGS = -std=c++11 -O0

# Raylib configuration using pkg-config (Posix). For Windows we assume a fixed path.
RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS = $(shell pkg-config --libs raylib 2>/dev/null)

ifeq ($(DETECTED_OS),macOS)
    CXXFLAGS += -arch arm64 $(RAYLIB_CFLAGS)
    LIBS = $(RAYLIB_LIBS) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    EXEC = ./$(BINARY)
else ifeq ($(DETECTED_OS),Windows)
    CXXFLAGS += -IC:/raylib/include -mconsole
    LIBS = -LC:/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm
    BINARY := $(TARGET).exe
    EXEC = $(BINARY)
else
    LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    EXEC = ./$(BINARY)
endif

# Build rule
$(BINARY): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(BINARY) $(SRCS) $(LIBS)

# Clean rule (OS-specific)
ifeq ($(DETECTED_OS),Windows)
clean:
	if exist $(BINARY) del /f /q $(BINARY)
else
clean:
	rm -f $(BINARY)
endif

# Run rule
run: $(BINARY)
	$(EXEC)
