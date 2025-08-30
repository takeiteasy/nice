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

default: exe

all: clean shaders tools assets clean-assets exe

BUILD_DIR := build
ASSET_DIR := assets

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

builddir: $(BUILD_DIR)

SOURCE := $(wildcard src/*.cpp) deps/fmt/format.cc deps/fmt/os.cc deps/imgui/backends/imgui_impl_metal.mm
SCENES := $(wildcard scenes/*.cpp)
EXE := $(BUILD_DIR)/$(NAME)_$(ARCH)$(PROG_EXT)
LIB := $(BUILD_DIR)/lib$(NAME)_$(ARCH).$(LIB_EXT)
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

lua: builddir
	$(CC) -o $(BUILD_DIR)/lua$(PROG_EXT) -Ideps -DLUA_MAKE_LUA deps/minilua.c

TOOLS := explode to_qoa to_qoi
TOOL_SOURCES := $(patsubst %,tools/%.c,$(TOOLS))
TOOL_TARGETS := $(patsubst %,$(BUILD_DIR)/%,$(TOOLS))

$(BUILD_DIR)/%: tools/%.c
	$(CC) -o $@ $< -Ideps -std=c99

tools: builddir $(TOOL_TARGETS) lua

EXPLODE := assets/tilemap.png
explode: builddir
	./$(BUILD_DIR)/explode $(EXPLODE)

# Generate the exploded output filenames from EXPLODE inputs
EXPLODED_FILES := $(patsubst %.png,%.exploded.png,$(EXPLODE))

# Get all image files but exclude the EXPLODE source files
QOI_IN_ALL := $(wildcard $(ASSET_DIR)/*.png) \
	   	      $(wildcard $(ASSET_DIR)/*.jpg) \
	   	      $(wildcard $(ASSET_DIR)/*.jpeg) \
	   	      $(wildcard $(ASSET_DIR)/*.bmp) \
	          $(wildcard $(ASSET_DIR)/*.tga) \
	          $(wildcard $(ASSET_DIR)/*.psd) \
	          $(wildcard $(ASSET_DIR)/*.hdr) \
	          $(wildcard $(ASSET_DIR)/*.pic) \
	          $(wildcard $(ASSET_DIR)/*.pnm) \
	          $(wildcard $(ASSET_DIR)/*.gif)

# Filter out the EXPLODE source files and add the exploded output files
QOI_IN := $(sort $(filter-out $(EXPLODE),$(QOI_IN_ALL)) $(EXPLODED_FILES))

# Strip extension and add .qoi
QOI_OUTS := $(addsuffix .qoi,$(basename $(QOI_IN)))

# Pattern rules for each image format
%.qoi: %.png | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.jpg | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.jpeg | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.bmp | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.tga | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.psd | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.hdr | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.pic | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.pnm | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
%.qoi: %.gif | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<
# Special rule for exploded PNG files
%.exploded.qoi: %.exploded.png | $(BUILD_DIR)/to_qoi
	./$(BUILD_DIR)/to_qoi $<

qoi: builddir explode $(QOI_OUTS)

QOA_IN := $(wildcard $(ASSET_DIR)/*.wav) \
		  $(wildcard $(ASSET_DIR)/*.flac) \
		  $(wildcard $(ASSET_DIR)/*.ogg) \
		  $(wildcard $(ASSET_DIR)/*.mp3)

# Strip extension and add .qoa
QOA_OUTS := $(addsuffix .qoa,$(basename $(QOA_IN)))

# Pattern rules for each audio format
%.qoa: %.wav | $(BUILD_DIR)/to_qoa
	./$(BUILD_DIR)/to_qoa $<
%.qoa: %.flac | $(BUILD_DIR)/to_qoa
	./$(BUILD_DIR)/to_qoa $<
%.qoa: %.ogg | $(BUILD_DIR)/to_qoa
	./$(BUILD_DIR)/to_qoa $<
%.qoa: %.mp3 | $(BUILD_DIR)/to_qoa
	./$(BUILD_DIR)/to_qoa $<

qoa: builddir $(QOA_OUTS)

ZIP_FILES := $(QOI_OUTS) $(QOA_OUTS)

zip:
	zip -r $(ASSET_DIR)/assets.zip $(ZIP_FILES) || true

assets: builddir explode qoi qoa zip clean-assets

clean-assets:
	rm -f $(ASSET_DIR)/*.qoi
	rm -f $(ASSET_DIR)/*.qoa
	rm -f $(ASSET_DIR)/*.exploded.*

libflecs_$(ARCH).$(LIB_EXT): builddir
	$(CC) -shared -fpic -Ideps/flecs -Ideps deps/flecs/*.c deps/minilua.c -o $(BUILD_DIR)/libflecs_$(ARCH).$(LIB_EXT)

flecs: libflecs_$(ARCH).$(LIB_EXT)

$(EXE): builddir flecs
	$(CXX) $(INC) $(CFLAGS) $(SOURCE) $(SCENES) -I$(SHADER_DST) -L$(BUILD_DIR) -lflecs_$(ARCH) -o $(EXE)

exe: $(EXE)

clean:
	rm -f $(EXE) 
	rm -f src/*.glsl.h

veryclean:
	rm -rf $(BUILD_DIR)

rerun:
	./$(EXE)

run: exe rerun

.PHONY: rerun run default all clean exe shaders lua tools builddir qoi qoa zip explode assets clean-assets veryclean