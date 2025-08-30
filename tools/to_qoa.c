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

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"     // WAV loading functions
#include "stb_vorbis.c" // OGG loading functions
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h" // MP3 loading functions
#define QOA_IMPLEMENTATION
#include "qoa.h" // QOA loading and saving functions
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_WIN32_IO
#include "dr_flac.h" // FLAC loading functions

void die(const char* msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

void print_usage(const char* prog_name) {
    printf("Usage: %s [OPTIONS] AUDIO...\n", prog_name);
    printf("Convert audio files to QOA format\n\n");
    printf("OPTIONS:\n");
    printf("  -h, --help           Show this help message\n\n");
    printf("ARGUMENTS:\n");
    printf("  AUDIO...             Path(s) to the input audio file(s)\n");
    printf("                       Supported formats: WAV, OGG, MP3, FLAC\n");
}

char* generate_output_path(const char* input_path) {
    const char* last_dot = strrchr(input_path, '.');
    
    size_t base_len;
    if (last_dot) {
        // Use everything up to the last dot
        base_len = last_dot - input_path;
    } else {
        // No extension, use the whole path
        base_len = strlen(input_path);
    }

    // Calculate total length needed: base + ".qoa" + null terminator
    size_t total_len = base_len + 5; // ".qoa" + null terminator

    char* output_path = malloc(total_len);
    if (!output_path)
        return NULL;

    // Build the output path: base + ".qoa"
    snprintf(output_path, total_len, "%.*s.qoa", (int)base_len, input_path);

    return output_path;
}

// Load different audio formats and convert to 16-bit PCM
short* load_audio_file(const char* input_path, unsigned int* channels, unsigned int* samplerate, unsigned int* total_samples) {
    const char* ext = strrchr(input_path, '.');
    if (!ext) {
        fprintf(stderr, "WARNING: No file extension found for '%s'\n", input_path);
        return NULL;
    }

    short* sample_data = NULL;
    
    if (strcasecmp(ext, ".wav") == 0) {
        // Load WAV file
        drwav wav;
        if (!drwav_init_file(&wav, input_path, NULL)) {
            fprintf(stderr, "WARNING: Failed to load WAV file '%s'\n", input_path);
            return NULL;
        }
        
        *channels = wav.channels;
        *samplerate = wav.sampleRate;
        *total_samples = (unsigned int)wav.totalPCMFrameCount;
        
        sample_data = malloc(*total_samples * *channels * sizeof(short));
        if (sample_data) {
            drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, sample_data);
        }
        drwav_uninit(&wav);
        
    } else if (strcasecmp(ext, ".ogg") == 0) {
        // Load OGG file
        int ch, sr;
        short* decoded_data;
        int decoded_samples = stb_vorbis_decode_filename(input_path, &ch, &sr, &decoded_data);
        if (decoded_samples > 0) {
            *channels = ch;
            *samplerate = sr;
            *total_samples = decoded_samples;
            sample_data = decoded_data;
        } else {
            fprintf(stderr, "WARNING: Failed to load OGG file '%s'\n", input_path);
            return NULL;
        }
        
    } else if (strcasecmp(ext, ".mp3") == 0) {
        // Load MP3 file
        drmp3 mp3;
        if (!drmp3_init_file(&mp3, input_path, NULL)) {
            fprintf(stderr, "WARNING: Failed to load MP3 file '%s'\n", input_path);
            return NULL;
        }
        
        *channels = mp3.channels;
        *samplerate = mp3.sampleRate;
        
        // Get total frame count
        drmp3_uint64 total_frames = drmp3_get_pcm_frame_count(&mp3);
        *total_samples = (unsigned int)total_frames;
        
        sample_data = malloc(*total_samples * *channels * sizeof(short));
        if (sample_data) {
            float* float_data = malloc(*total_samples * *channels * sizeof(float));
            if (float_data) {
                drmp3_read_pcm_frames_f32(&mp3, total_frames, float_data);
                // Convert float to short
                for (unsigned int i = 0; i < *total_samples * *channels; i++) {
                    float sample = float_data[i];
                    if (sample >= 1.0f) sample_data[i] = 32767;
                    else if (sample <= -1.0f) sample_data[i] = -32768;
                    else sample_data[i] = (short)(sample * 32767.0f);
                }
                free(float_data);
            }
        }
        drmp3_uninit(&mp3);
        
    } else if (strcasecmp(ext, ".flac") == 0) {
        // Load FLAC file
        unsigned int ch, sr;
        drflac_uint64 total_frames;
        short* decoded = drflac_open_file_and_read_pcm_frames_s16(input_path, &ch, &sr, &total_frames, NULL);
        if (decoded) {
            *channels = ch;
            *samplerate = sr;
            *total_samples = (unsigned int)total_frames;
            sample_data = decoded;
        } else {
            fprintf(stderr, "WARNING: Failed to load FLAC file '%s'\n", input_path);
            return NULL;
        }
        
    } else {
        fprintf(stderr, "WARNING: Unsupported file format '%s' for file '%s'\n", ext, input_path);
        return NULL;
    }
    
    return sample_data;
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Check if we have at least one audio file
    if (optind >= argc) {
        fprintf(stderr, "ERROR: Missing required argument: AUDIO\n\n");
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
        
        // Load audio file
        unsigned int channels, samplerate, total_samples;
        short* sample_data = load_audio_file(input_path, &channels, &samplerate, &total_samples);
        
        if (!sample_data) {
            free(output_path);
            continue;
        }

        printf("Loaded audio: %u samples, %u channels, %u Hz\n", total_samples, channels, samplerate);

        // Prepare QOA descriptor
        qoa_desc qoa_info = {0};
        qoa_info.channels = channels;
        qoa_info.samplerate = samplerate;
        qoa_info.samples = total_samples;

        // Encode to QOA format
        unsigned int qoa_len;
        void* qoa_data = qoa_encode(sample_data, &qoa_info, &qoa_len);

        if (!qoa_data) {
            fprintf(stderr, "WARNING: Failed to encode audio '%s' to QOA format, skipping\n", input_path);
            free(sample_data);
            free(output_path);
            continue;
        }

        printf("Encoded to QOA: %u bytes\n", qoa_len);

        // Write QOA data to file
        FILE* output_file = fopen(output_path, "wb");
        if (!output_file) {
            fprintf(stderr, "WARNING: Failed to open output file '%s', skipping\n", output_path);
            free(sample_data);
            free(qoa_data);
            free(output_path);
            continue;
        }

        size_t written = fwrite(qoa_data, 1, qoa_len, output_file);
        fclose(output_file);

        if (written != qoa_len) {
            fprintf(stderr, "WARNING: Failed to write complete QOA data to '%s', skipping\n", output_path);
            free(sample_data);
            free(qoa_data);
            free(output_path);
            continue;
        }

        printf("Successfully converted '%s' to QOA format!\n", input_path);

        // Clean up
        free(sample_data);
        free(qoa_data);
        free(output_path);
    }

    return 0;
}

