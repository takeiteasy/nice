NAME := nice
UNAME := $(shell uname -s)
PROG_EXT :=
LIB_EXT := dylib
STATIC_LIB_EXT := a
CFLAGS := -x objective-c++ -DSOKOL_METAL -fenable-matrix \
		  -framework Metal -framework Cocoa -framework IOKit \
		  -framework MetalKit -framework Quartz -framework AudioToolbox
ARCH := $(shell uname -m)
ifeq ($(ARCH),arm64)
	ARCH:=osx_arm64
else
	ARCH:=osx
endif
SHDC_FLAGS := metal_macos
CXX := clang++
IGNORE := -Wno-c99-designator -Wno-reorder-init-list -Wno-arc-bridge-casts-disallowed-in-nonarc
CXXFLAGS := -std=c++17 -arch arm64 $(IGNORE)
LDFLAGS := -arch arm64

default: nice

all: clean shaders lua nicepkg nice 

BUILD_DIR := build
ASSET_DIR := assets

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

builddir: $(BUILD_DIR)

SOURCE := $(wildcard src/*.cpp) deps/fmt/format.cc deps/fmt/os.cc deps/imgui/backends/imgui_impl_metal.mm
EXE := $(BUILD_DIR)/$(NAME)_$(ARCH)$(PROG_EXT)
INC := $(CXXFLAGS) -Iscenes -Isrc -Ideps -Ideps/flecs -Ideps/imgui $(LDFLAGS)

SHADERS_SRC := shaders
SHADER_DST := $(BUILD_DIR)
ARCH_PATH := bin/$(ARCH)
SHDC_PATH := $(ARCH_PATH)/sokol-shdc$(PROG_EXT)
SHADERS := $(wildcard $(SHADERS_SRC)/*.glsl)
SHADER_OUTS := $(patsubst $(SHADERS_SRC)/%,$(SHADER_DST)/%.h,$(SHADERS))

.SECONDEXPANSION:
SHADER_OUT := $@
$(SHADER_DST)/%.glsl.h: $(SHADERS_SRC)/%.glsl
	$(SHDC_PATH) -i $< -o $@ -l $(SHDC_FLAGS)

shaders: builddir $(SHADER_OUTS)

DAT_H := $(BUILD_DIR)/nice.dat.h
DAT_SRC := src/setup.lua
LUA := $(BUILD_DIR)/lua$(PROG_EXT)

$(DAT_H): $(DAT_SRC)
	./$(LUA) tools/embed.lua $(DAT_SRC) > $(DAT_H)

dat: lua $(DAT_H)

lua: builddir
	$(CC) -o $(LUA) -Ideps -DLUA_MAKE_LUA deps/minilua.c

FLECS_LIB := $(BUILD_DIR)/libflecs_$(ARCH).$(LIB_EXT)

$(FLECS_LIB): builddir
	$(CC) -shared -fpic -Ideps/flecs -Ideps deps/flecs/*.c deps/minilua.c -o $(FLECS_LIB)

flecs: FLECS_LIB

$(EXE): builddir $(FLECS_LIB)
	$(CXX) $(INC) $(CFLAGS) $(SOURCE) -I$(SHADER_DST) -L$(BUILD_DIR) -lflecs_$(ARCH) -o $(EXE)

nice: $(EXE)

NICEPKG := $(BUILD_DIR)/nicepkg.$(LIB_EXT)

$(NICEPKG): builddir shaders dat
	$(CC) -shared -fpic -o $(NICEPKG) tools/nicepkg.c deps/minilua.c -Ideps

nicepkg: $(NICEPKG)

clean:
	rm -f $(EXE) || true
	rm -f $(BUILD_DIR)/lua$(PROG_EXT) || true
	rm -f $(FLECS_LIB) || true
	rm -f $(NICEPKG) || true
	rm -r $(BUILD_DIR)/*.glsl.h || true
	rm -r $(DAT_H) || true
	rm *.niceworld || true

rerun:
	./$(EXE)

run: nice rerun

.PHONY: rerun run default all clean nice shaders lua builddir
