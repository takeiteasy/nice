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
#define QOI_IMPLEMENTATION
#include "qoi.h"

void die(const char* msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

void print_usage(const char* prog_name) {
    printf("Usage: %s [OPTIONS] IMAGE...\n", prog_name);
    printf("Convert images to QOI format\n\n");
    printf("OPTIONS:\n");
    printf("  --channels CHANNELS  Force number of channels (3=RGB, 4=RGBA, 0=auto) (default: 0)\n");
    printf("  --colorspace SPACE   Colorspace (0=sRGB with linear alpha, 1=all linear) (default: 0)\n");
    printf("  -h, --help           Show this help message\n\n");
    printf("ARGUMENTS:\n");
    printf("  IMAGE...             Path(s) to the input image file(s)\n");
}

char* generate_output_path(const char* input_path) {
    const char* last_dot = strrchr(input_path, '.');
    const char* last_slash = strrchr(input_path, '/');

    // Find the base name without path and extension
    const char* base_start = last_slash ? last_slash + 1 : input_path;
    size_t base_len;
    if (last_dot && last_dot > base_start)
        base_len = last_dot - base_start;
    else
        base_len = strlen(base_start);

    // Calculate total length needed
    size_t path_len = base_start - input_path; // Path prefix length
    size_t total_len = path_len + base_len + strlen(".qoi") + 1;

    char* output_path = malloc(total_len);
    if (!output_path)
        return NULL;

    // Build the output path: path + base + ".qoi"
    snprintf(output_path, total_len, "%.*s%.*s.qoi",
             (int)path_len, input_path,
             (int)base_len, base_start);

    return output_path;
}

int main(int argc, char* argv[]) {
    int force_channels = 0;  // 0 = auto, 3 = RGB, 4 = RGBA
    int colorspace = 0;      // 0 = sRGB with linear alpha, 1 = all linear

    static struct option long_options[] = {
        {"channels",   required_argument, 0, 'c'},
        {"colorspace", required_argument, 0, 's'},
        {"help",       no_argument,       0, '?'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "c:s:?", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                force_channels = atoi(optarg);
                if (force_channels != 0 && force_channels != 3 && force_channels != 4)
                    die("Channels must be 0 (auto), 3 (RGB), or 4 (RGBA)");
                break;
            case 's':
                colorspace = atoi(optarg);
                if (colorspace != 0 && colorspace != 1)
                    die("Colorspace must be 0 (sRGB with linear alpha) or 1 (all linear)");
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

        printf("Converting '%s' to '%s'\n", input_path, output_path);
        
        // Use a wrapper to catch errors and continue with next file
        int width, height, channels;
        unsigned char* img_data = stbi_load(input_path, &width, &height, &channels, force_channels);
        if (!img_data) {
            fprintf(stderr, "WARNING: Failed to load image '%s': %s, skipping\n", 
                   input_path, stbi_failure_reason());
            free(output_path);
            continue;
        }

        // Use forced channels if specified, otherwise use original
        int output_channels = force_channels > 0 ? force_channels : channels;

        printf("Loaded image: %dx%d, %d channels -> %d channels\n",
               width, height, channels, output_channels);

        // Prepare QOI descriptor
        qoi_desc desc = {
            .width = width,
            .height = height,
            .channels = output_channels,
            .colorspace = colorspace
        };

        // Encode to QOI format
        int qoi_len;
        void* qoi_data = qoi_encode(img_data, &desc, &qoi_len);

        if (!qoi_data) {
            fprintf(stderr, "WARNING: Failed to encode image '%s' to QOI format, skipping\n", input_path);
            stbi_image_free(img_data);
            free(output_path);
            continue;
        }

        printf("Encoded to QOI: %d bytes\n", qoi_len);

        // Write QOI data to file
        FILE* output_file = fopen(output_path, "wb");
        if (!output_file) {
            fprintf(stderr, "WARNING: Failed to open output file '%s', skipping\n", output_path);
            stbi_image_free(img_data);
            free(qoi_data);
            free(output_path);
            continue;
        }

        size_t written = fwrite(qoi_data, 1, qoi_len, output_file);
        fclose(output_file);

        if (written != (size_t)qoi_len) {
            fprintf(stderr, "WARNING: Failed to write complete QOI data to '%s', skipping\n", output_path);
            stbi_image_free(img_data);
            free(qoi_data);
            free(output_path);
            continue;
        }

        printf("Successfully converted '%s' to QOI format!\n", input_path);

        // Clean up
        stbi_image_free(img_data);
        free(qoi_data);
        free(output_path);
    }

    return 0;
}
