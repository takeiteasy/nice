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
CXX = clang++
CXXFLAGS = -std=c++17 -arch arm64 -Wno-c99-designator -Wno-reorder-init-list
LDFLAGS = -arch arm64

default: exe

all: clean shaders exe

SOURCE := $(wildcard src/*.cc) deps/fmt/format.cc deps/fmt/os.cc
SCENES := $(wildcard scenes/*.cc)
EXE := build/rpg_$(ARCH)$(PROG_EXT)
LIB := build/librpg_$(ARCH).$(LIB_EXT)
INC := $(CXXFLAGS) -Iscenes -Isrc -Ideps $(LDFLAGS)

ARCH_PATH := bin/$(ARCH)
SHDC_PATH := $(ARCH_PATH)/sokol-shdc$(PROG_EXT)
SHADERS := $(wildcard shaders/*.glsl)
SHADER_OUTS := $(patsubst shaders/%,src/%.h,$(SHADERS))

.SECONDEXPANSION:
SHADER_OUT := $@
src/%.glsl.h: shaders/%.glsl
	$(SHDC_PATH) -i $< -o $@ -l $(SHDC_FLAGS)

shaders: $(SHADER_OUTS)

exe:
	$(CXX) $(INC) $(CFLAGS) $(SOURCE) $(SCENES) -o $(EXE)

clean:
	rm -f $(EXE) 
	rm -f src/*.glsl.h

rrun:
	./$(EXE)

run: exe rrun

.PHONY: rrun run default all clean exe shaders
