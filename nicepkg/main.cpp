/* https://github.com/takeiteasy/nice 

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#include "imgui_internal.h"
#include "nice_config.h"
#include "asset_manager.hpp"
#include "osdialog/osdialog.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_time.h"
#include "sokol/util/sokol_debugtext.h"
#include "imgui.h"
#include "sokol/util/sokol_imgui.h"
#include "passthru.glsl.h"
#include "input_manager.hpp"
#include "argparse.hpp"
#include "json.hpp"
#include "sokol_gp.h"
#include "texture.hpp"
#include <cstddef>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <fstream>
#include <vector>
#include <filesystem>
#include <string>
#include <algorithm>
#include "stb_image.h"
#include "stb_image_write.h"
#include "qoi.h"
#include "dr_wav.h"
#include "dr_mp3.h"
#include "dr_flac.h"
#include "stb_vorbis.c"
#include "qoa.h"
#include "just_zip.h"
#include "./dat.h"
#undef L
#include "minilua.h"

#ifdef _WIN32
#define PATH_LIMIT 256
#else
#define PATH_LIMIT 1024
#endif

struct Tileset {
    std::string path = "";
    Texture *texture = nullptr;
    int tile_width = 0;
    int tile_height = 0;

    bool is_valid() const {
        return path.length() > 0 &&
               texture != nullptr &&
               texture->is_valid() &&
               tile_width > 0 &&
               tile_height > 0;
    }

    void unload() {
        if (texture != nullptr) {
            texture->unload();
            delete texture;
            texture = nullptr;
        }
        path = "";
        tile_width = 0;
        tile_height = 0;
    }
};

struct Camera {
    glm::vec2 position = glm::vec2(0.f);
    glm::vec2 target_position = glm::vec2(0.f);
    float zoom = 128.f;
};

struct Neighbours {
    int grid[9];
    int x, y;
};

struct Point {
   int x, y;
};

static struct {
    sg_pipeline pipeline;
    sg_pass_action pass_action;
    sg_bindings bind;
    sg_pass pass;
    sg_image color, depth;
    sg_sampler sampler;
    sg_shader shader;
    int framebuffer_width = DEFAULT_WINDOW_WIDTH;
    int framebuffer_height = DEFAULT_WINDOW_HEIGHT;
    Camera camera;
    int mouseX, mouseY;
    int worldX, worldY;
    Texture *cross = nullptr;
    Texture *white_tex = nullptr;
    Texture *red_tex = nullptr;
    Texture *green_tex = nullptr;
    Texture *red_green_tex = nullptr;
    int tile_cols;
    int tile_rows;

    bool project_loaded = false;
    bool new_project_dialog = true;
    std::string project_path = "";
    bool has_been_saved = false;

    bool is_tileset_loaded = false;
    bool show_tileset_dialog = false;
    Tileset tileset;

    bool show_autotile_dialog = false;
    std::vector<bool> grid;
    int grid_width = 32;
    int grid_height = 32;
    float tileset_scale = 4.0f;
    bool autotile_simplified = false;

    bool show_export_dialog = true;
    bool show_confirm_save_dialog = false;
    std::vector<Neighbours> tileset_masks;
    Point autotile_map[256];
    int selected_tile_x = -1;
    int selected_tile_y = -1;
    int hovered_tile_x = -1;
    int hovered_tile_y = -1;
    bool autotile_has_duplicates = false;
    bool autotile_not_empty = false;
    int default_tile_x = -1;
    int default_tile_y = -1;
    bool selecting_default_tile = false;

    std::string lua_script_path = "";
    bool is_lua_script_loaded = false;
    bool show_lua_script_dialog = false;

    std::vector<std::string> extra_files;
} state;

int framebuffer_width(void) {
    return state.framebuffer_width;
}

int framebuffer_height(void) {
    return state.framebuffer_height;
}

std::string make_relative_path(const std::string& from_dir, const std::string& to_path) {
    try {
        std::filesystem::path from(from_dir);
        std::filesystem::path to(to_path);
        return std::filesystem::relative(to, from).string();
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to make relative path: %s\n", e.what());
        return to_path; // Return absolute path on failure
    }
}

std::string make_absolute_path(const std::string& base_dir, const std::string& relative_path) {
    try {
        std::filesystem::path base(base_dir);
        std::filesystem::path rel(relative_path);
        if (rel.is_absolute()) {
            return relative_path;
        }
        return (base / rel).lexically_normal().string();
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to make absolute path: %s\n", e.what());
        return relative_path; // Return as-is on failure
    }
}

bool save_project() {
    if (state.project_path.empty()) {
        fprintf(stderr, "Error: No project path set\n");
        return false;
    }

    try {
        nlohmann::json j;
        
        // Save tileset information
        if (state.is_tileset_loaded && !state.tileset.path.empty()) {
            j["tileset_path"] = make_relative_path(state.project_path, state.tileset.path);
            j["tile_width"] = state.tileset.tile_width;
            j["tile_height"] = state.tileset.tile_height;
        } else {
            j["tileset_path"] = "";
            j["tile_width"] = 0;
            j["tile_height"] = 0;
        }
        
        // Save autotile map (array of 256 points)
        nlohmann::json autotile_array = nlohmann::json::array();
        for (int i = 0; i < 256; i++) {
            autotile_array.push_back({state.autotile_map[i].x, state.autotile_map[i].y});
        }
        j["autotile_map"] = autotile_array;
        
        // Save autotile settings
        j["default_tile_x"] = state.default_tile_x;
        j["default_tile_y"] = state.default_tile_y;
        j["autotile_simplified"] = state.autotile_simplified;
        
        // Save lua entry path
        if (state.is_lua_script_loaded && !state.lua_script_path.empty()) {
            j["lua_entry"] = make_relative_path(state.project_path, state.lua_script_path);
        } else {
            j["lua_entry"] = "";
        }
        
        // Save extra files
        nlohmann::json extra_files_array = nlohmann::json::array();
        for (const auto& file : state.extra_files) {
            extra_files_array.push_back(make_relative_path(state.project_path, file));
        }
        j["extra_files"] = extra_files_array;
        
        // Write to file
        std::string json_path = state.project_path + "/nicepkg.json";
        std::ofstream file(json_path);
        if (!file.is_open()) {
            fprintf(stderr, "Error: Failed to open file for writing: %s\n", json_path.c_str());
            return false;
        }
        
        file << j.dump(2); // Pretty print with 2 space indent
        file.close();
        
        state.has_been_saved = true;
        printf("Project saved successfully to: %s\n", json_path.c_str());
        return true;
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Error saving project: %s\n", e.what());
        return false;
    }
}

// Load project from JSON file
// Load project from JSON file
bool load_project(const std::string& json_path, bool headless = false) {
    try {
        std::ifstream file(json_path);
        if (!file.is_open()) {
            fprintf(stderr, "Error: Failed to open project file: %s\n", json_path.c_str());
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        // Get the project directory (parent directory of the JSON file)
        std::filesystem::path json_file_path(json_path);
        std::string project_dir = json_file_path.parent_path().string();
        state.project_path = project_dir;
        
        // Load tileset
        if (j.contains("tileset_path") && !j["tileset_path"].get<std::string>().empty()) {
            std::string tileset_path = make_absolute_path(project_dir, j["tileset_path"]);
            int tile_width = j.value("tile_width", 8);
            int tile_height = j.value("tile_height", 8);
            
            state.tileset.path = tileset_path;
            state.tileset.tile_width = tile_width;
            state.tileset.tile_height = tile_height;
            state.is_tileset_loaded = true;
            
            if (!headless) {
                // Load the tileset texture using the existing Texture::load() method
                std::ifstream tileset_file(tileset_path, std::ios::binary | std::ios::ate);
                if (tileset_file.is_open()) {
                    std::streamsize size = tileset_file.tellg();
                    tileset_file.seekg(0, std::ios::beg);
                    
                    std::vector<unsigned char> buffer(size);
                    if (tileset_file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                        tileset_file.close();
                        
                        // Clean up existing texture if any
                        if (state.tileset.texture != nullptr) {
                            state.tileset.texture->unload();
                            delete state.tileset.texture;
                            state.tileset.texture = nullptr;
                        }
                        
                        // Create and load texture
                        state.tileset.texture = new Texture();
                        if (state.tileset.texture->load(buffer.data(), buffer.size())) {
                            state.tile_cols = state.tileset.texture->width() / tile_width;
                            state.tile_rows = state.tileset.texture->height() / tile_height;
                            state.tileset_masks.clear();
                            state.tileset_masks.resize(state.tile_cols * state.tile_rows);
                        } else {
                            delete state.tileset.texture;
                            state.tileset.texture = nullptr;
                            fprintf(stderr, "Failed to load tileset texture: %s\n", tileset_path.c_str());
                        }
                    }
                }
            } else {
                // In headless mode, we still need tile_cols and tile_rows for autotile logic if needed
                // We can get dimensions from stbi_info without creating a texture
                int width, height, comp;
                if (stbi_info(tileset_path.c_str(), &width, &height, &comp)) {
                    state.tile_cols = width / tile_width;
                    state.tile_rows = height / tile_height;
                    state.tileset_masks.clear();
                    state.tileset_masks.resize(state.tile_cols * state.tile_rows);
                }
            }
        }
        
        // Load autotile map
        if (j.contains("autotile_map") && j["autotile_map"].is_array()) {
            auto autotile_array = j["autotile_map"];
            for (int i = 0; i < 256 && i < (int)autotile_array.size(); i++) {
                if (autotile_array[i].is_array() && autotile_array[i].size() >= 2) {
                    state.autotile_map[i].x = autotile_array[i][0];
                    state.autotile_map[i].y = autotile_array[i][1];
                }
            }
        }
        
        // Load autotile settings
        state.default_tile_x = j.value("default_tile_x", -1);
        state.default_tile_y = j.value("default_tile_y", -1);
        state.autotile_simplified = j.value("autotile_simplified", false);
        
        // Load lua entry
        if (j.contains("lua_entry") && !j["lua_entry"].get<std::string>().empty()) {
            state.lua_script_path = make_absolute_path(project_dir, j["lua_entry"]);
            state.is_lua_script_loaded = std::filesystem::exists(state.lua_script_path);
        }
        
        // Load extra files
        if (j.contains("extra_files") && j["extra_files"].is_array()) {
            state.extra_files.clear();
            for (const auto& file_path : j["extra_files"]) {
                std::string abs_path = make_absolute_path(project_dir, file_path);
                if (std::filesystem::exists(abs_path)) {
                    state.extra_files.push_back(abs_path);
                }
            }
        }
        
        state.has_been_saved = true;
        printf("Project loaded successfully from: %s\n", json_path.c_str());
        return true;
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Error loading project: %s\n", e.what());
        return false;
    }
}

std::vector<unsigned char> explode_image(
    const unsigned char* data, 
    int width, int height, int channels,
    int tile_width, int tile_height, int padding,
    int* out_width, int* out_height
) {
    if (tile_width > width || tile_height > height) {
        fprintf(stderr, "Tile size larger than image: tile size: %d,%d, image size: %d,%d\n",
                tile_width, tile_height, width, height);
        return {};
    }
    
    if (width % tile_width != 0 || height % tile_height != 0) {
        fprintf(stderr, "Tile size is not multiple of image size: tile size: %d,%d, image size: %d,%d\n",
                tile_width, tile_height, width, height);
        return {};
    }
    
    int rows = height / tile_height;
    int cols = width / tile_width;
    int new_width = (cols * tile_width) + ((cols + 1) * padding);
    int new_height = (rows * tile_height) + ((rows + 1) * padding);
    
    *out_width = new_width;
    *out_height = new_height;
    
    // Create new image data (4 channels always for output)
    std::vector<unsigned char> new_data(new_width * new_height * 4, 0);
    
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            // Source rectangle
            int src_x = x * tile_width;
            int src_y = y * tile_height;
            
            // Destination
            int dx = x * tile_width + ((x + 1) * padding);
            int dy = y * tile_height + ((y + 1) * padding);
            
            for (int tile_y = 0; tile_y < tile_height; tile_y++) {
                for (int tile_x = 0; tile_x < tile_width; tile_x++) {
                    int src_pixel_x = src_x + tile_x;
                    int src_pixel_y = src_y + tile_y;
                    int src_index = (src_pixel_y * width + src_pixel_x) * channels;
                    
                    int dst_pixel_x = dx + tile_x;
                    int dst_pixel_y = dy + tile_y;
                    int dst_index = (dst_pixel_y * new_width + dst_pixel_x) * 4;
                    
                    // Copy pixels (handle different channel counts)
                    for (int c = 0; c < std::min(channels, 4); c++) {
                        new_data[dst_index + c] = data[src_index + c];
                    }
                    // Set alpha to 255 if source doesn't have alpha
                    if (channels == 3) {
                        new_data[dst_index + 3] = 255;
                    }
                }
            }
        }
    }
    
    return new_data;
}

static std::vector<unsigned char> convert_to_qoi(
    const unsigned char* data,
    int width, int height, int channels,
    int colorspace = 0
) {
    qoi_desc desc = {
        .width = (unsigned int)width,
        .height = (unsigned int)height,
        .channels = (unsigned char)channels,
        .colorspace = (unsigned char)colorspace
    };
    
    int out_len;
    void* qoi_data = qoi_encode(data, &desc, &out_len);
    if (!qoi_data) {
        fprintf(stderr, "Failed to encode image to QOI format\n");
        return {};
    }
    
    std::vector<unsigned char> result((unsigned char*)qoi_data, (unsigned char*)qoi_data + out_len);
    free(qoi_data);
    
    return result;
}

struct AudioData {
    std::vector<short> samples;
    unsigned int channels;
    unsigned int samplerate;
    unsigned int total_samples;
};

static AudioData load_audio_file(const std::string& path) {
    AudioData result;
    
    // Determine file type by extension
    std::string ext = path.substr(path.find_last_of("."));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".wav") {
        drwav wav;
        if (!drwav_init_file(&wav, path.c_str(), NULL)) {
            fprintf(stderr, "Failed to load WAV: %s\n", path.c_str());
            return result;
        }
        
        result.channels = wav.channels;
        result.samplerate = wav.sampleRate;
        result.total_samples = (unsigned int)wav.totalPCMFrameCount;
        result.samples.resize(result.total_samples * result.channels);
        
        drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, result.samples.data());
        drwav_uninit(&wav);
        
    } else if (ext == ".ogg") {
        int channels, samplerate;
        short* data;
        int samples = stb_vorbis_decode_filename(path.c_str(), &channels, &samplerate, &data);
        
        if (samples <= 0) {
            fprintf(stderr, "Failed to load OGG: %s\n", path.c_str());
            return result;
        }
        
        result.channels = channels;
        result.samplerate = samplerate;
        result.total_samples = samples;
        result.samples.assign(data, data + samples * channels);
        free(data);
        
    } else if (ext == ".mp3") {
        drmp3 mp3;
        if (!drmp3_init_file(&mp3, path.c_str(), NULL)) {
            fprintf(stderr, "Failed to load MP3: %s\n", path.c_str());
            return result;
        }
        
        result.channels = mp3.channels;
        result.samplerate = mp3.sampleRate;
        drmp3_uint64 total_frames = drmp3_get_pcm_frame_count(&mp3);
        result.total_samples = (unsigned int)total_frames;
        
        // Load as float and convert to short
        std::vector<float> float_data(result.total_samples * result.channels);
        drmp3_read_pcm_frames_f32(&mp3, total_frames, float_data.data());
        drmp3_uninit(&mp3);
        
        result.samples.resize(result.total_samples * result.channels);
        for (size_t i = 0; i < float_data.size(); i++) {
            float sample = float_data[i];
            if (sample >= 1.0f) result.samples[i] = 32767;
            else if (sample <= -1.0f) result.samples[i] = -32768;
            else result.samples[i] = (short)(sample * 32767.0f);
        }
        
    } else if (ext == ".flac") {
        unsigned int channels, samplerate;
        drflac_uint64 total_frames;
        short* data = drflac_open_file_and_read_pcm_frames_s16(
            path.c_str(), &channels, &samplerate, &total_frames, NULL
        );
        
        if (!data) {
            fprintf(stderr, "Failed to load FLAC: %s\n", path.c_str());
            return result;
        }
        
        result.channels = channels;
        result.samplerate = samplerate;
        result.total_samples = (unsigned int)total_frames;
        result.samples.assign(data, data + result.total_samples * result.channels);
        free(data);
        
    } else {
        fprintf(stderr, "Unsupported audio format: %s\n", ext.c_str());
    }
    
    return result;
}

static std::vector<unsigned char> convert_to_qoa(const AudioData& audio) {
    if (audio.samples.empty()) {
        return {};
    }
    
    qoa_desc desc = {
        .channels = audio.channels,
        .samplerate = audio.samplerate,
        .samples = audio.total_samples
    };
    
    unsigned int out_len;
    void* qoa_data = qoa_encode(audio.samples.data(), &desc, &out_len);
    if (!qoa_data) {
        fprintf(stderr, "Failed to encode audio to QOA format\n");
        return {};
    }
    
    std::vector<unsigned char> result((unsigned char*)qoa_data, (unsigned char*)qoa_data + out_len);
    free(qoa_data);
    
    return result;
}

static bool is_image_file(const std::string& path) {
    std::string ext = path.substr(path.find_last_of("."));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || 
           ext == ".gif" || ext == ".bmp" || ext == ".tga";
}

static bool is_audio_file(const std::string& path) {
    std::string ext = path.substr(path.find_last_of("."));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac";
}

// Amalgamate Lua files using lua-amalg
static std::string amalgamate_lua_script(const std::string& main_lua_path, const std::vector<std::string>& extra_lua_files) {
    // Create Lua state
    lua_State* lua = luaL_newstate();
    if (!lua) {
        fprintf(stderr, "Failed to create Lua state for amalgamation\n");
        return "";
    }
    
    luaL_openlibs(lua);
    
    // Load the amalg.lua script using dofile to ensure isscript() returns false
    // and we get the module table back
    std::string load_script = "return dofile('deps/lua-amalg/amalg.lua')";
    if (luaL_dostring(lua, load_script.c_str()) != LUA_OK) {
        fprintf(stderr, "Failed to load amalg.lua: %s\n", lua_tostring(lua, -1));
        lua_close(lua);
        return "";
    }
    
    // Get the amalg module (the script should return a table)
    if (!lua_istable(lua, -1)) {
        fprintf(stderr, "amalg.lua did not return a table\n");
        lua_close(lua);
        return "";
    }
    
    // Get the amalgamate function from the module
    lua_getfield(lua, -1, "amalgamate");
    if (!lua_isfunction(lua, -1)) {
        fprintf(stderr, "amalg.amalgamate is not a function\n");
        lua_close(lua);
        return "";
    }
    
    // Create temporary output file
    std::string temp_lua = std::tmpnam(nullptr);
    temp_lua += ".lua";
    
    // Build arguments for amalgamate function: -o, output_path, -s, script_path, [extra files...]
    int args_count = 0;
    
    lua_pushstring(lua, "-o");
    lua_pushstring(lua, temp_lua.c_str());
    args_count += 2;
    
    lua_pushstring(lua, "-s");
    lua_pushstring(lua, main_lua_path.c_str());
    args_count += 2;
    
    // Add extra Lua files as modules
    // Convert file paths to module names: "src/foo.lua" -> "src.foo"
    for (const auto& file : extra_lua_files) {
        std::string module_name = file;
        
        // Make relative if absolute (assuming relative to CWD)
        if (std::filesystem::path(module_name).is_absolute()) {
            try {
                module_name = std::filesystem::relative(module_name).string();
            } catch (...) {
                // Keep absolute if relative fails
            }
        }
        
        // Remove extension
        size_t last_dot = module_name.find_last_of(".");
        if (last_dot != std::string::npos) {
            module_name = module_name.substr(0, last_dot);
        }
        
        // Replace separators with dots
        std::replace(module_name.begin(), module_name.end(), '/', '.');
        std::replace(module_name.begin(), module_name.end(), '\\', '.');
        
        lua_pushstring(lua, module_name.c_str());
        args_count++;
    }
    
    // Call amalgamate
    if (lua_pcall(lua, args_count, 0, 0) != LUA_OK) {
        fprintf(stderr, "Failed to amalgamate Lua script: %s\n", lua_tostring(lua, -1));
        lua_close(lua);
        return "";
    }
    
    lua_close(lua);
    
    // Verify the output file was created
    if (!std::filesystem::exists(temp_lua)) {
        fprintf(stderr, "Amalgamation did not create output file: %s\n", temp_lua.c_str());
        return "";
    }
    
    printf("Lua amalgamation complete: %s\n", temp_lua.c_str());
    return temp_lua;
}

static bool export_package(
    const std::string& output_path,
    const std::string& tileset_path,
    int tile_width, int tile_height,
    const std::string& autotile_json_path,
    const std::string& lua_script_path,
    const std::vector<std::string>& extra_files
) {
    printf("Exporting package to: %s\n", output_path.c_str());
    
    // Create a zip file
    zip* z = zip_open(output_path.c_str(), "w");
    if (!z) {
        fprintf(stderr, "Failed to create zip file: %s\n", output_path.c_str());
        return false;
    }
    
    // Process tileset - explode and convert to QOI
    if (!tileset_path.empty()) {
        printf("Processing tileset: %s\n", tileset_path.c_str());
        
        // Load the tileset image
        int img_width, img_height, img_channels;
        unsigned char* img_data = stbi_load(tileset_path.c_str(), &img_width, &img_height, &img_channels, 4);
        
        if (!img_data) {
            fprintf(stderr, "Failed to load tileset: %s\n", tileset_path.c_str());
            zip_close(z);
            return false;
        }
        
        // Explode the tileset (add padding between tiles)
        int exploded_width, exploded_height;
        auto exploded_data = explode_image(
            img_data, img_width, img_height, 4,
            tile_width, tile_height, 4, // 4 pixel padding
            &exploded_width, &exploded_height
        );
        stbi_image_free(img_data);
        
        if (exploded_data.empty()) {
            fprintf(stderr, "Failed to explode tileset\n");
            zip_close(z);
            return false;
        }
        
        // Convert to QOI
        auto qoi_data = convert_to_qoi(
            exploded_data.data(),
            exploded_width, exploded_height, 4,
            0 // sRGB colorspace
        );
        
        if (qoi_data.empty()) {
            fprintf(stderr, "Failed to convert tileset to QOI\n");
            zip_close(z);
            return false;
        }
        
        // Write to a temporary file then add to zip
        std::string temp_qoi = std::tmpnam(nullptr);
        temp_qoi += ".qoi";
        std::ofstream qoi_file(temp_qoi, std::ios::binary);
        if (!qoi_file.is_open()) {
            fprintf(stderr, "Failed to create temporary QOI file\n");
            zip_close(z);
            return false;
        }
        qoi_file.write((char*)qoi_data.data(), qoi_data.size());
        qoi_file.close();
        
        // Add to zip as "tileset.qoi"
        FILE* f = fopen(temp_qoi.c_str(), "rb");
        if (!f || !zip_append_file_ex(z, temp_qoi.c_str(), "tileset.qoi", f, 6)) {
            if (f) fclose(f);
            fprintf(stderr, "Failed to add tileset to zip\n");
            std::remove(temp_qoi.c_str());
            zip_close(z);
            return false;
        }
        fclose(f);
        std::remove(temp_qoi.c_str());
        
        printf("Added tileset.qoi to package\n");
    }
    
    // Add autotile JSON if it exists
    if (!autotile_json_path.empty() && std::filesystem::exists(autotile_json_path)) {
        printf("Adding autotile config: %s\n", autotile_json_path.c_str());
        FILE* f = fopen(autotile_json_path.c_str(), "rb");
        if (!f || !zip_append_file_ex(z, autotile_json_path.c_str(), "autotile.json", f, 6)) {
            if (f) fclose(f);
            fprintf(stderr, "Failed to add autotile config to zip\n");
            zip_close(z);
            return false;
        }
        fclose(f);
    }

    // Process extra files
    std::vector<std::string> extra_lua_files;
    
    for (const auto& file_path : extra_files) {
        if (!std::filesystem::exists(file_path)) {
            fprintf(stderr, "Warning: Extra file not found: %s\n", file_path.c_str());
            continue;
        }
        
        std::string filename = std::filesystem::path(file_path).filename().string();
        
        // Check if it's a Lua file
        if (file_path.substr(file_path.find_last_of(".")) == ".lua") {
            // Don't include the main script if it's in extra_files
            if (std::filesystem::absolute(file_path) != std::filesystem::absolute(lua_script_path)) {
                extra_lua_files.push_back(file_path);
            }
            continue;
        }
        
        if (is_image_file(file_path)) {
            // Convert to QOI
            int width, height, channels;
            unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &channels, 0);
            if (!data) {
                fprintf(stderr, "Failed to load image: %s\n", file_path.c_str());
                continue;
            }
            
            auto qoi_data = convert_to_qoi(data, width, height, channels, 0); // 0 for sRGB
            stbi_image_free(data);
            
            if (qoi_data.empty()) {
                fprintf(stderr, "Failed to convert image to QOI: %s\n", file_path.c_str());
                continue;
            }
            
            // Change extension to .qoi
            std::string qoi_name = std::filesystem::path(filename).replace_extension(".qoi").string();
            
            // Write to temp file
            std::string temp_qoi = std::tmpnam(nullptr);
            temp_qoi += ".qoi";
            std::ofstream qoi_file(temp_qoi, std::ios::binary);
            qoi_file.write((char*)qoi_data.data(), qoi_data.size());
            qoi_file.close();
            
            // Add to zip
            FILE* f = fopen(temp_qoi.c_str(), "rb");
            if (f) {
                zip_append_file_ex(z, temp_qoi.c_str(), qoi_name.c_str(), f, 6);
                fclose(f);
            }
            std::remove(temp_qoi.c_str());
            printf("Added %s\n", qoi_name.c_str());
            
        } else if (is_audio_file(file_path)) {
            // Convert to QOA
            AudioData audio_data = load_audio_file(file_path);
            
            if (audio_data.samples.empty()) {
                fprintf(stderr, "Failed to load audio: %s\n", file_path.c_str());
                continue;
            }
            
            auto qoa_data = convert_to_qoa(audio_data);
            
            if (qoa_data.empty()) {
                fprintf(stderr, "Failed to convert audio to QOA: %s\n", file_path.c_str());
                continue;
            }
            
            // Change extension to .qoa
            std::string qoa_name = std::filesystem::path(filename).replace_extension(".qoa").string();
            
            // Write to temp file
            std::string temp_qoa = std::tmpnam(nullptr);
            temp_qoa += ".qoa";
            std::ofstream qoa_file(temp_qoa, std::ios::binary);
            qoa_file.write((char*)qoa_data.data(), qoa_data.size());
            qoa_file.close();
            
            // Add to zip
            FILE* f = fopen(temp_qoa.c_str(), "rb");
            if (f) {
                zip_append_file_ex(z, temp_qoa.c_str(), qoa_name.c_str(), f, 6);
                fclose(f);
            }
            std::remove(temp_qoa.c_str());
            printf("Added %s\n", qoa_name.c_str());
            
        } else {
            // Add as is
            FILE* f = fopen(file_path.c_str(), "rb");
            if (f) {
                zip_append_file_ex(z, file_path.c_str(), filename.c_str(), f, 6);
                fclose(f);
                printf("Added %s\n", filename.c_str());
            }
        }
    }
    
    // Add Lua script - amalgamate it first
    std::string amalgamated_lua;
    if (!lua_script_path.empty() && std::filesystem::exists(lua_script_path)) {
        printf("Amalgamating Lua script: %s\n", lua_script_path.c_str());
        amalgamated_lua = amalgamate_lua_script(lua_script_path, extra_lua_files);
        
        if (amalgamated_lua.empty()) {
            fprintf(stderr, "Failed to amalgamate Lua script\n");
            zip_close(z);
            return false;
        }
        
        printf("Adding Lua script: %s\n", amalgamated_lua.c_str());
        FILE* f = fopen(amalgamated_lua.c_str(), "rb");
        if (!f || !zip_append_file_ex(z, amalgamated_lua.c_str(), "main.lua", f, 6)) {
            if (f) fclose(f);
            fprintf(stderr, "Failed to add Lua script to zip\n");
            std::remove(amalgamated_lua.c_str());
            zip_close(z);
            return false;
        }
        fclose(f);
    }
    
    // Clean up amalgamated Lua file
    if (!amalgamated_lua.empty()) {
        std::remove(amalgamated_lua.c_str());
    }
    
    zip_close(z);
    printf("Package created successfully: %s\n", output_path.c_str());
    return true;
}

// Save autotile configuration to a temporary JSON file for export
std::string save_autotile_json() {
    try {
        nlohmann::json j;
        
        // Save autotile map (array of 256 points)
        nlohmann::json autotile_array = nlohmann::json::array();
        for (int i = 0; i < 256; i++) {
            autotile_array.push_back({state.autotile_map[i].x, state.autotile_map[i].y});
        }
        j["autotile_map"] = autotile_array;
        
        // Save autotile settings
        j["default_tile_x"] = state.default_tile_x;
        j["default_tile_y"] = state.default_tile_y;
        j["autotile_simplified"] = state.autotile_simplified;
        j["tile_width"] = state.tileset.tile_width;
        j["tile_height"] = state.tileset.tile_height;
        
        // Create a temporary file
        std::string temp_path = std::tmpnam(nullptr);
        temp_path += ".json";
        
        std::ofstream file(temp_path);
        if (!file.is_open()) {
            fprintf(stderr, "Error: Failed to create temporary autotile JSON file\n");
            return "";
        }
        
        file << j.dump(2);
        file.close();
        
        return temp_path;
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Error saving autotile JSON: %s\n", e.what());
        return "";
    }
}

static void framebuffer_resize(int width, int height) {
    if (sg_query_image_state(state.color) == SG_RESOURCESTATE_VALID)
        sg_destroy_image(state.color);
    if (sg_query_image_state(state.depth) == SG_RESOURCESTATE_VALID)
        sg_destroy_image(state.depth);
    sg_image_desc img_desc = {
        .usage.render_attachment = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8
    };
    state.color = sg_make_image(&img_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    state.depth = sg_make_image(&img_desc);
    sg_attachments_desc attr_desc = {
        .colors[0].image = state.color,
        .depth_stencil.image = state.depth
    };
    state.pass = (sg_pass) {
        .attachments = sg_make_attachments(&attr_desc),
        .action = state.pass_action
    };
    state.bind.images[IMG_tex] = state.color;
    state.bind.samplers[SMP_smp] = state.sampler;
}

struct PassThruVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

static void init(void) {
    sg_desc desc = {
        .environment = sglue_environment(),
        .buffer_pool_size = (1<<16)-1,
        .logger.func = slog_func,
    };
    sg_setup(&desc);
    sdtx_desc_t dtx_desc = {
        .fonts = { sdtx_font_oric() }
    };
    sdtx_setup(&dtx_desc);
    stm_setup();

    sgp_desc sgpdesc = {
        .color_format = SG_PIXELFORMAT_RGBA8,
        .depth_format = SG_PIXELFORMAT_DEPTH
    };
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid()) {
        fprintf(stderr, "Failed to create Sokol GP context: %s\n", sgp_get_error_message(sgp_get_last_error()));
        exit(-1);
    }

    state.shader = sg_make_shader(passthru_shader_desc(sg_query_backend()));

    sg_pipeline_desc pip_desc = {
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .shader = state.shader,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers[0].stride = sizeof(PassThruVertex),
            .attrs = {
                [ATTR_passthru_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_passthru_texcoord].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .cull_mode = SG_CULLMODE_BACK,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        }
    };
    state.pipeline = sg_make_pipeline(&pip_desc);

    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { .9f, .9f, .9f, 1.f }
        }
    };

    sg_sampler_desc smp_desc = {
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
        .wrap_v = SG_WRAP_CLAMP_TO_BORDER,
    };
    state.sampler = sg_make_sampler(&smp_desc);

    framebuffer_resize(state.framebuffer_width, state.framebuffer_height);

    float vertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, // Top-left
         1.0f,  1.0f, 1.0f, 0.0f, // Top-right
         1.0f, -1.0f, 1.0f, 1.0f, // Bottom-right
        -1.0f, -1.0f, 0.0f, 1.0f, // Bottom-left
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    sg_buffer_desc v_desc = {
        .data = SG_RANGE(vertices)
    };
    state.bind.vertex_buffers[0] = sg_make_buffer(&v_desc);
    sg_buffer_desc i_desc = {
        .usage.index_buffer = true,
        .data = SG_RANGE(indices)
    };
    state.bind.index_buffer = sg_make_buffer(&i_desc);

    simgui_desc_t simgui_desc = { };
    simgui_desc.logger.func = slog_func;
    simgui_setup(&simgui_desc);

    // Initialize the dummy map with default grid
    // Initialize the dummy map with default grid
    state.grid.resize(state.grid_width * state.grid_height, false);
    // Center camera on the grid (grid is 64x64 tiles of 8x8 pixels = 512x512, center is 256,256)
    state.camera.position = glm::vec2(0.f, 0.f);
    state.camera.target_position = glm::vec2(0.f, 0.f);
    
    state.cross = new Texture();
    assert(state.cross->load(x_png, x_png_size));
    state.white_tex = new Texture();
    assert(state.white_tex->load(white_png, white_png_size));
    state.red_tex = new Texture();
    assert(state.red_tex->load(red_png, red_png_size));
    state.green_tex = new Texture();
    assert(state.green_tex->load(green_png, green_png_size));
    state.red_green_tex = new Texture();
    assert(state.red_green_tex->load(red_green_png, red_green_png_size));
}

static void SlimButton(const char *label, std::function<void()> callback = nullptr) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 originalFramePadding = style.FramePadding;
    style.FramePadding.y = 0.f;
    if (ImGui::Button(label))
        if (callback != nullptr)
            callback();
    style.FramePadding = originalFramePadding;
}

static uint8_t _bitmask(Neighbours *mask) {
    if (!state.autotile_simplified) {
#define CHECK_CORNER(N, A, B) \
        mask->grid[(N)] = !mask->grid[(A)] || !mask->grid[(B)] ? 0 : mask->grid[(N)];
        CHECK_CORNER(0, 1, 3);
        CHECK_CORNER(2, 1, 5);
        CHECK_CORNER(6, 7, 3);
        CHECK_CORNER(8, 7, 5);
#undef CHECK_CORNER
    }

    uint8_t result = 0;
    for (int y = 0, n = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            if (!(y == 1 && x == 1))
                result += (mask->grid[y * 3 + x] << n++);
    return result;
}

static void draw_tileset(ImDrawList* dl, ImVec2 min, ImVec2 max) {
    if (state.tileset.texture == nullptr || !state.tileset.texture->is_valid())
        return;
    if (state.tileset.tile_width <= 0 || state.tileset.tile_height <= 0)
        return;
    sg_image tileset_img = *state.tileset.texture;
    if (tileset_img.id == SG_INVALID_ID || sg_query_image_state(tileset_img) != SG_RESOURCESTATE_VALID)
        return;
    dl->AddImage(simgui_imtextureid(*state.tileset.texture), min, max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE);

    int tex_width = state.tileset.texture->width();
    int tex_height = state.tileset.texture->height();
    int tile_w = state.tileset.tile_width;
    int tile_h = state.tileset.tile_height;
    float scale_x = (max.x - min.x) / tex_width;
    float scale_y = (max.y - min.y) / tex_height;
    ImVec2 mouse_pos = ImGui::GetMousePos();

    for (int ty = 0; ty < state.tile_rows; ty++)
        for (int tx = 0; tx < state.tile_cols; tx++) {
            Neighbours *n = &state.tileset_masks[ty * state.tile_cols + tx];
            float dx = ((tx * tile_w)*scale_x);
            float dy = ((ty * tile_h)*scale_y);
            int gw = tile_w*scale_x;
            int gh = tile_h*scale_y;
            float ex = gw/3, ey = gh/3;
            for (int yy = 0; yy < 3; yy++)
                for (int xx = 0; xx < 3; xx++)
                    if (n->grid[yy*3+xx]) {
                        ImVec2 tmin = {min.x+dx+(xx*ex), min.y+dy+(yy*ey)};
                        ImVec2 tmax = {tmin.x+ex, tmin.y+ey};
                        dl->AddImage(simgui_imtextureid(*state.cross), tmin, tmax);
                    }
        }

    if (ImGui::IsWindowHovered() && mouse_pos.x >= min.x && mouse_pos.x < max.x && mouse_pos.y >= min.y && mouse_pos.y < max.y) {
        int tx = (int)((mouse_pos.x - min.x) / (tile_w * scale_x));
        int ty = (int)((mouse_pos.y - min.y) / (tile_h * scale_y));

        if (tx >= 0 && tx < state.tile_cols && ty >= 0 && ty < state.tile_rows) {
            state.hovered_tile_x = tx;
            state.hovered_tile_y = ty;
            ImVec2 t_min = ImVec2(min.x + tx * tile_w * scale_x, min.y + ty * tile_h * scale_y);
            ImVec2 t_max = ImVec2(t_min.x + tile_w * scale_x, t_min.y + tile_h * scale_y);
            dl->AddImage(simgui_imtextureid(state.selecting_default_tile ? *state.green_tex : *state.white_tex), t_min, t_max);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (state.selecting_default_tile) {
                    state.default_tile_x = tx;
                    state.default_tile_y = ty;
                    state.selecting_default_tile = false;
                } else {
                    state.selected_tile_x = tx;
                    state.selected_tile_y = ty;
                }
            }
        }
    } else {
        state.hovered_tile_x = -1;
        state.hovered_tile_y = -1;
    }

    if ((state.selected_tile_x >= 0 && state.selected_tile_y >= 0 && state.default_tile_x >= 0 && state.default_tile_y >= 0) && state.selected_tile_x == state.default_tile_x && state.selected_tile_y == state.default_tile_y) {
        ImVec2 t_min = ImVec2(min.x + state.default_tile_x * tile_w * scale_x, min.y + state.default_tile_y * tile_h * scale_y);
        ImVec2 t_max = ImVec2(t_min.x + tile_w * scale_x, t_min.y + tile_h * scale_y);
        dl->AddImage(simgui_imtextureid(*state.red_green_tex), t_min, t_max);
    } else {
        if (!state.selecting_default_tile && state.default_tile_x >= 0 && state.default_tile_y >= 0) {
            ImVec2 t_min = ImVec2(min.x + state.default_tile_x * tile_w * scale_x, min.y + state.default_tile_y * tile_h * scale_y);
            ImVec2 t_max = ImVec2(t_min.x + tile_w * scale_x, t_min.y + tile_h * scale_y);
            dl->AddImage(simgui_imtextureid(*state.green_tex), t_min, t_max);
        }
        if (state.selected_tile_x >= 0 && state.selected_tile_y >= 0) {
            ImVec2 t_min = ImVec2(min.x + state.selected_tile_x * tile_w * scale_x, min.y + state.selected_tile_y * tile_h * scale_y);
            ImVec2 t_max = ImVec2(t_min.x + tile_w * scale_x, t_min.y + tile_h * scale_y);
            dl->AddImage(simgui_imtextureid(*state.red_tex), t_min, t_max);
        }
    }
}

static void rebuild_autotile_map() {
    for (int i = 0; i < 256; i++)
        state.autotile_map[i] = (Point){-1, -1};
    for (int ty = 0; ty < state.tile_rows; ty++)
        for (int tx = 0; tx < state.tile_cols; tx++) {
            Neighbours *n = &state.tileset_masks[ty * state.tile_cols + tx];
            if (n->grid[4] == 0)
                continue;
            int bmask = _bitmask(n);
            state.autotile_map[bmask] = (Point){tx, ty};
        }
}

static void frame(void) {
    int width = sapp_width();
    int height = sapp_height();
    float half_width = static_cast<float>(width) / 2.0f;
    float half_height = static_cast<float>(height) / 2.0f;
 
    float aspectRatio = width/(float)height;
    float viewVolumeLeft = -state.camera.zoom * aspectRatio;
    float viewVolumeRight = state.camera.zoom * aspectRatio;
    float viewVolumeBottom = -state.camera.zoom;
    float viewVolumeTop = state.camera.zoom;
    float adjustedLeft = viewVolumeLeft + state.camera.position.x;
    float adjustedRight = viewVolumeRight + state.camera.position.x;
    float adjustedBottom = viewVolumeBottom + state.camera.position.y;
    float adjustedTop = viewVolumeTop + state.camera.position.y;
    sgp_begin(width, height);
    sgp_viewport(0, 0, width, height);
    sgp_project(adjustedLeft, adjustedRight, adjustedBottom, adjustedTop);
    sgp_set_color(1, 0, 0, 1);
    sgp_clear();
    sgp_reset_color();
    sgp_set_blend_mode(SGP_BLENDMODE_BLEND);
    
    simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });

    if (state.new_project_dialog)
        ImGui::OpenPopup("New Project");

    if (state.show_confirm_save_dialog)
        ImGui::OpenPopup("Save Project");

    if (state.show_tileset_dialog)
        ImGui::OpenPopup("Load Tileset");

    if (state.show_lua_script_dialog)
        ImGui::OpenPopup("Load Lua Script");

#define CENTRE_MODAL \
        ImGui::SetWindowPos((ImVec2){(width - half_width) / 2.0f, (height - half_height) / 2.0f}, ImGuiCond_Always); \
        ImGui::SetWindowSize((ImVec2){half_width, half_height}, ImGuiCond_Always);

    if (ImGui::BeginPopupModal("New Project", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        CENTRE_MODAL;
        ImGui::Text("New Project");
        ImGui::Separator();
        static char project_path[PATH_LIMIT] = "";
        static bool project_first_time = true;
        static bool valid_path = false;
        if (project_first_time) {
            project_first_time = false;
            std::string current_path = std::filesystem::current_path().string();
            int len = current_path.length() < PATH_LIMIT ? current_path.length() : PATH_LIMIT;
            strncpy(project_path, current_path.c_str(), len);
            project_path[len] = '\0';
            valid_path = std::filesystem::is_directory(project_path);
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputText("Path", project_path, PATH_LIMIT, ImGuiInputTextFlags_EnterReturnsTrue, [](ImGuiInputTextCallbackData* data) {
            return (valid_path = std::filesystem::is_directory(data->Buf)) ? 0 : 1;
        });
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            char *path = osdialog_file(OSDIALOG_OPEN_DIR, ".", NULL, NULL);
            if (path) {
                size_t len = strlen(path);
                len = len < PATH_LIMIT ? len : PATH_LIMIT;
                memcpy(project_path, path, len);
                project_path[len] = '\0';
                valid_path = true;
                free(path);
            }
        }
        if (!valid_path)
            ImGui::Text("Error: Invalid project path");
        ImGui::Separator();
        if (ImGui::Button("Create")) {
            if (valid_path) {
                state.project_path = project_path;
                state.new_project_dialog = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            sapp_quit();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Save Project", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        CENTRE_MODAL;
        ImGui::Text("Save Project");
        ImGui::Separator();
        ImGui::Text("The current project has unsaved changes.");
        ImGui::Text("Do you want to save before creating a new project?");
        ImGui::Separator();
        if (ImGui::Button("Save")) {
            // TODO: Actually save the project
            state.show_confirm_save_dialog = false;
            ImGui::CloseCurrentPopup();
            // After saving, open the new project dialog
            state.new_project_dialog = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Don't Save")) {
            state.show_confirm_save_dialog = false;
            ImGui::CloseCurrentPopup();
            // Open the new project dialog
            state.new_project_dialog = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            state.show_confirm_save_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Load Tileset", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        CENTRE_MODAL;
        ImGui::Text("Load Tileset");
        ImGui::Separator();
        static char tileset_path[PATH_LIMIT] = "";
        static bool valid_path = false;
        ImGui::InputText("Path", tileset_path, PATH_LIMIT, ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_EnterReturnsTrue, [](ImGuiInputTextCallbackData* data) {
            return (valid_path = std::filesystem::is_regular_file(data->Buf)) ? 0 : 1;
        });
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            osdialog_filters *filters = osdialog_filters_parse("Image:png,jpg,jpeg,bmp,qoi,tiff,tga");
            char *path = osdialog_file(OSDIALOG_OPEN, ".", NULL, filters);
            osdialog_filters_free(filters);
            if (path) {
                size_t len = strlen(path);
                len = len < PATH_LIMIT ? len : PATH_LIMIT;
                memcpy(tileset_path, path, len);
                tileset_path[len] = '\0';
                free(path);
                if (!(valid_path = std::filesystem::is_regular_file(tileset_path)))
                    tileset_path[0] = '\0';
            }
        }
        ImGui::Text("Tile Settings");
        ImGui::InputInt("Tile Width", &state.tileset.tile_width);
        state.tileset.tile_width = std::clamp(state.tileset.tile_width, 8, 256);
        ImGui::InputInt("Tile Height", &state.tileset.tile_height);
        state.tileset.tile_height = std::clamp(state.tileset.tile_height, 8, 256);
        if (!valid_path) {
            if (tileset_path[0] != '\0')
                ImGui::Text("Error! No file found at \"%s\"", tileset_path);
            else
                ImGui::Text("Error! No file selected");
        }
        if (ImGui::Button("Create")) {
            if (valid_path && tileset_path[0] != '\0') {
                // Load the texture
                std::ifstream file(tileset_path, std::ios::binary | std::ios::ate);
                std::vector<unsigned char> buffer;
                std::vector<unsigned char> qoi_buffer;
                std::streamsize size;
                bool loaded = false;
                int img_width = 0, img_height = 0;
                bool is_qoi = false;
                
                if (!file.is_open()) {
                    fprintf(stderr, "Failed to open tileset file: %s\n", tileset_path);
                    goto skip_map_creation;
                }
                
                size = file.tellg();
                file.seekg(0, std::ios::beg);
                
                buffer.resize(size);
                if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                    file.close();
                    fprintf(stderr, "Failed to read tileset file: %s\n", tileset_path);
                    goto skip_map_creation;
                }
                file.close();
                
                // Check if file is QOI (magic bytes "qoif")
                if ((is_qoi = memcmp(buffer.data(), "qoif", 4)) == 0) {
                    // Try to decode QOI to get dimensions
                    qoi_desc desc;
                    unsigned char *pixels = (unsigned char*)qoi_decode(buffer.data(), (int)size, &desc, 4);
                    if (!pixels) {
                        fprintf(stderr, "Failed to decode QOI file: %s\n", tileset_path);
                        goto skip_map_creation;
                    }
                    img_width = desc.width;
                    img_height = desc.height;
                    free(pixels);
                    
                    // Check dimensions before proceeding
                    if (img_width % state.tileset.tile_width != 0) {
                        fprintf(stderr, "Texture width must be divisible by tile width: %s\n", tileset_path);
                        goto skip_map_creation;
                    }
                    if (img_height % state.tileset.tile_height != 0) {
                        fprintf(stderr, "Texture height must be divisible by tile height: %s\n", tileset_path);
                        goto skip_map_creation;
                    }
                    
                    // Use original QOI buffer
                    qoi_buffer = buffer;
                } else {
                    // Load image using stb_image (force 4 channels RGBA)
                    int channels;
                    unsigned char *pixels = stbi_load(tileset_path, &img_width, &img_height, &channels, 4);
                    if (!pixels) {
                        fprintf(stderr, "Failed to load image with stb_image: %s - %s\n", tileset_path, stbi_failure_reason());
                        goto skip_map_creation;
                    }
                    
                    // Check dimensions before encoding
                    if (img_width % state.tileset.tile_width != 0) {
                        stbi_image_free(pixels);
                        fprintf(stderr, "Texture width must be divisible by tile width: %s\n", tileset_path);
                        goto skip_map_creation;
                    }
                    if (img_height % state.tileset.tile_height != 0) {
                        stbi_image_free(pixels);
                        fprintf(stderr, "Texture height must be divisible by tile height: %s\n", tileset_path);
                        goto skip_map_creation;
                    }
                    
                    // Encode to QOI
                    qoi_desc desc = {
                        static_cast<unsigned int>(img_width),
                        static_cast<unsigned int>(img_height),
                        static_cast<unsigned char>(4),
                        static_cast<unsigned char>(0) // 4 channels RGBA, sRGB colorspace
                    };
                    int qoi_size;
                    void *qoi_data = qoi_encode(pixels, &desc, &qoi_size);
                    stbi_image_free(pixels);
                    
                    if (!qoi_data) {
                        fprintf(stderr, "Failed to encode image to QOI: %s\n", tileset_path);
                        goto skip_map_creation;
                    }
                    
                    qoi_buffer.resize(qoi_size);
                    memcpy(qoi_buffer.data(), qoi_data, qoi_size);
                    free(qoi_data);
                }
                
                // Clean up existing texture if any
                if (state.tileset.texture != nullptr) {
                    state.tileset.texture->unload();
                    delete state.tileset.texture;
                    state.tileset.texture = nullptr;
                }
                
                // Create and load texture with QOI data
                if (!(state.tileset.texture = new Texture()))
                    goto skip_map_creation;
                if (!(loaded = state.tileset.texture->load(qoi_buffer.data(), qoi_buffer.size()))) {
                    delete state.tileset.texture;
                    fprintf(stderr, "Failed to load texture: %s\n", tileset_path);
                    goto skip_map_creation;
                }
                if (!state.tileset.texture->is_valid()) {
                    delete state.tileset.texture;
                    state.tileset.texture = nullptr;
                    fprintf(stderr, "Texture is not valid after loading: %s\n", tileset_path);
                    goto skip_map_creation;
                }

                // The tileset is all good! Finally!
                state.tileset.path = tileset_path;
                if (state.extra_files.size() > 0)
                    state.extra_files.erase(std::find(state.extra_files.begin(), state.extra_files.end(), state.tileset.path));
                state.tileset.tile_width = state.tileset.tile_width;
                state.tileset.tile_height = state.tileset.tile_height;
                state.is_tileset_loaded = true;
                state.show_tileset_dialog = false;
                state.tileset_masks.clear();
                memset(state.autotile_map, 0, sizeof(state.autotile_map));
                state.tile_cols = state.tileset.texture->width() / state.tileset.tile_width;
                state.tile_rows = state.tileset.texture->height() / state.tileset.tile_height;
                state.tileset_masks.resize(state.tile_cols * state.tile_rows);
                ImGui::CloseCurrentPopup();
                
                skip_map_creation:;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            state.show_tileset_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos((ImVec2){0,0}, ImGuiCond_Once, (ImVec2){0,0});
    ImGui::SetNextWindowSize((ImVec2){static_cast<float>(width), 10}, ImGuiCond_Once);
    ImGui::Begin("Main Menu", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File", true)) {
            if (ImGui::MenuItem("New", "Ctrl+N", false, true)) {
                state.show_confirm_save_dialog = true;
                ImGui::OpenPopup("Save Project");
            }
            if (ImGui::MenuItem("Open", "Ctrl+O", false, false)) {
                // TODO: Open project dialog
            }
            if (ImGui::MenuItem("Save", "Ctrl+S", false, true)) {
                state.show_export_dialog = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();

    if (ImGui::BeginPopupModal("Load Lua Script", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        CENTRE_MODAL;
        ImGui::Text("Load Lua Script");
        ImGui::Separator();
        static bool valid_path = true;
        static char lua_script_path[PATH_LIMIT] = "";
        ImGui::InputText("Path", lua_script_path, PATH_LIMIT, ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_EnterReturnsTrue, [](ImGuiInputTextCallbackData* data) {
            return (valid_path = std::filesystem::is_regular_file(data->Buf)) ? 0 : 1;
        });
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            osdialog_filters *filters = osdialog_filters_parse("Lua Script:lua");
            char *path = osdialog_file(OSDIALOG_OPEN, ".", NULL, filters);
            osdialog_filters_free(filters);
            if (path) {
                size_t len = strlen(path);
                len = len < PATH_LIMIT ? len : PATH_LIMIT;
                memcpy(lua_script_path, path, len);
                lua_script_path[len] = '\0';
                free(path);
                if (!(valid_path = std::filesystem::is_regular_file(lua_script_path)))
                    lua_script_path[0] = '\0';
            }
        }
        if (!valid_path && lua_script_path[0] != '\0')
            ImGui::Text("Error: Invalid path");
        if (ImGui::Button("Load")) {
            if (valid_path && lua_script_path[0] != '\0') {
                state.lua_script_path = lua_script_path;
                if (state.extra_files.size() > 0)
                    state.extra_files.erase(std::find(state.extra_files.begin(), state.extra_files.end(), state.lua_script_path));
                state.is_lua_script_loaded = true;
                state.show_lua_script_dialog = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            state.show_lua_script_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.show_autotile_dialog) {
        if (ImGui::Begin("Setup Autotile", &state.show_autotile_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Autotile");
            ImGui::Separator();
            
            ImGui::Text("Scale:");
            ImGui::SameLine();
            if (ImGui::Button("-")) {
                state.tileset_scale = std::max(1.0f, state.tileset_scale - 0.1f);
            }
            ImGui::SameLine();
            ImGui::Text("%.1fx", state.tileset_scale);
            ImGui::SameLine();
            if (ImGui::Button("+")) {
                state.tileset_scale = std::min(10.0f, state.tileset_scale + 0.1f);
            }
            ImGui::SliderFloat("Zoom", &state.tileset_scale, 1.f, 10.0f, "%.1fx");
            
            ImGui::Checkbox("Simplified (ignore corner neighbors)", &state.autotile_simplified);
            
            // Button to select default tile
            if (ImGui::Button(state.selecting_default_tile ? "Cancel Selection" : "Select default tile")) {
                state.selecting_default_tile = !state.selecting_default_tile;
            }
            if (state.default_tile_x >= 0 && state.default_tile_y >= 0) {
                ImGui::SameLine();
                ImGui::Text("Default: (%d, %d)", state.default_tile_x, state.default_tile_y);
            }
            
            ImGui::Separator();
            ImGui::Text("Mask View:");
            
            // Calculate scaled size for the child window
            ImVec2 scaled_size = (ImVec2) {
                (float)state.tileset.texture->width() * state.tileset_scale + 10,
                (float)state.tileset.texture->height() * state.tileset_scale + 10
            };
            if (ImGui::BeginChild("MaskEditorChild", scaled_size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                // Draw the tileset texture using draw_tileset
                if (state.tileset.texture != nullptr && state.tileset.texture->is_valid()) {
                    ImVec2 tex_size((float)state.tileset.texture->width() * state.tileset_scale, 
                                   (float)state.tileset.texture->height() * state.tileset_scale);
                    
                    // Use Dummy to reserve space and get position
                    ImGui::Dummy(tex_size);
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    
                    draw_tileset(ImGui::GetWindowDrawList(), min, max);
                }
                ImGui::EndChild();
            }

            bool masks_modified = false;
            if (state.selected_tile_x >= 0 && state.selected_tile_y >= 0) {
                ImGui::BeginChild("Button Grid", scaled_size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                ImGui::Text("Mask Editor");

                Neighbours *n = &state.tileset_masks[state.selected_tile_y * state.tile_cols + state.selected_tile_x];
                uint8_t bmask = _bitmask(n);
                char buf[9];
                for (int i = 0; i < 8; i++)
                    buf[i] = !!((bmask << i) & 0x80) ? 'F' : '0';
                buf[8] = '\0';
                ImGui::Text("tile: %d, %d - mask: %d,0x%x,0b%s", state.selected_tile_x, state.selected_tile_y, bmask, bmask, buf);
                
                float avail_height = ImGui::GetContentRegionAvail().y;
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 4.0f));
                if (ImGui::BeginTable("button_grid", 3, ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingStretchSame)) {
                    float row_height = avail_height / 3.0f;
                    for (int y = 0; y < 3; y++) {
                        ImGui::TableNextRow(ImGuiTableRowFlags_None, row_height);
                        for (int x = 0; x < 3; x++) {
                            ImGui::TableSetColumnIndex(x);
                            static const char *labels[9] = {
                                "TL", "T", "TR", "L", "X", "R", "BL", "B", "BR"
                            };
                            ImGui::PushStyleColor(ImGuiCol_Button, n->grid[y * 3 + x] ? (ImVec4){0.f, 1.f, 0.f, 1.f} : (ImVec4){1.f, 0.f, 0.f, 1.f});
                            if (ImGui::Button(labels[y * 3 + x], ImVec2(-FLT_MIN, row_height - 8.0f))) {
                                n->grid[y * 3 + x] = !n->grid[y * 3 + x];
                                if (!(x == 1 && y == 1)) {
                                    bool empty = true;
                                    for (int i = 0; i < 9; i++)
                                        if (i != 4 && n->grid[i]) {
                                            empty = false;
                                            break;
                                        }
                                    n->grid[4] = !empty;
                                } else
                                    if (!n->grid[4])
                                        for (int i = 0; i < 9; i++)
                                            n->grid[i] = false;
                                masks_modified = true;
                            }
                            ImGui::PopStyleColor(1);
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
                ImGui::EndChild();
            }

            std::vector<std::pair<int, int>> duplicates[256];
            bool has_duplicates = false;
            bool is_empty = true;
            for (int y = 0; y < state.tile_rows; y++)
                for (int x = 0; x < state.tile_cols; x++) {
                    Neighbours *n = &state.tileset_masks[y * state.tile_cols + x];
                    if (!n->grid[4])
                        continue;
                    duplicates[_bitmask(n)].push_back({x, y});
                    if (is_empty) {
                        for (int i = 0; i < 9; i++)
                            if (n->grid[i]) {
                                is_empty = false;
                                break;
                            }
                    }
                }
            state.autotile_not_empty = !is_empty;

            for (int i = 0; i < 256; i++) {
                if (duplicates[i].size() > 1) {
                    has_duplicates = true;
                    break;
                }
            }
            state.autotile_has_duplicates = has_duplicates;

            if (masks_modified && !state.autotile_has_duplicates)
                rebuild_autotile_map();

            if (has_duplicates) {
                ImGui::Separator();
                ImGui::Text("Duplicate Masks:");
                for (int i = 0; i < 256; i++)
                    if (duplicates[i].size() > 1) {
                        ImGui::Text("Mask %d (0x%x):", i, i);
                        for (auto &p : duplicates[i]) {
                            ImGui::SameLine();
                            ImGui::Text("(%d, %d)", p.first, p.second);
                        }
                    }
            }

            if (!state.autotile_not_empty) {
                ImGui::Separator();
                ImGui::Text("Autotile is empty!");
            }

            if (ImGui::Button("Close"))
                state.show_autotile_dialog = false;
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                state.tileset_masks.clear();
                state.tileset_masks.resize(state.tile_rows * state.tile_cols);
                state.selected_tile_x = -1;
                state.selected_tile_y = -1;
                state.autotile_has_duplicates = false;
                state.tileset_scale = 4.0f;
            }
        }
        ImGui::End();
    }

    if (state.show_export_dialog && !state.new_project_dialog && state.project_path != "") {
        int issue_count = 0;
        if (ImGui::Begin("Export", &state.show_export_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
            // Project generation dialog
            // Main dialog for project settings
            ImGui::Text("Project Path: %s/nicepkg.json %s", state.project_path.c_str(), state.has_been_saved ? "" : "(not yet saved)");
            if (!state.is_tileset_loaded) {
                ImGui::Text("Tileset: (not loaded)");
            } else {
                ImGui::Text("Tileset: OK (%s)", state.tileset.path.c_str());
            }
            ImGui::SameLine();
            SlimButton("Load Tileset", [=]() {
                state.show_tileset_dialog = true;
            });
            if (!state.is_tileset_loaded) {
                ImGui::Text("Autotile: (load tileset first)");
            } else {
                ImGui::Text("Autotile: %s", state.autotile_not_empty ? state.autotile_has_duplicates ? "(duplicates found)" : "OK" : "(empty)");
                ImGui::SameLine();
                SlimButton("Setup Autotile", [=]() {
                    state.show_autotile_dialog = true;
                });
            }
            if (!state.is_lua_script_loaded) {
                ImGui::Text("Lua Entry: (not loaded)");
            } else {
                ImGui::Text("Lua Entry: OK (%s)", state.lua_script_path.c_str());
            }
            ImGui::SameLine();
            SlimButton("Browse", [&]() {
                state.show_lua_script_dialog = true;
            });
            if (state.extra_files.empty()) {
                ImGui::Text("Extra Files: (none)");
            } else {
                ImGui::Text("Extra Files: %zu", state.extra_files.size());
            }
            ImGui::SameLine();
            SlimButton("Add", [&]() {
                int count = 0;
                char **files = osdialog_multifile(OSDIALOG_OPEN, "", NULL, NULL, &count);
                if (files && count > 0) {
                    for (int i = 0; i < count; i++) {
                        for (auto &f : state.extra_files)
                            if (f == files[i] || f == state.lua_script_path || f == state.tileset.path)
                                goto skip;
                        state.extra_files.push_back(files[i]);
                    skip:
                        free(files[i]);
                    }
                    free(files);
                }
            });
            if (!state.extra_files.empty()) {
                ImGui::Separator();
                ImGui::Text("Extra Files:");
                for (auto &path : state.extra_files) {
                    ImGui::Text("%s", path.c_str());
                    ImGui::SameLine();
                    SlimButton(fmt::format("Remove##{}", path).c_str(), [&]() {
                        state.extra_files.erase(std::remove(state.extra_files.begin(), state.extra_files.end(), path), state.extra_files.end());
                    });
                }
            }
            ImGui::Separator();
#define CHECK_ISSUE(CND, MSG) \
            if (!(CND)) { \
                ImGui::Text("Error: %s (%s)", MSG, #CND); \
                issue_count++; \
            }
            CHECK_ISSUE(state.is_tileset_loaded, "Tileset not set");
            CHECK_ISSUE(!state.autotile_has_duplicates, "Autotile has duplicates");
            CHECK_ISSUE(state.is_lua_script_loaded, "Lua entry script not set");
#undef CHECK_ISSUE
            if (issue_count > 0)
                ImGui::Text("Issues found: %d", issue_count);
            else
                ImGui::Text("No issues found!");
            if (!state.autotile_not_empty && (state.default_tile_x < 0 || state.default_tile_y < 0))
                ImGui::Text("Warning: Autotile is empty, autotiling will be disabled");
            if (state.extra_files.empty())
                ImGui::Text("Warning: No extra files");

            if (ImGui::Button("Save Project")) {
                if (issue_count == 0) {
                    save_project();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Build Project")) {
                if (issue_count == 0) {
                    // Open save file dialog
                    osdialog_filters* filters = osdialog_filters_parse("NICE Package:nice");
                    char* path = osdialog_file(OSDIALOG_SAVE, ".", "game.nice", filters);
                    osdialog_filters_free(filters);
                    
                    if (path) {
                        std::string output_path = path;
                        free(path);
                        
                        // Generate temporary autotile JSON
                        std::string autotile_json = save_autotile_json();
                        
                        // Export the package
                        bool success = export_package(
                            output_path,
                            state.tileset.path,
                            state.tileset.tile_width,
                            state.tileset.tile_height,
                            autotile_json,
                            state.lua_script_path,
                            state.extra_files
                        );
                        
                        // Clean up temporary autotile JSON
                        if (!autotile_json.empty()) {
                            std::remove(autotile_json.c_str());
                        }
                        
                        if (success) {
                            printf("Package exported successfully to: %s\n", output_path.c_str());
                        } else {
                            fprintf(stderr, "Failed to export package\n");
                        }
                    }
                }
            }
        }
        ImGui::End();
    }

    sdtx_canvas(width, height);
    sdtx_pos(0, 3);
    sdtx_printf("fps:    %.2f\n", 1.f / sapp_frame_duration());
    sdtx_printf("zoom:   %.2f\n", state.camera.zoom);
    sdtx_printf("pos:    (%.2f, %.2f)\n", state.camera.position.x, state.camera.position.y);
    sdtx_printf("target: (%.2f, %.2f)\n", state.camera.target_position.x, state.camera.target_position.y);

    float mx = $Input.mouse_position().x;
    float my = $Input.mouse_position().y;
    float wx = mx * (viewVolumeRight - viewVolumeLeft) / width + viewVolumeLeft;
    float wy = my * (viewVolumeTop - viewVolumeBottom) / height + viewVolumeBottom;

    if (state.is_tileset_loaded) {
        state.mouseX = (int)mx;
        state.mouseY = (int)my;
        state.worldX = (int)((wx + state.camera.position.x) / state.tileset.tile_width);
        state.worldY = (int)((wy + state.camera.position.y) / state.tileset.tile_height);
    }

    static bool was_mouse_down_last_frame = false;
    if ($Input.is_down(SAPP_MOUSEBUTTON_LEFT)) {
        if ($Input.is_shift_down()) {
            state.camera.target_position -= $Input.mouse_delta() * (1024.f - state.camera.zoom) / 1024.f;
        } else {
            if (!was_mouse_down_last_frame) {
                int gx = (int)((wx + state.camera.position.x) / state.tileset.tile_width);
                int gy = (int)((wy + state.camera.position.y) / state.tileset.tile_height);
                if (gx >= 0 && gx < state.grid_width && gy >= 0 && gy < state.grid_height)
                    state.grid[gx * state.grid_height + gy] = !state.grid[gx * state.grid_height + gy];
            }
        }
        was_mouse_down_last_frame = true;
    } else {
        was_mouse_down_last_frame = false;
    }

    // Zoom with scroll
    if ($Input.mouse_wheel().y != 0.0f) {
        state.camera.zoom -= $Input.mouse_wheel().y * .5;
        if (state.camera.zoom < 8.0f)
            state.camera.zoom = 8.0f;
        if (state.camera.zoom > 1024.0f)
            state.camera.zoom = 1024.0f;
    }

    float dt = (float)sapp_frame_duration();
    float speed = 10.0f;
    state.camera.position += (state.camera.target_position - state.camera.position) * (1.0f - exp(-speed * dt));

    sg_begin_pass(&state.pass);
    sgp_set_color(1.f, 1.f, 1.f, 1.f);

    float _tile_width = state.tileset.tile_width;
    float _tile_height = state.tileset.tile_height;
    int _grid_width = state.grid_width;
    int _grid_height = state.grid_height;
    if (state.is_tileset_loaded) {
        for (int x = 0; x < _grid_width; x++)
            for (int y = 0; y < _grid_height; y++)
                if (state.grid[x * _grid_height + y]) {
                    Neighbours n = {0};
                    for (int xx = x - 1; xx <= x + 1; xx++)
                        for (int yy = y - 1; yy <= y + 1; yy++) {
                            if (xx < 0 || xx >= _grid_width || yy < 0 || yy >= _grid_height)
                                continue;
                            if (state.grid[xx * _grid_height + yy])
                                n.grid[(xx - (x - 1)) + (yy - (y - 1)) * 3] = 1;
                        }
                    int bmask = _bitmask(&n);
                    Point p = state.autotile_map[bmask];
                    if (p.x != -1 && p.y != -1 && state.autotile_not_empty) {
                        sgp_set_image(0, *state.tileset.texture);
                        sgp_rect src = { (float)(p.x * _tile_width), (float)(p.y * _tile_height), _tile_width, _tile_height };
                        sgp_rect dst = { (float)(x * _tile_width), (float)(y * _tile_height), _tile_width, _tile_height };
                        sgp_draw_textured_rect(0, dst, src);
                        sgp_reset_image(0);
                    } else {
                        if (state.default_tile_x >= 0 && state.default_tile_y >= 0) {
                            if (state.default_tile_x >= state.tile_cols)
                                state.default_tile_x = state.tile_cols - 1;
                            if (state.default_tile_y >= state.tile_rows)
                                state.default_tile_y = state.tile_rows - 1;
                            sgp_set_image(0, *state.tileset.texture);
                            sgp_rect src = { (float)(state.default_tile_x * _tile_width), (float)(state.default_tile_y * _tile_height), _tile_width, _tile_height };
                            sgp_rect dst = { (float)(x * _tile_width), (float)(y * _tile_height), _tile_width, _tile_height };
                            sgp_draw_textured_rect(0, dst, src);
                            sgp_reset_image(0);
                        } else {
                            sgp_draw_filled_rect(x * _tile_width, y * _tile_height, _tile_width, _tile_height);
                        }
                    }
                }
        
        // Draw grid lines
        for (int x = 0; x < _grid_width+1; x++) {
            float xx = x * _tile_width;
            sgp_draw_line(xx, 0, xx, (_tile_height*_grid_height));
        }
        for (int y = 0; y < _grid_height+1; y++) {
            float yy = y * _tile_height;
            sgp_draw_line(0, yy, (_tile_width*_grid_width), yy);
        }
        sgp_reset_color();
    }
    
    sgp_flush();
    sgp_end();
    sg_end_pass();

    sg_pass pass_desc = {
        .action = state.pass_action,
        .swapchain = sglue_swapchain()
    };
    sg_begin_pass(&pass_desc);
    sg_apply_pipeline(state.pipeline);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 1);
    sdtx_draw();
    simgui_render();
    sg_end_pass();
    sg_commit();
    $Input.update();
}

static void event(const sapp_event *event) {
    if (!simgui_handle_event(event))
        $Input.handle(event);
}

static void cleanup(void) {
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    argparse::ArgumentParser program("nicepkg");
    
    program.add_argument("-p", "--project")
        .help("The path to the project file to load")
        .default_value("");
    
    program.add_argument("-t", "--tileset")
        .help("Path to the tileset image")
        .default_value("");
    
    program.add_argument("-a", "--autotile")
        .help("Path to the autotile JSON configuration")
        .default_value("");
    
    program.add_argument("-l", "--lua")
        .help("Path to the Lua entry script")
        .default_value("");
    
    program.add_argument("-e", "--extra")
        .help("Extra files to include in the package (can be specified multiple times)")
        .append()
        .default_value(std::vector<std::string>{});
    
    program.add_argument("-o", "--output")
        .help("Output path for the .nice package file (headless mode)")
        .default_value("");
    
    program.add_argument("--tile-width")
        .help("Tile width for the tileset (default: 8)")
        .default_value(8)
        .scan<'i', int>();
    
    program.add_argument("--tile-height")
        .help("Tile height for the tileset (default: 8)")
        .default_value(8)
        .scan<'i', int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << program;
        exit(1);
    }

    // Check if we're in headless mode (output path is specified)
    std::string output_path = program.get<std::string>("--output");
    if (!output_path.empty()) {
        // Headless mode - create package and exit
        std::string project_path = program.get<std::string>("--project");
        
        // Load project if specified
        if (!project_path.empty()) {
            if (!load_project(project_path, true)) {
                fprintf(stderr, "Error: Failed to load project file: %s\n", project_path.c_str());
                exit(1);
            }
        }
        
        std::string tileset_path = program.get<std::string>("--tileset");
        std::string autotile_path = program.get<std::string>("--autotile");
        std::string lua_path = program.get<std::string>("--lua");
        auto extra_files = program.get<std::vector<std::string>>("--extra");
        int tile_width = program.get<int>("--tile-width");
        int tile_height = program.get<int>("--tile-height");
        
        // Use project settings if CLI args are not provided
        if (tileset_path.empty() && state.is_tileset_loaded) {
            tileset_path = state.tileset.path;
            // Only use project tile size if CLI args are default (8)
            if (tile_width == 8) tile_width = state.tileset.tile_width;
            if (tile_height == 8) tile_height = state.tileset.tile_height;
        }
        
        if (lua_path.empty() && state.is_lua_script_loaded) {
            lua_path = state.lua_script_path;
        }
        
        if (autotile_path.empty()) {
            // Generate temporary autotile JSON from project state
            autotile_path = save_autotile_json();
        }
        
        // Merge extra files from project
        for (const auto& file : state.extra_files) {
            bool found = false;
            for (const auto& cli_file : extra_files) {
                if (std::filesystem::equivalent(file, cli_file)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                extra_files.push_back(file);
            }
        }
        
        // Check required arguments
        if (tileset_path.empty()) {
            fprintf(stderr, "Error: Tileset path is required (-t/--tileset) or in project file\n");
            exit(1);
        }
        if (lua_path.empty()) {
            fprintf(stderr, "Error: Lua script path is required (-l/--lua) or in project file\n");
            exit(1);
        }
        
        // Convert relative paths to absolute
        if (!std::filesystem::path(tileset_path).is_absolute()) {
            tileset_path = std::filesystem::absolute(tileset_path).string();
        }
        if (!autotile_path.empty() && !std::filesystem::path(autotile_path).is_absolute()) {
            autotile_path = std::filesystem::absolute(autotile_path).string();
        }
        if (!std::filesystem::path(lua_path).is_absolute()) {
            lua_path = std::filesystem::absolute(lua_path).string();
        }
        if (!std::filesystem::path(output_path).is_absolute()) {
            output_path = std::filesystem::absolute(output_path).string();
        }
        
        // Convert extra files to absolute paths
        std::vector<std::string> absolute_extra_files;
        for (const auto& file : extra_files) {
            if (std::filesystem::path(file).is_absolute()) {
                absolute_extra_files.push_back(file);
            } else {
                absolute_extra_files.push_back(std::filesystem::absolute(file).string());
            }
        }
        
        // Create the package
        printf("Creating package in headless mode...\n");
        bool success = export_package(
            output_path,
            tileset_path,
            tile_width,
            tile_height,
            autotile_path,
            lua_path,
            absolute_extra_files
        );
        
        if (success) {
            printf("Package created successfully: %s\n", output_path.c_str());
            exit(0);
        } else {
            fprintf(stderr, "Failed to create package\n");
            exit(1);
        }
    }

    // GUI mode
    std::string project_path = program.get<std::string>("--project");
    if (!project_path.empty()) {
        // Convert to absolute path if needed
        std::filesystem::path proj_path(project_path);
        if (!proj_path.is_absolute()) {
            proj_path = std::filesystem::absolute(proj_path);
        }
        
        // If the path is a directory, append "/nicepkg.json"
        if (std::filesystem::is_directory(proj_path)) {
            proj_path = proj_path / "nicepkg.json";
        }
        
        // Try to load the project
        if (load_project(proj_path.string())) {
            // Successfully loaded, skip the New Project dialog
            state.new_project_dialog = false;
        } else {
            // Failed to load, show error and still show New Project dialog
            fprintf(stderr, "Failed to load project from: %s\n", proj_path.string().c_str());
            fprintf(stderr, "Starting with new project dialog instead.\n");
        }
    }

    return (sapp_desc) {
        .width = DEFAULT_WINDOW_WIDTH,
        .height = DEFAULT_WINDOW_HEIGHT,
        .window_title = fmt::format("nicepkg").c_str(),
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup
    };
}
