# =============================================================================
# Nice Game Engine - Makefile
# =============================================================================

# Project Configuration
# -----------------------------------------------------------------------------
NAME := nice
BUILD_DIR := build
TOOLS_DIR := assets

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
CXXFLAGS := -std=c++17 -arch arm64 $(IGNORE_WARNINGS)
LDFLAGS := -arch arm64

CFLAGS := -x objective-c++ -DSOKOL_METAL -fenable-matrix \
          -framework Metal -framework Cocoa -framework IOKit \
          -framework MetalKit -framework Quartz -framework AudioToolbox

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
FLECS_LIB := $(BUILD_DIR)/libflecs_$(ARCH).$(LIB_EXT)
NICEPKG := $(BUILD_DIR)/nicepkg.$(LIB_EXT)
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
DAT_SRC := $(TOOLS_DIR)/setup.lua

# =============================================================================
# Build Rules
# =============================================================================

# Default and Meta Targets
# -----------------------------------------------------------------------------
.PHONY: default all clean run makerun test builddir shaders dat lua flecs nicepkg nice

default: nice

all: clean builddir shaders dat lua flecs nicepkg test nice

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
$(DAT_H): $(DAT_SRC) $(LUA)
	./$(LUA) $(TOOLS_DIR)/embed.lua $(DAT_SRC) > $(DAT_H)

dat: $(DAT_H)

# Flecs Library
# -----------------------------------------------------------------------------
$(FLECS_LIB): builddir
	$(CC) -shared -fpic -Ideps/flecs -Ideps deps/flecs/*.c deps/minilua.c -o $(FLECS_LIB)

flecs: $(FLECS_LIB)

# NicePkg Library
# -----------------------------------------------------------------------------
$(NICEPKG): builddir
	$(CC) -shared -fpic -Wno-tautological-compare -o $(NICEPKG) $(TOOLS_DIR)/nicepkg.c deps/minilua.c -Ideps

nicepkg: $(NICEPKG)

# Main Executable
# -----------------------------------------------------------------------------
$(EXE): builddir shaders dat flecs
	$(CXX) $(INC) $(CFLAGS) $(SOURCE) -I$(SHADER_DST) -L$(BUILD_DIR) -lflecs_$(ARCH) -o $(EXE)

nice: $(EXE)

# Test Asset Generation
# -----------------------------------------------------------------------------
$(ASSETS): $(LUA) $(NICEPKG)
	./$(LUA) $(TOOLS_DIR)/nicepkg.lua test/hand.png -x test/tilemap.png test/test.lua -o test/assets.nice

test: $(ASSETS)

# Run Application
# -----------------------------------------------------------------------------
run:
	./$(EXE); rm -f *.niceworld

makerun: test nice run

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
