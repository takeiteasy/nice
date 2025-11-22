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
#include "sokol_gp.h"
#include "dummy_map.hpp"
#include "texture.hpp"
#include <cstddef>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <fstream>
#include <vector>
#include <filesystem>
#include "stb_image.h"
#include "stb_image.h"
#include "qoi.h"
#include "./dat.h"

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
    int selected_tile_x = -1;
    int selected_tile_y = -1;
    int hovered_tile_x = -1;
    int hovered_tile_y = -1;

    bool project_loaded = false;
    bool new_project_dialog = true;
    std::string project_path = "";
    bool has_been_saved = false;

    bool is_tileset_loaded = false;
    bool show_tileset_dialog = false;
    Tileset tileset;

    bool is_autotile_loaded = false;
    bool show_autotile_dialog = false;
    DummyMap dummy_map;
    float tileset_scale = 4.0f;

    bool show_export_dialog = true;
    bool show_save_project_dialog = false;
} state;

int framebuffer_width(void) {
    return state.framebuffer_width;
}

int framebuffer_height(void) {
    return state.framebuffer_height;
}

void framebuffer_resize(int width, int height) {
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

    state.cross = new Texture();
    assert(state.cross->load(x_png, x_png_size));

    if (!state.white_tex) {
        state.white_tex = new Texture();
        assert(state.white_tex->load(white_png, white_png_size));
    }
    if (!state.red_tex) {
        state.red_tex = new Texture();
        assert(state.red_tex->load(red_png, red_png_size));
    }
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
    state.dummy_map.reset();
    // Center camera on the grid (grid is 64x64 tiles of 8x8 pixels = 512x512, center is 256,256)
    state.camera.position = glm::vec2(0.f, 0.f);
    state.camera.target_position = glm::vec2(0.f, 0.f);
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

static void draw_tileset(ImDrawList* dl, ImVec2 min, ImVec2 max) {
    if (state.tileset.texture == nullptr || !state.tileset.texture->is_valid())
        return;
    if (state.tileset.tile_width <= 0 || state.tileset.tile_height <= 0)
        return;

    sg_image tileset_img = *state.tileset.texture;
    if (tileset_img.id == SG_INVALID_ID || sg_query_image_state(tileset_img) != SG_RESOURCESTATE_VALID)
        return;
    
    ImTextureID tileset_tex = simgui_imtextureid(tileset_img);
    dl->AddImage(tileset_tex, min, max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE);

    int tex_width = state.tileset.texture->width();
    int tex_height = state.tileset.texture->height();
    int tile_w = state.tileset.tile_width;
    int tile_h = state.tileset.tile_height;
    int cols = tex_width / tile_w;
    int rows = tex_height / tile_h;

    float scale_x = (max.x - min.x) / tex_width;
    float scale_y = (max.y - min.y) / tex_height;
    ImVec2 mouse_pos = ImGui::GetMousePos();
    if (ImGui::IsWindowHovered() && mouse_pos.x >= min.x && mouse_pos.x < max.x && mouse_pos.y >= min.y && mouse_pos.y < max.y) {
        int tx = (int)((mouse_pos.x - min.x) / (tile_w * scale_x));
        int ty = (int)((mouse_pos.y - min.y) / (tile_h * scale_y));

        if (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
            state.hovered_tile_x = tx;
            state.hovered_tile_y = ty;

            if (state.white_tex && state.white_tex->is_valid()) {
                ImVec2 t_min = ImVec2(min.x + tx * tile_w * scale_x, min.y + ty * tile_h * scale_y);
                ImVec2 t_max = ImVec2(t_min.x + tile_w * scale_x, t_min.y + tile_h * scale_y);
                sg_image white_img = *state.white_tex;
                dl->AddImage(simgui_imtextureid(white_img), t_min, t_max);
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                state.selected_tile_x = tx;
                state.selected_tile_y = ty;
            }
        }
    } else {
        state.hovered_tile_x = -1;
        state.hovered_tile_y = -1;
    }

    if (state.selected_tile_x >= 0 && state.selected_tile_y >= 0) {
        int tx = state.selected_tile_x;
        int ty = state.selected_tile_y;
        if (tx < cols && ty < rows) {
            ImVec2 t_min = ImVec2(min.x + tx * tile_w * scale_x, min.y + ty * tile_h * scale_y);
            ImVec2 t_max = ImVec2(t_min.x + tile_w * scale_x, t_min.y + tile_h * scale_y);

            if (state.red_tex && state.red_tex->is_valid()) {
                sg_image red_img = *state.red_tex;
                dl->AddImage(simgui_imtextureid(red_img), t_min, t_max);
            }
        }
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

    if (state.show_save_project_dialog)
        ImGui::OpenPopup("Save Project");

    if (state.show_tileset_dialog)
        ImGui::OpenPopup("Load Tileset");

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
            state.show_save_project_dialog = false;
            ImGui::CloseCurrentPopup();
            // After saving, open the new project dialog
            state.new_project_dialog = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Don't Save")) {
            state.show_save_project_dialog = false;
            ImGui::CloseCurrentPopup();
            // Open the new project dialog
            state.new_project_dialog = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            state.show_save_project_dialog = false;
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
                state.tileset.tile_width = state.tileset.tile_width;
                state.tileset.tile_height = state.tileset.tile_height;
                state.is_tileset_loaded = true;
                state.show_tileset_dialog = false;
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
                state.show_save_project_dialog = true;
                ImGui::OpenPopup("Save Project");
            }
            if (ImGui::MenuItem("Open", "Ctrl+O", false, false)) { /* TODO! */ }
            if (ImGui::MenuItem("Save", "Ctrl+S", false, true)) {
                state.show_export_dialog = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();

    if (state.show_autotile_dialog) {
        if (ImGui::Begin("Setup Autotile", &state.show_autotile_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Autotile");
            ImGui::Separator();
            
            // Scale controls
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
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                state.tileset_scale = 4.0f;
            }
            ImGui::SliderFloat("Zoom", &state.tileset_scale, 1.f, 10.0f, "%.1fx");
            
            ImGui::Separator();
            ImGui::Text("Mask View:");
            
            // Calculate scaled size for the child window
            ImVec2 scaled_size = (ImVec2) {
                (float)state.tileset.texture->width() * state.tileset_scale + 10,
                (float)state.tileset.texture->height() * state.tileset_scale + 10
            };
            if (ImGui::BeginChild("Mask Editor", scaled_size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
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

            ImGui::Text("Mask Editor");
            ImGui::BeginChild("Button Grid", scaled_size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
                        static bool b = false;
                        ImGui::PushStyleColor(ImGuiCol_Button, b ? (ImVec4){0.f, 1.f, 0.f, 1.f} : (ImVec4){1.f, 0.f, 0.f, 1.f});
                        if (ImGui::Button(labels[y * 3 + x], ImVec2(-FLT_MIN, row_height - 8.0f))) {
                            b = !b;
                        }
                        ImGui::PopStyleColor(1);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
            ImGui::EndChild();

            ImGui::Separator();
            if (ImGui::Button("Cancel"))
                state.show_autotile_dialog = false;
            ImGui::End();
        }
    }

    if (state.show_export_dialog && !state.new_project_dialog && state.project_path != "") {
        int issue_count = 0;
        if (ImGui::Begin("Export", &state.show_export_dialog, ImGuiWindowFlags_None)) {
            // Project generation dialog
            // Main dialog for project settings
            ImGui::Text("Project Status");
            ImGui::Text("Project Path: %s/nicepkg.json %s", state.project_path.c_str(), state.has_been_saved ? "" : "(not yet saved)");
            if (!state.is_tileset_loaded) {
                ImGui::Text("Tileset: (not loaded)");
            } else {
                ImGui::Text("Tileset: %s", state.tileset.path.c_str());
            }
            ImGui::SameLine();
            SlimButton("Load Tileset", [=]() {
                state.show_tileset_dialog = true;
            });
            if (!state.is_tileset_loaded) {
                ImGui::Text("Autotile: (load tileset first)");
            } else {
                if (!state.is_autotile_loaded) {
                    ImGui::Text("Autotile: (not setup)");
                } else {
                    ImGui::Text("Autotile: Done");
                }
                ImGui::SameLine();
                SlimButton("Setup Autotile", [=]() {
                    state.show_autotile_dialog = true;
                });
            }
            ImGui::Separator();
#define CHECK_ISSUE(CND, MSG) \
            if (!(CND)) { \
                ImGui::Text("Error: %s (%s)", MSG, #CND); \
                issue_count++; \
            }
            CHECK_ISSUE(state.is_tileset_loaded, "Tileset not loaded");
            CHECK_ISSUE(state.is_autotile_loaded, "Autotile not loaded");
#undef CHECK_ISSUE
            if (issue_count > 0)
                ImGui::Text("Issues found: %d", issue_count);
            else
                ImGui::Text("No issues found!");
        }
        ImGui::Separator();
        if (ImGui::Button("Export Project")) {
            if (issue_count == 0) {
                // TODO: Export the project
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

    if (state.is_tileset_loaded) {
        state.mouseX = $Input.mouse_position().x;
        state.mouseY = $Input.mouse_position().y;
        state.worldX = state.mouseX * (viewVolumeRight - viewVolumeLeft) / width + viewVolumeLeft;
        state.worldY = state.mouseY * (viewVolumeTop - viewVolumeBottom) / height + viewVolumeBottom;
        state.worldX = (int)((state.worldX + state.camera.position.x) / state.tileset.tile_width);
        state.worldY = (int)((state.worldY + state.camera.position.y) / state.tileset.tile_height);
    }

    if ($Input.is_down(SAPP_MOUSEBUTTON_LEFT) && $Input.is_shift_down()) {
        state.camera.target_position -= $Input.mouse_delta() * (1024.f - state.camera.zoom) / 1024.f;
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
    state.dummy_map.draw();
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
        .default_value("")
        .action([](const std::string& value) {
            state.project_path = value;
        });

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << program;
        exit(1);
    }

    if (state.project_path != "") {
        // Actually load the project...
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
