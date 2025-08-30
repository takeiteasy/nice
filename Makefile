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

default: exe

all: clean shaders exe

BUILD_DIR := build
SOURCE := $(wildcard src/*.cpp) deps/fmt/format.cc deps/fmt/os.cc deps/imgui/backends/imgui_impl_metal.mm
SCENES := $(wildcard scenes/*.cpp)
EXE := $(BUILD_DIR)/ice_$(ARCH)$(PROG_EXT)
LIB := $(BUILD_DIR)/libice_$(ARCH).$(LIB_EXT)
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

shaders: $(SHADER_OUTS)

libflecs_$(ARCH).$(LIB_EXT):
	$(CC) -shared -fpic -Ideps/flecs -Ideps deps/flecs/*.c deps/minilua.c -o $(BUILD_DIR)/libflecs_$(ARCH).$(LIB_EXT)

flecs: libflecs_$(ARCH).$(LIB_EXT)

exe: flecs
	$(CXX) $(INC) $(CFLAGS) $(SOURCE) $(SCENES) -I$(SHADER_DST) -L$(BUILD_DIR) -lflecs_$(ARCH) -o $(EXE)

clean:
	rm -f $(EXE) 
	rm -f src/*.glsl.h

rrun:
	./$(EXE)

run: exe rrun

.PHONY: rrun run default all clean exe shaders
