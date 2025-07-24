UNAME    := $(shell uname -s)
PROG_EXT :=
LIB_EXT  := dylib
CFLAGS   := -x objective-c -DSOKOL_METAL -fenable-matrix \
		    -framework Metal -framework Cocoa -framework IOKit \
		    -framework MetalKit -framework Quartz -framework AudioToolbox
ARCH	 := $(shell uname -m)
ifeq ($(ARCH),arm64)
	ARCH:=osx_arm64
else
	ARCH:=osx
endif
SHDC_FLAGS := metal_macos

SOURCE := $(wildcard src/*.c)
SCENES := $(wildcard scenes/*.c)
EXE    := build/rpg_$(ARCH)$(PROG_EXT)
LIB    := build/librpg_$(ARCH).$(LIB_EXT)
INC    := -Iinclude -Iscenes -Lbuild -Ideps

ARCH_PATH   := ./bin/$(ARCH)
SHDC_PATH   := $(ARCH_PATH)/sokol-shdc$(PROG_EXT)
SHADERS     := $(wildcard shaders/*.glsl)
SHADER_OUTS := $(patsubst shaders/%,src/%.h,$(SHADERS))

.SECONDEXPANSION:
SHADER 	   := $(patsubst src/%.h,shaders/%,$@)
SHADER_OUT := $@
%.glsl.h: $(SHADERS)
	$(SHDC_PATH) -i $(SHADER) -o $(SHADER_OUT) -l $(SHDC_FLAGS)

shaders: $(SHADER_OUTS)

app: shaders
	$(CC) $(INC) $(CFLAGS) $(SOURCE) $(SCENES) -o $(EXE)

default: app

.PHONY: default all app shaders
