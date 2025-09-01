#include "minilua.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

#define QOA_IMPLEMENTATION
#include "qoa.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_WIN32_IO
#include "dr_flac.h"

#include "stb_vorbis.c"

#define ZIP_IMPLEMENTATION
#include "just_zip.h"

#undef L

static int l_load_image(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 4);
    if (!data) {
        lua_pushnil(L);
        lua_pushstring(L, stbi_failure_reason());
        return 2;
    }
    lua_pushinteger(L, width);
    lua_pushinteger(L, height);
    lua_pushinteger(L, channels);
    lua_pushlstring(L, (char*)data, (size_t)width * height * 4);
    stbi_image_free(data);
    return 4;
}

static int l_write_png(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    int width = luaL_checkinteger(L, 2);
    int height = luaL_checkinteger(L, 3);
    int channels = luaL_checkinteger(L, 4);
    size_t len;
    const char *data = luaL_checklstring(L, 5, &len);
    int stride = luaL_optinteger(L, 6, width * channels);
    int result = stbi_write_png(filename, width, height, channels, data, stride);
    lua_pushboolean(L, result != 0);
    return 1;
}

static int l_qoi_encode(lua_State *L) {
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);
    int width = luaL_checkinteger(L, 2);
    int height = luaL_checkinteger(L, 3);
    int channels = luaL_checkinteger(L, 4);
    int colorspace = luaL_optinteger(L, 5, 0);
    qoi_desc desc = {width, height, channels, colorspace};
    int out_len;
    void *out = qoi_encode((unsigned char*)data, &desc, &out_len);
    if (!out) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushlstring(L, (char*)out, out_len);
    free(out);
    return 1;
}

static int l_load_wav(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    drwav wav;
    if (!drwav_init_file(&wav, filename, NULL)) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to load WAV");
        return 2;
    }
    unsigned int channels = wav.channels;
    unsigned int samplerate = wav.sampleRate;
    unsigned int total_samples = (unsigned int)wav.totalPCMFrameCount;
    short *data = malloc(total_samples * channels * sizeof(short));
    if (!data) {
        drwav_uninit(&wav);
        lua_pushnil(L);
        lua_pushstring(L, "Out of memory");
        return 2;
    }
    drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, data);
    drwav_uninit(&wav);
    lua_pushinteger(L, channels);
    lua_pushinteger(L, samplerate);
    lua_pushinteger(L, total_samples);
    lua_pushlstring(L, (char*)data, total_samples * channels * sizeof(short));
    free(data);
    return 4;
}

static int l_load_ogg(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    int channels, samplerate;
    short *data;
    int samples = stb_vorbis_decode_filename(filename, &channels, &samplerate, &data);
    if (samples <= 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to load OGG");
        return 2;
    }
    lua_pushinteger(L, channels);
    lua_pushinteger(L, samplerate);
    lua_pushinteger(L, samples);
    lua_pushlstring(L, (char*)data, samples * channels * sizeof(short));
    free(data);
    return 4;
}

static int l_load_mp3(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    drmp3 mp3;
    if (!drmp3_init_file(&mp3, filename, NULL)) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to load MP3");
        return 2;
    }
    unsigned int channels = mp3.channels;
    unsigned int samplerate = mp3.sampleRate;
    drmp3_uint64 total_frames = drmp3_get_pcm_frame_count(&mp3);
    unsigned int samples = (unsigned int)total_frames;
    float *float_data = malloc(samples * channels * sizeof(float));
    if (!float_data) {
        drmp3_uninit(&mp3);
        lua_pushnil(L);
        lua_pushstring(L, "Out of memory");
        return 2;
    }
    drmp3_read_pcm_frames_f32(&mp3, total_frames, float_data);
    drmp3_uninit(&mp3);
    short *data = malloc(samples * channels * sizeof(short));
    if (!data) {
        free(float_data);
        lua_pushnil(L);
        lua_pushstring(L, "Out of memory");
        return 2;
    }
    for (unsigned int i = 0; i < samples * channels; i++) {
        float sample = float_data[i];
        if (sample >= 1.0f) data[i] = 32767;
        else if (sample <= -1.0f) data[i] = -32768;
        else data[i] = (short)(sample * 32767.0f);
    }
    free(float_data);
    lua_pushinteger(L, channels);
    lua_pushinteger(L, samplerate);
    lua_pushinteger(L, samples);
    lua_pushlstring(L, (char*)data, samples * channels * sizeof(short));
    free(data);
    return 4;
}

static int l_load_flac(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    unsigned int channels, samplerate;
    drflac_uint64 total_frames;
    short *data = drflac_open_file_and_read_pcm_frames_s16(filename, &channels, &samplerate, &total_frames, NULL);
    if (!data) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to load FLAC");
        return 2;
    }
    unsigned int samples = (unsigned int)total_frames;
    lua_pushinteger(L, channels);
    lua_pushinteger(L, samplerate);
    lua_pushinteger(L, samples);
    lua_pushlstring(L, (char*)data, samples * channels * sizeof(short));
    free(data);
    return 4;
}

static int l_qoa_encode(lua_State *L) {
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);
    int channels = luaL_checkinteger(L, 2);
    int samplerate = luaL_checkinteger(L, 3);
    int samples = luaL_checkinteger(L, 4);
    qoa_desc desc = {channels, samplerate, samples};
    unsigned int out_len;
    void *out = qoa_encode((short*)data, &desc, &out_len);
    if (!out) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushlstring(L, (char*)out, out_len);
    free(out);
    return 1;
}

static int l_create_zip(lua_State *L) {
    const char *zipname = luaL_checkstring(L, 1);
    if (!lua_istable(L, 2)) {
        lua_pushnil(L);
        lua_pushstring(L, "Second argument must be a table of paths");
        return 2;
    }
    zip *z = zip_open(zipname, "w");
    if (!z) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to create zip file");
        return 2;
    }
    lua_pushnil(L);  // first key
    while (lua_next(L, 2) != 0) {
        const char *path = luaL_checkstring(L, -1);
        const char *entryname = NULL;
        // Check if the key is a string (custom entry name) or number (use path as entry name)
        if (lua_type(L, -2) == LUA_TSTRING)
            entryname = lua_tostring(L, -2);
        
        FILE *f = fopen(path, "rb");
        if (!f) {
            zip_close(z);
            lua_pushnil(L);
            lua_pushstring(L, "Failed to open file");
            return 2;
        }
        if (!zip_append_file_ex(z, path, entryname, f, 6)) {
            fclose(f);
            zip_close(z);
            lua_pushnil(L);
            lua_pushstring(L, "Failed to append file");
            return 2;
        }
        fclose(f);
        lua_pop(L, 1);  // remove value, keep key for next iteration
    }
    zip_close(z);
    lua_pushboolean(L, 1);
    return 1;
}

static const luaL_Reg nicepkg_funcs[] = {
    {"load", l_load_image},
    {"write_png", l_write_png},
    {"qoi_encode", l_qoi_encode},
    {"load_wav", l_load_wav},
    {"load_ogg", l_load_ogg},
    {"load_mp3", l_load_mp3},
    {"load_flac", l_load_flac},
    {"qoa_encode", l_qoa_encode},
    {"create_zip", l_create_zip},
    {NULL, NULL}
};

int luaopen_nicepkg(lua_State *L) {
    luaL_newlib(L, nicepkg_funcs);
    return 1;
}
