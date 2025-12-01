# =============================================================================
# Nice Game Engine - Makefile
# =============================================================================

# Project Configuration
# -----------------------------------------------------------------------------
NAME := nice
BUILD_DIR := build
ASSETS_DIR := assets

# Platform Detection
# -----------------------------------------------------------------------------
UNAME := $(shell uname -s)
ARCH := $(shell uname -m)
ifeq ($(ARCH),arm64)
	ARCH := osx_arm64
else
	ARCH := osx
endif

# File Extensions
# -----------------------------------------------------------------------------
PROG_EXT :=
LIB_EXT := dylib
STATIC_LIB_EXT := a

# Build Tools
# -----------------------------------------------------------------------------
CXX := clang++
CC := clang
ARCH_PATH := bin/$(ARCH)
SHDC_PATH := $(ARCH_PATH)/sokol-shdc$(PROG_EXT)

# Compiler Flags
# -----------------------------------------------------------------------------
IGNORE_WARNINGS := -Wno-c99-designator -Wno-reorder-init-list -Wno-arc-bridge-casts-disallowed-in-nonarc
CXXFLAGS := -std=c++17 -stdlib=libc++ -arch arm64 $(IGNORE_WARNINGS)
# Add Homebrew libc++ path if using Homebrew clang++ (check if /opt/homebrew/opt/llvm exists)
LLVM_LIB_PATH := $(shell if [ -d /opt/homebrew/opt/llvm/lib/c++ ]; then echo /opt/homebrew/opt/llvm/lib/c++; fi)
ifeq ($(LLVM_LIB_PATH),)
	LDFLAGS := -arch arm64 -stdlib=libc++
else
	LDFLAGS := -arch arm64 -stdlib=libc++ -L$(LLVM_LIB_PATH) -Wl,-rpath,$(LLVM_LIB_PATH)
endif

CFLAGS := -x objective-c++ -DSOKOL_METAL -fenable-matrix \
          -framework Metal -framework Cocoa -framework IOKit \
          -framework MetalKit -framework Quartz -framework AudioToolbox

DEBUG ?= 1
ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG
endif

# Include Paths
# -----------------------------------------------------------------------------
INCLUDE_PATHS := -Iscenes -Isrc -Ideps -Ideps/flecs -Ideps/imgui
INC := $(CXXFLAGS) $(INCLUDE_PATHS) $(LDFLAGS)

# Source Files
# -----------------------------------------------------------------------------
SOURCE := $(wildcard src/*.cpp) \
          deps/fmt/format.cc \
          deps/fmt/os.cc \
          deps/imgui/backends/imgui_impl_metal.mm

# Build Targets
# -----------------------------------------------------------------------------
EXE := $(BUILD_DIR)/$(NAME)_$(ARCH)$(PROG_EXT)
LUA := $(BUILD_DIR)/lua$(PROG_EXT)
FLECS_LIB := $(BUILD_DIR)/libflecs_$(ARCH).$(STATIC_LIB_EXT)
DAT_H := $(BUILD_DIR)/nice.dat.h
ASSETS := test/assets.nice

# Shader Configuration
# -----------------------------------------------------------------------------
SHADERS_SRC := assets
SHADER_DST := $(BUILD_DIR)
SHDC_FLAGS := metal_macos
SHADERS := $(wildcard $(SHADERS_SRC)/*.glsl)
SHADER_OUTS := $(patsubst $(SHADERS_SRC)/%,$(SHADER_DST)/%.h,$(SHADERS))

# Data Generation
# -----------------------------------------------------------------------------
DAT_SRC := $(ASSETS_DIR)/setup.lua \
		   $(ASSETS_DIR)/default_tilemap.qoi

# =============================================================================
# Build Rules
# =============================================================================

# Default and Meta Targets
# -----------------------------------------------------------------------------
.PHONY: default all clean run testpkg test builddir shaders dat lua flecs nicepkg nice

default: nice

all: builddir shaders lua dat flecs nicepkg testpkg nice

# Directory Creation
# -----------------------------------------------------------------------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

builddir: $(BUILD_DIR)

# Shader Compilation
# -----------------------------------------------------------------------------
.SECONDEXPANSION:
$(SHADER_DST)/%.glsl.h: $(SHADERS_SRC)/%.glsl
	$(SHDC_PATH) -i $< -o $@ -l $(SHDC_FLAGS)

shaders: builddir $(SHADER_OUTS)

# Lua Interpreter
# -----------------------------------------------------------------------------
$(LUA): builddir
	$(CC) -o $(LUA) -Ideps -DLUA_MAKE_LUA deps/minilua.c

lua: $(LUA)

# Data Embedding
# -----------------------------------------------------------------------------
$(DAT_H): $(DAT_SRC)
	./$(LUA) $(ASSETS_DIR)/embed.lua $(DAT_SRC) > $(DAT_H)

dat: $(DAT_H)

# Flecs Static Library
# -----------------------------------------------------------------------------
$(FLECS_LIB): builddir
	$(CC) -c -Ideps/flecs -Ideps deps/flecs/*.c deps/minilua.c
	ar rcs $(FLECS_LIB) *.o
	rm -f *.o

flecs: $(FLECS_LIB)

# Main Executable
# -----------------------------------------------------------------------------
$(EXE): builddir flecs
	$(CXX) $(INC) $(CFLAGS) $(SOURCE) -I$(SHADER_DST) -L$(BUILD_DIR) -lflecs_$(ARCH) -o $(EXE)

nice: $(EXE)

# Test Asset Generation
# -----------------------------------------------------------------------------

TEST_PROJECT := test/nicepkg.json
TEST_OUTPUT := test/test.nice

$(TEST_PROJECT): nicepkg
	./$(NICEPKG) -t test/tilemap.png -l test/test.lua -e test/hand.png,test/robot.png,test/boom.wav -o $(TEST_PROJECT)

$(TEST_OUTPUT): builddir $(TEST_PROJECT)
	./$(NICEPKG) -p $(TEST_PROJECT) -o $(TEST_OUTPUT)

testpkg: $(TEST_PROJECT) $(TEST_OUTPUT)

# NicePkg Tool
# -----------------------------------------------------------------------------

NICEPKG := $(BUILD_DIR)/nicepkg
NICEPKG_SRCS := $(wildcard nicepkg/*.cpp) \
          deps/fmt/format.cc \
          deps/fmt/os.cc \
          deps/imgui/backends/imgui_impl_metal.mm \
		  deps/osdialog/osdialog.c \
		  deps/osdialog/osdialog_mac.m \
		  deps/minilua.c

nicepkg: builddir
	$(CXX) $(INC) $(CFLAGS) -DNICEPKG $(NICEPKG_SRCS) -Ideps/osdialog -I$(SHADER_DST) -o $(NICEPKG)

# Run Application
# -----------------------------------------------------------------------------
run:
	./$(EXE); rm -f *.niceworld

test: testpkg nice run

# Cleanup
# -----------------------------------------------------------------------------
clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(EXE) 2>/dev/null || true
	@rm -f $(LUA) 2>/dev/null || true
	@rm -f $(FLECS_LIB) 2>/dev/null || true
	@rm -f $(NICEPKG) 2>/dev/null || true
	@rm -f $(BUILD_DIR)/*.glsl.h 2>/dev/null || true
	@rm -f $(DAT_H) 2>/dev/null || true
	@rm -rf test/assets.nice 2>/dev/null || true
	@echo "Clean complete."
