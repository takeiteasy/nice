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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void die(const char* msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

void print_usage(const char* prog_name) {
    printf("Usage: %s [OPTIONS] IMAGE...\n", prog_name);
    printf("Split images into smaller tiles\n\n");
    printf("OPTIONS:\n");
    printf("  --width WIDTH    Width of each tile (default: 8)\n");
    printf("  --height HEIGHT  Height of each tile (default: 8)\n");
    printf("  --padding PADDING Padding to add around tiles (default: 4)\n");
    printf("  -h, --help       Show this help message\n\n");
    printf("ARGUMENTS:\n");
    printf("  IMAGE...         Path(s) to the image file(s)\n");
}

char* generate_output_path(const char* input_path) {
    const char* last_dot = strrchr(input_path, '.');
    if (!last_dot) {
        // No extension found, append .exploded
        size_t len = strlen(input_path) + strlen(".exploded") + 1;
        char* output_path = malloc(len);
        if (!output_path)
            return NULL;
        snprintf(output_path, len, "%s.exploded", input_path);
        return output_path;
    }

    // Calculate lengths
    size_t base_len = last_dot - input_path;
    const char* extension = last_dot;
    size_t total_len = base_len + strlen(".exploded") + strlen(extension) + 1;

    char* output_path = malloc(total_len);
    if (!output_path)
        return NULL;

    // Build the output path: base + ".exploded" + extension
    snprintf(output_path, total_len, "%.*s.exploded%s", (int)base_len, input_path, extension);
    return output_path;
}

int main(int argc, char* argv[]) {
    int tile_width = 8;
    int tile_height = 8;
    int padding = 4;

    static struct option long_options[] = {
        {"width",   required_argument, 0, 'w'},
        {"height",  required_argument, 0, 'h'},
        {"padding", required_argument, 0, 'p'},
        {"help",    no_argument,       0, '?'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "w:h:p:?", long_options, &option_index)) != -1) {
        switch (c) {
            case 'w':
                tile_width = atoi(optarg);
                if (tile_width <= 0) {
                    die("Width must be positive");
                }
                break;
            case 'h':
                tile_height = atoi(optarg);
                if (tile_height <= 0)
                    die("Height must be positive");
                break;
            case 'p':
                padding = atoi(optarg);
                if (padding < 0)
                    die("Padding must be non-negative");
                break;
            case '?':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Check if we have at least one image file
    if (optind >= argc) {
        fprintf(stderr, "ERROR: Missing required argument: IMAGE\n\n");
        print_usage(argv[0]);
        return 1;
    }

    // Process each input file
    for (int i = optind; i < argc; i++) {
        char* input_path = argv[i];
        char* output_path = generate_output_path(input_path);
        
        if (!output_path) {
            fprintf(stderr, "WARNING: Failed to generate output path for '%s', skipping\n", input_path);
            continue;
        }

        printf("Processing '%s' -> '%s'\n", input_path, output_path);
        
        // Load and validate the image
        int width, height, channels;
        unsigned char* img = stbi_load(input_path, &width, &height, &channels, 4); // Force 4 channels (RGBA)

        if (!img) {
            fprintf(stderr, "WARNING: Failed to load image '%s': %s, skipping\n", 
                   input_path, stbi_failure_reason());
            free(output_path);
            continue;
        }

        // Validate tile size
        if (tile_width > width || tile_height > height) {
            fprintf(stderr, "WARNING: Tile size larger than image '%s': tile size: %d,%d, image size: %d,%d, skipping\n",
                    input_path, tile_width, tile_height, width, height);
            stbi_image_free(img);
            free(output_path);
            continue;
        }

        if (width % tile_width != 0 || height % tile_height != 0) {
            fprintf(stderr, "WARNING: Tile size is not multiple of image size for '%s': tile size: %d,%d, image size: %d,%d, skipping\n",
                    input_path, tile_width, tile_height, width, height);
            stbi_image_free(img);
            free(output_path);
            continue;
        }

        int rows = height / tile_height;
        int cols = width / tile_width;
        int new_width = (cols * tile_width) + ((cols + 1) * padding);
        int new_height = (rows * tile_height) + ((rows + 1) * padding);

        // Allocate output image buffer (4 channels for RGBA)
        unsigned char* outimg = (unsigned char*)calloc(new_width * new_height * 4, sizeof(unsigned char));
        if (!outimg) {
            fprintf(stderr, "WARNING: Failed to allocate memory for output image '%s', skipping\n", input_path);
            stbi_image_free(img);
            free(output_path);
            continue;
        }

        // Process each tile
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < cols; x++) {
                // Source rectangle coordinates
                int src_x = x * tile_width;
                int src_y = y * tile_height;

                // Destination coordinates
                int dx = x * tile_width + ((x + 1) * padding);
                int dy = y * tile_height + ((y + 1) * padding);

                // Copy tile pixel by pixel
                for (int tile_y = 0; tile_y < tile_height; tile_y++)
                    for (int tile_x = 0; tile_x < tile_width; tile_x++) {
                        // Source pixel position
                        int src_pixel_x = src_x + tile_x;
                        int src_pixel_y = src_y + tile_y;
                        int src_index = (src_pixel_y * width + src_pixel_x) * 4;

                        // Destination pixel position
                        int dst_pixel_x = dx + tile_x;
                        int dst_pixel_y = dy + tile_y;
                        int dst_index = (dst_pixel_y * new_width + dst_pixel_x) * 4;

                        // Copy RGBA channels
                        outimg[dst_index + 0] = img[src_index + 0]; // R
                        outimg[dst_index + 1] = img[src_index + 1]; // G
                        outimg[dst_index + 2] = img[src_index + 2]; // B
                        outimg[dst_index + 3] = img[src_index + 3]; // A
                    }
            }

        // Save the output image
        if (!stbi_write_png(output_path, new_width, new_height, 4, outimg, new_width * 4)) {
            fprintf(stderr, "WARNING: Failed to save output image '%s', skipping\n", output_path);
            stbi_image_free(img);
            free(outimg);
            free(output_path);
            continue;
        }

        printf("Successfully exploded image: %s -> %s\n", input_path, output_path);

        // Clean up
        stbi_image_free(img);
        free(outimg);
        free(output_path);
    }

    return 0;
}
