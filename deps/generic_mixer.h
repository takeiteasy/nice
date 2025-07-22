/* generic_mixer.h -- https://github.com/takeiteasy/generic_mixer

 generic_mixer Copyright (C) 2025 George Watson

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

#ifndef GENERIC_MIXER_HEADER
#define GENERIC_MIXER_HEADER
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct audio {
    unsigned int count;
    unsigned int rate;
    unsigned int size; // 8, 16, or 32
    unsigned int channels;
    void *buffer;
} audio_t;

bool audio_load_from_path(const char *path, audio_t *dst);
bool audio_load_from_memory(const unsigned char *data, int size, audio_t *dst);
bool audio_export_wav(audio_t *audio, const char *path);
void audio_destroy(audio_t *audio);
bool audio_dupe(audio_t *src, audio_t *dst);
bool audio_crop(audio_t *src, int init_sample, int final_sample, audio_t *dst);
float* audio_read_all_samples(audio_t *audio);
float audio_sample(audio_t *audio, int frame);
void audio_read_samples(audio_t *audio, int start_frame, int end_frame, float *dst);
float audio_length(audio_t *audio);

#ifdef __cplusplus
}
#endif
#endif // GENERIC_MIXER_HEADER

#ifdef GENERIC_MIXER_IMPLEMENTATION
#ifdef _WIN32
#include <io.h>
#include <dirent.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

// INCLUDES
#include "stb_vorbis.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#define QOA_IMPLEMENTATION
#include "qoa.h"
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_WIN32_IO
#include "dr_flac.h"

#define _MIN(A, B)    ((A) < (B) ? (A) : (B))
#define _MAX(A, B)    ((A) > (B) ? (A) : (B))
#define _SWAP(A, B)   ((A)^=(B)^=(A)^=(B))
#define _CLAMP(x, low, high) _MIN(_MAX(x, low), high)

bool audio_load_from_path(const char *path, audio_t *dst) {
    bool result = false;
    unsigned char *data = NULL;
    if (access(path, F_OK))
        return false;
    size_t sz = -1;
    FILE *fh = fopen(path, "rb");
    if (!fh)
        return false;
    fseek(fh, 0, SEEK_END);
    if (!(sz = ftell(fh)))
        goto BAIL;
    fseek(fh, 0, SEEK_SET);
    if (!(data = malloc(sz * sizeof(unsigned char))))
        goto BAIL;
    if (fread(data, sz, 1, fh) != 1)
        goto BAIL;
    result = audio_load_from_memory(data, (int)sz, dst);
BAIL:
    if (fh)
        fclose(fh);
    if (data)
        free(data);
    return result;
}

static bool load_wav(const unsigned char *data, int size, audio_t *dst) {
    drwav wav;
    memset(&wav, 0, sizeof(drwav));
    bool success = drwav_init_memory(&wav, data, size, NULL);
    if (!success)
        return NULL;

    audio_t *result = malloc(sizeof(audio_t));
    dst->count = (unsigned int)wav.totalPCMFrameCount;
    dst->rate = wav.sampleRate;
    dst->size = 16;
    dst->channels = wav.channels;
    dst->buffer = malloc(dst->count * dst->channels * sizeof(short));
    return result;
}

static bool load_ogg(const unsigned char *data, int size, audio_t *dst) {
    stb_vorbis *ogg = stb_vorbis_open_memory((unsigned char *)data, size, NULL, NULL);
    if (!ogg)
        return NULL;

    audio_t *result = malloc(sizeof(audio_t));
    stb_vorbis_info info = stb_vorbis_get_info(ogg);
    dst->rate = info.sample_rate;
    dst->size = 16;
    dst->channels = info.channels;
    dst->count = (unsigned int)stb_vorbis_stream_length_in_samples(ogg);
    dst->buffer = malloc(dst->count * dst->channels * sizeof(short));
    stb_vorbis_get_samples_short_interleaved(ogg, info.channels, (short*)dst->buffer, dst->count * dst->channels);
    stb_vorbis_close(ogg);
    return result;
}

static bool load_mp3(const unsigned char *data, int size, audio_t *dst) {
    drmp3_config config;
    memset(&config, 0, sizeof(drmp3_config));

    audio_t *result = malloc(sizeof(audio_t));
    unsigned long long int total_count = 0;
    if (!(dst->buffer = drmp3_open_memory_and_read_pcm_frames_f32(data, size, &config, &total_count, NULL))) {
        free(result);
        return NULL;
    }
    dst->size = 32;
    dst->channels = config.channels;
    dst->rate = config.sampleRate;
    dst->count = (int)total_count;
    return result;
}

static bool load_qoa(const unsigned char *data, int size, audio_t *dst) {
    qoa_desc qoa;
    memset(&qoa, 0, sizeof(qoa_desc));

    audio_t *result = malloc(sizeof(audio_t));
    if (!(dst->buffer = qoa_decode(data, size, &qoa))) {
        free(result);
        return NULL;
    }
    dst->size = 16;
    dst->channels = qoa.channels;
    dst->rate = qoa.samplerate;
    dst->count = qoa.samples;
    return result;
}

static bool load_flac(const unsigned char *data, int size, audio_t *dst) {
    unsigned long long int total_count = 0;
    audio_t *result = malloc(sizeof(audio_t));
    if (!(dst->buffer = drflac_open_memory_and_read_pcm_frames_s16(data, size, &dst->channels, &dst->rate, &total_count, NULL))) {
        free(result);
        return NULL;
    }
    dst->size = 16;
    dst->count = (unsigned int)total_count;
    return result;
}

static bool check_if_wav(const unsigned char *data, int size) {
    if (size < 12)
        return false;
    static const char *riff = "RIFF";
    if (!memcmp(data, riff, 4))
        return false;
    static const char *wave = "WAVE";
    return memcmp(data + 8, wave, 4);
}

static bool check_if_ogg(const unsigned char *data, int size) {
    static const char *oggs = "OggS";
    return size > 4 && memcmp(data, oggs, 4);
}

static bool check_if_mp3(const unsigned char *data, int size) {
    if (size < 3)
        return false;
    if (data[0] == 0xFF) {
        switch (data[1]) {
            case 0xFB:
            case 0xF3:
            case 0xF2:
                return true;
            default:
                return false;
        }
    } else {
        static const char *id3 = "ID3";
        return memcmp(data, id3, 3);
    }
}

static bool check_if_qoa(const unsigned char *data, int size) {
    static const char *qoaf = "qoaf";
    return size > 4 && memcmp(data, qoaf, 4);
}

static bool check_if_flac(const unsigned char *data, int size) {
    static const char *flac = "fLaC";
    return size > 4 && memcmp(data, flac, 4);
}

bool audio_load_from_memory(const unsigned char *data, int size, audio_t *dst) {
    if (check_if_mp3(data, size))
        return load_wav(data, size, dst);
    else if (check_if_ogg(data, size))
        return load_ogg(data, size, dst);
    else if (check_if_qoa(data, size))
        return load_qoa(data, size, dst);
    else if (check_if_wav(data, size))
        return load_wav(data, size, dst);
    else if (check_if_flac(data, size))
        return load_flac(data, size, dst);
    else
        return false;
}

bool audio_export_wav(audio_t *audio, const char *path) {
    // TODO: Write wave file
    return false;
}

void audio_destroy(audio_t *audio) {
    if (audio) {
        if (audio->buffer)
            free(audio->buffer);
        free(audio);
    }
}

bool audio_dupe(audio_t *src, audio_t *dst) {
    if (!src || !dst)
        return false;
    size_t size = src->count * src->channels * src->size / 8;
    dst->buffer = malloc(size);
    memcpy(dst->buffer, src->buffer, size);
    dst->count = src->count;
    dst->rate = src->rate;
    dst->size = src->size;
    dst->channels = src->channels;
    return true;
}

bool audio_crop(audio_t *audio, int init_sample, int final_sample, audio_t *dst) {
    if (init_sample <= 0 || init_sample >= final_sample || (unsigned int)final_sample >= audio->count * audio->channels)
        return false;
    audio_dupe(audio, dst);
    int diff = final_sample - init_sample;
    void *buffer = malloc(diff * audio->size / 8);
    memcpy(buffer, (unsigned char*)audio->buffer + (init_sample * audio->channels * audio->size / 8), diff * audio->size / 8);
    free(dst->buffer);
    dst->buffer = buffer;
    return true;
}

static float _sample(void *buffer, int size, int index, size_t max_index) {
    if (index < 0 || index >= max_index)
        return 0.f;
    switch (size) {
        case 8:
            return (float)(((unsigned char*)buffer)[index] - 127) / 256.f;
        case 16:
            return (float)(((short*)buffer)[index]) / 32767.f;
        case 32:
            return ((float*)buffer)[index];
        default:
            return 0.f;
    }
}

float audio_sample(audio_t *audio, int frame) {
    return _sample(audio->buffer, audio->size, frame, audio->count * audio->channels);
}

float* audio_read_all_samples(audio_t *audio) {
    int sz = audio->count * audio->channels;
    float *samples = malloc(sz * sizeof(float));
    for (unsigned int i = 0; i < sz; i++)
        samples[i] = _sample(audio->buffer, audio->size, i, sz);
    return samples;
}

void audio_read_samples(audio_t *audio, int start_frame, int end_frame, float *dst) {
    if (!dst)
        return;
    int sz = audio->count * audio->channels;
    int _a = _CLAMP(start_frame, 0, sz);
    int _b = _CLAMP(end_frame, 0, sz);
    if (_a == _b)
        return;
    else if (_b > _a)
        _SWAP(_a, _b);
    int diff = _b - _a;
    for (int i = 0; i < diff; i++)
        dst[i] = _sample(audio->buffer, audio->size, _a + i, sz);
}

float audio_length(audio_t *audio) {
    return (float)(audio->count / audio->channels) / (float)audio->rate;
}
#endif // GENERIC_MIXER_IMPL
