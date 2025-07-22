/* generic_image.h -- https://github.com/takeiteasy/generic_image

 generic_image Copyright (C) 2025 George Watson

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

#ifndef GENERIC_IMAGE_HEADER
#define GENERIC_IMAGE_HEADER
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef union color_t {
    struct {
        uint8_t r, g, b, a;
    };
    uint32_t rgba;
} color_t;

#ifdef __cplusplus
#define _CONSTEXPR constexpr
#else
#if __STDC_VERSION__ >= 202311L
#define _CONSTEXPR constexpr
#else
#define _CONSTEXPR const
#endif
#endif

#ifndef NO_COLOR_CONSTANTS
#define COLOR_ALICE_BLUE { 240, 248, 255, 255 }
#define COLOR_ALICE_BLUE_FLT { 0.941176471f, 0.97254902f, 1.f, 1.f }
#define COLOR_ALICE_BLUE_INT 0xF0F8FFFF
static _CONSTEXPR color_t color_alice_blue = COLOR_ALICE_BLUE;
#define COLOR_ANTIQUE_WHITE { 250, 235, 215, 255 }
#define COLOR_ANTIQUE_WHITE_FLT { 0.980392157f, 0.921568627f, 0.843137255f, 1.f }
#define COLOR_ANTIQUE_WHITE_INT 0xFAEBD7FF
static _CONSTEXPR color_t color_antique_white = COLOR_ANTIQUE_WHITE;
#define COLOR_AQUA { 0, 255, 255, 255 }
#define COLOR_AQUA_FLT { 0.f, 1.f, 1.f, 1.f }
#define COLOR_AQUA_INT 0x00FFFFFF
static _CONSTEXPR color_t color_aqua = COLOR_AQUA;
#define COLOR_AQUAMARINE { 127, 255, 212, 255 }
#define COLOR_AQUAMARINE_FLT { 0.498039216f, 1.f, 0.831372549f, 1.f }
#define COLOR_AQUAMARINE_INT 0x7FFFD4FF
static _CONSTEXPR color_t color_aquamarine = COLOR_AQUAMARINE;
#define COLOR_AZURE { 240, 255, 255, 255 }
#define COLOR_AZURE_FLT { 0.941176471f, 1.f, 1.f, 1.f }
#define COLOR_AZURE_INT 0xF0FFFFFF
static _CONSTEXPR color_t color_azure = COLOR_AZURE;
#define COLOR_BEIGE { 245, 245, 220, 255 }
#define COLOR_BEIGE_FLT { 0.960784314f, 0.960784314f, 0.862745098f, 1.f }
#define COLOR_BEIGE_INT 0xF5F5DCFF
static _CONSTEXPR color_t color_beige = COLOR_BEIGE;
#define COLOR_BISQUE { 255, 228, 196, 255 }
#define COLOR_BISQUE_FLT { 1.f, 0.894117647f, 0.768627451f, 1.f }
#define COLOR_BISQUE_INT 0xFFE4C4FF
static _CONSTEXPR color_t color_bisque = COLOR_BISQUE;
#define COLOR_BLACK { 0, 0, 0, 255 }
#define COLOR_BLACK_FLT { 0.f, 0.f, 0.f, 1.f }
#define COLOR_BLACK_INT 0x000000FF
static _CONSTEXPR color_t color_black = COLOR_BLACK;
#define COLOR_BLANCHED_ALMOND { 255, 235, 205, 255 }
#define COLOR_BLANCHED_ALMOND_FLT { 1.f, 0.921568627f, 0.803921569f, 1.f }
#define COLOR_BLANCHED_ALMOND_INT 0xFFEBCDFF
static _CONSTEXPR color_t color_blanched_almond = COLOR_BLANCHED_ALMOND;
#define COLOR_BLUE { 0, 0, 255, 255 }
#define COLOR_BLUE_FLT { 0.f, 0.f, 1.f, 1.f }
#define COLOR_BLUE_INT 0x0000FFFF
static _CONSTEXPR color_t color_blue = COLOR_BLUE;
#define COLOR_BLUE_VIOLET { 138, 43, 226, 255 }
#define COLOR_BLUE_VIOLET_FLT { 0.541176471f, 0.168627451f, 0.88627451f, 1.f }
#define COLOR_BLUE_VIOLET_INT 0x8A2BE2FF
static _CONSTEXPR color_t color_blue_violet = COLOR_BLUE_VIOLET;
#define COLOR_BROWN { 165, 42, 42, 255 }
#define COLOR_BROWN_FLT { 0.647058824f, 0.164705882f, 0.164705882f, 1.f }
#define COLOR_BROWN_INT 0xA52A2AFF
static _CONSTEXPR color_t color_brown = COLOR_BROWN;
#define COLOR_BURLYWOOD { 222, 184, 135, 255 }
#define COLOR_BURLYWOOD_FLT { 0.870588235f, 0.721568627f, 0.529411765f, 1.f }
#define COLOR_BURLYWOOD_INT 0xDEB887FF
static _CONSTEXPR color_t color_burlywood = COLOR_BURLYWOOD;
#define COLOR_CADET_BLUE { 95, 158, 160, 255 }
#define COLOR_CADET_BLUE_FLT { 0.37254902f, 0.619607843f, 0.62745098f, 1.f }
#define COLOR_CADET_BLUE_INT 0x5F9EA0FF
static _CONSTEXPR color_t color_cadet_blue = COLOR_CADET_BLUE;
#define COLOR_CHARTREUSE { 127, 255, 0, 255 }
#define COLOR_CHARTREUSE_FLT { 0.498039216f, 1.f, 0.f, 1.f }
#define COLOR_CHARTREUSE_INT 0x7FFF00FF
static _CONSTEXPR color_t color_chartreuse = COLOR_CHARTREUSE;
#define COLOR_CHOCOLATE { 210, 105, 30, 255 }
#define COLOR_CHOCOLATE_FLT { 0.823529412f, 0.411764706f, 0.117647059f, 1.f }
#define COLOR_CHOCOLATE_INT 0xD2691EFF
static _CONSTEXPR color_t color_chocolate = COLOR_CHOCOLATE;
#define COLOR_CORAL { 255, 127, 80, 255 }
#define COLOR_CORAL_FLT { 1.f, 0.498039216f, 0.31372549f, 1.f }
#define COLOR_CORAL_INT 0xFF7F50FF
static _CONSTEXPR color_t color_coral = COLOR_CORAL;
#define COLOR_CORNFLOWER_BLUE { 100, 149, 237, 255 }
#define COLOR_CORNFLOWER_BLUE_FLT { 0.392156863f, 0.584313725f, 0.929411765f, 1.f }
#define COLOR_CORNFLOWER_BLUE_INT 0x6495EDFF
static _CONSTEXPR color_t color_cornflower_blue = COLOR_CORNFLOWER_BLUE;
#define COLOR_CORNSILK { 255, 248, 220, 255 }
#define COLOR_CORNSILK_FLT { 1.f, 0.97254902f, 0.862745098f, 1.f }
#define COLOR_CORNSILK_INT 0xFFF8DCFF
static _CONSTEXPR color_t color_cornsilk = COLOR_CORNSILK;
#define COLOR_CRIMSON { 220, 20, 60, 255 }
#define COLOR_CRIMSON_FLT { 0.862745098f, 0.0784313725f, 0.235294118f, 1.f }
#define COLOR_CRIMSON_INT 0xDC143CFF
static _CONSTEXPR color_t color_crimson = COLOR_CRIMSON;
#define COLOR_CYAN { 0, 255, 255, 255 }
#define COLOR_CYAN_FLT { 0.f, 1.f, 1.f, 1.f }
#define COLOR_CYAN_INT 0x00FFFFFF
static _CONSTEXPR color_t color_cyan = COLOR_CYAN;
#define COLOR_DARK_BLUE { 0, 0, 139, 255 }
#define COLOR_DARK_BLUE_FLT { 0.f, 0.f, 0.545098039f, 1.f }
#define COLOR_DARK_BLUE_INT 0x00008BFF
static _CONSTEXPR color_t color_dark_blue = COLOR_DARK_BLUE;
#define COLOR_DARK_CYAN { 0, 139, 139, 255 }
#define COLOR_DARK_CYAN_FLT { 0.f, 0.545098039f, 0.545098039f, 1.f }
#define COLOR_DARK_CYAN_INT 0x008B8BFF
static _CONSTEXPR color_t color_dark_cyan = COLOR_DARK_CYAN;
#define COLOR_DARK_GOLDENROD { 184, 134, 11, 255 }
#define COLOR_DARK_GOLDENROD_FLT { 0.721568627f, 0.525490196f, 0.0431372549f, 1.f }
#define COLOR_DARK_GOLDENROD_INT 0xB8860BFF
static _CONSTEXPR color_t color_dark_goldenrod = COLOR_DARK_GOLDENROD;
#define COLOR_DARK_GRAY { 169, 169, 169, 255 }
#define COLOR_DARK_GRAY_FLT { 0.662745098f, 0.662745098f, 0.662745098f, 1.f }
#define COLOR_DARK_GRAY_INT 0xA9A9A9FF
static _CONSTEXPR color_t color_dark_gray = COLOR_DARK_GRAY;
#define COLOR_DARK_GREEN { 0, 100, 0, 255 }
#define COLOR_DARK_GREEN_FLT { 0.f, 0.392156863f, 0.f, 1.f }
#define COLOR_DARK_GREEN_INT 0x006400FF
static _CONSTEXPR color_t color_dark_green = COLOR_DARK_GREEN;
#define COLOR_DARK_KHAKI { 189, 183, 107, 255 }
#define COLOR_DARK_KHAKI_FLT { 0.741176471f, 0.717647059f, 0.419607843f, 1.f }
#define COLOR_DARK_KHAKI_INT 0xBDB76BFF
static _CONSTEXPR color_t color_dark_khaki = COLOR_DARK_KHAKI;
#define COLOR_DARK_MAGENTA { 139, 0, 139, 255 }
#define COLOR_DARK_MAGENTA_FLT { 0.545098039f, 0.f, 0.545098039f, 1.f }
#define COLOR_DARK_MAGENTA_INT 0x8B008BFF
static _CONSTEXPR color_t color_dark_magenta = COLOR_DARK_MAGENTA;
#define COLOR_DARK_OLIVE_GREEN { 85, 107, 47, 255 }
#define COLOR_DARK_OLIVE_GREEN_FLT { 0.333333333f, 0.419607843f, 0.184313725f, 1.f }
#define COLOR_DARK_OLIVE_GREEN_INT 0x556B2FFF
static _CONSTEXPR color_t color_dark_olive_green = COLOR_DARK_OLIVE_GREEN;
#define COLOR_DARK_ORANGE { 255, 140, 0, 255 }
#define COLOR_DARK_ORANGE_FLT { 1.f, 0.549019608f, 0.f, 1.f }
#define COLOR_DARK_ORANGE_INT 0xFF8C00FF
static _CONSTEXPR color_t color_dark_orange = COLOR_DARK_ORANGE;
#define COLOR_DARK_ORCHID { 153, 50, 204, 255 }
#define COLOR_DARK_ORCHID_FLT { 0.6f, 0.196078431f, 0.8f, 1.f }
#define COLOR_DARK_ORCHID_INT 0x9932CCFF
static _CONSTEXPR color_t color_dark_orchid = COLOR_DARK_ORCHID;
#define COLOR_DARK_RED { 139, 0, 0, 255 }
#define COLOR_DARK_RED_FLT { 0.545098039f, 0.f, 0.f, 1.f }
#define COLOR_DARK_RED_INT 0x8B0000FF
static _CONSTEXPR color_t color_dark_red = COLOR_DARK_RED;
#define COLOR_DARK_SALMON { 233, 150, 122, 255 }
#define COLOR_DARK_SALMON_FLT { 0.91372549f, 0.588235294f, 0.478431373f, 1.f }
#define COLOR_DARK_SALMON_INT 0xE9967AFF
static _CONSTEXPR color_t color_dark_salmon = COLOR_DARK_SALMON;
#define COLOR_DARK_SEA_GREEN { 143, 188, 143, 255 }
#define COLOR_DARK_SEA_GREEN_FLT { 0.560784314f, 0.737254902f, 0.560784314f, 1.f }
#define COLOR_DARK_SEA_GREEN_INT 0x8FBC8FFF
static _CONSTEXPR color_t color_dark_sea_green = COLOR_DARK_SEA_GREEN;
#define COLOR_DARK_SLATE_BLUE { 72, 61, 139, 255 }
#define COLOR_DARK_SLATE_BLUE_FLT { 0.282352941f, 0.239215686f, 0.545098039f, 1.f }
#define COLOR_DARK_SLATE_BLUE_INT 0x483D8BFF
static _CONSTEXPR color_t color_dark_slate_blue = COLOR_DARK_SLATE_BLUE;
#define COLOR_DARK_SLATE_GRAY { 47, 79, 79, 255 }
#define COLOR_DARK_SLATE_GRAY_FLT { 0.184313725f, 0.309803922f, 0.309803922f, 1.f }
#define COLOR_DARK_SLATE_GRAY_INT 0x2F4F4FFF
static _CONSTEXPR color_t color_dark_slate_gray = COLOR_DARK_SLATE_GRAY;
#define COLOR_DARK_TURQUOISE { 0, 206, 209, 255 }
#define COLOR_DARK_TURQUOISE_FLT { 0.f, 0.807843137f, 0.819607843f, 1.f }
#define COLOR_DARK_TURQUOISE_INT 0x00CED1FF
static _CONSTEXPR color_t color_dark_turquoise = COLOR_DARK_TURQUOISE;
#define COLOR_DARK_VIOLET { 148, 0, 211, 255 }
#define COLOR_DARK_VIOLET_FLT { 0.580392157f, 0.f, 0.82745098f, 1.f }
#define COLOR_DARK_VIOLET_INT 0x9400D3FF
static _CONSTEXPR color_t color_dark_violet = COLOR_DARK_VIOLET;
#define COLOR_DEEP_PINK { 255, 20, 147, 255 }
#define COLOR_DEEP_PINK_FLT { 1.f, 0.0784313725f, 0.576470588f, 1.f }
#define COLOR_DEEP_PINK_INT 0xFF1493FF
static _CONSTEXPR color_t color_deep_pink = COLOR_DEEP_PINK;
#define COLOR_DEEP_SKY_BLUE { 0, 191, 255, 255 }
#define COLOR_DEEP_SKY_BLUE_FLT { 0.f, 0.749019608f, 1.f, 1.f }
#define COLOR_DEEP_SKY_BLUE_INT 0x00BFFFFF
static _CONSTEXPR color_t color_deep_sky_blue = COLOR_DEEP_SKY_BLUE;
#define COLOR_DIM_GRAY { 105, 105, 105, 255 }
#define COLOR_DIM_GRAY_FLT { 0.411764706f, 0.411764706f, 0.411764706f, 1.f }
#define COLOR_DIM_GRAY_INT 0x696969FF
static _CONSTEXPR color_t color_dim_gray = COLOR_DIM_GRAY;
#define COLOR_DODGER_BLUE { 30, 144, 255, 255 }
#define COLOR_DODGER_BLUE_FLT { 0.117647059f, 0.564705882f, 1.f, 1.f }
#define COLOR_DODGER_BLUE_INT 0x1E90FFFF
static _CONSTEXPR color_t color_dodger_blue = COLOR_DODGER_BLUE;
#define COLOR_FIREBRICK { 178, 34, 34, 255 }
#define COLOR_FIREBRICK_FLT { 0.698039216f, 0.133333333f, 0.133333333f, 1.f }
#define COLOR_FIREBRICK_INT 0xB22222FF
static _CONSTEXPR color_t color_firebrick = COLOR_FIREBRICK;
#define COLOR_FLORAL_WHITE { 255, 250, 240, 255 }
#define COLOR_FLORAL_WHITE_FLT { 1.f, 0.980392157f, 0.941176471f, 1.f }
#define COLOR_FLORAL_WHITE_INT 0xFFFAF0FF
static _CONSTEXPR color_t color_floral_white = COLOR_FLORAL_WHITE;
#define COLOR_FOREST_GREEN { 34, 139, 34, 255 }
#define COLOR_FOREST_GREEN_FLT { 0.133333333f, 0.545098039f, 0.133333333f, 1.f }
#define COLOR_FOREST_GREEN_INT 0x228B22FF
static _CONSTEXPR color_t color_forest_green = COLOR_FOREST_GREEN;
#define COLOR_FUCHSIA { 255, 0, 255, 255 }
#define COLOR_FUCHSIA_FLT { 1.f, 0.f, 1.f, 1.f }
#define COLOR_FUCHSIA_INT 0xFF00FFFF
static _CONSTEXPR color_t color_fuchsia = COLOR_FUCHSIA;
#define COLOR_GAINSBORO { 220, 220, 220, 255 }
#define COLOR_GAINSBORO_FLT { 0.862745098f, 0.862745098f, 0.862745098f, 1.f }
#define COLOR_GAINSBORO_INT 0xDCDCDCFF
static _CONSTEXPR color_t color_gainsboro = COLOR_GAINSBORO;
#define COLOR_GHOST_WHITE { 248, 248, 255, 255 }
#define COLOR_GHOST_WHITE_FLT { 0.97254902f, 0.97254902f, 1.f, 1.f }
#define COLOR_GHOST_WHITE_INT 0xF8F8FFFF
static _CONSTEXPR color_t color_ghost_white = COLOR_GHOST_WHITE;
#define COLOR_GOLD { 255, 215, 0, 255 }
#define COLOR_GOLD_FLT { 1.f, 0.843137255f, 0.f, 1.f }
#define COLOR_GOLD_INT 0xFFD700FF
static _CONSTEXPR color_t color_gold = COLOR_GOLD;
#define COLOR_GOLDENROD { 218, 165, 32, 255 }
#define COLOR_GOLDENROD_FLT { 0.854901961f, 0.647058824f, 0.125490196f, 1.f }
#define COLOR_GOLDENROD_INT 0xDAA520FF
static _CONSTEXPR color_t color_goldenrod = COLOR_GOLDENROD;
#define COLOR_GRAY { 190, 190, 190, 255 }
#define COLOR_GRAY_FLT { 0.745098039f, 0.745098039f, 0.745098039f, 1.f }
#define COLOR_GRAY_INT 0xBEBEBEFF
static _CONSTEXPR color_t color_gray = COLOR_GRAY;
#define COLOR_WEB_GRAY { 128, 128, 128, 255 }
#define COLOR_WEB_GRAY_FLT { 0.501960784f, 0.501960784f, 0.501960784f, 1.f }
#define COLOR_WEB_GRAY_INT 0x808080FF
static _CONSTEXPR color_t color_web_gray = COLOR_WEB_GRAY;
#define COLOR_GREEN { 0, 255, 0, 255 }
#define COLOR_GREEN_FLT { 0.f, 1.f, 0.f, 1.f }
#define COLOR_GREEN_INT 0x00FF00FF
static _CONSTEXPR color_t color_green = COLOR_GREEN;
#define COLOR_WEB_GREEN { 0, 128, 0, 255 }
#define COLOR_WEB_GREEN_FLT { 0.f, 0.501960784f, 0.f, 1.f }
#define COLOR_WEB_GREEN_INT 0x008000FF
static _CONSTEXPR color_t color_web_green = COLOR_WEB_GREEN;
#define COLOR_GREEN_YELLOW { 173, 255, 47, 255 }
#define COLOR_GREEN_YELLOW_FLT { 0.678431373f, 1.f, 0.184313725f, 1.f }
#define COLOR_GREEN_YELLOW_INT 0xADFF2FFF
static _CONSTEXPR color_t color_green_yellow = COLOR_GREEN_YELLOW;
#define COLOR_HONEYDEW { 240, 255, 240, 255 }
#define COLOR_HONEYDEW_FLT { 0.941176471f, 1.f, 0.941176471f, 1.f }
#define COLOR_HONEYDEW_INT 0xF0FFF0FF
static _CONSTEXPR color_t color_honeydew = COLOR_HONEYDEW;
#define COLOR_HOT_PINK { 255, 105, 180, 255 }
#define COLOR_HOT_PINK_FLT { 1.f, 0.411764706f, 0.705882353f, 1.f }
#define COLOR_HOT_PINK_INT 0xFF69B4FF
static _CONSTEXPR color_t color_hot_pink = COLOR_HOT_PINK;
#define COLOR_INDIAN_RED { 205, 92, 92, 255 }
#define COLOR_INDIAN_RED_FLT { 0.803921569f, 0.360784314f, 0.360784314f, 1.f }
#define COLOR_INDIAN_RED_INT 0xCD5C5CFF
static _CONSTEXPR color_t color_indian_red = COLOR_INDIAN_RED;
#define COLOR_INDIGO { 75, 0, 130, 255 }
#define COLOR_INDIGO_FLT { 0.294117647f, 0.f, 0.509803922f, 1.f }
#define COLOR_INDIGO_INT 0x4B0082FF
static _CONSTEXPR color_t color_indigo = COLOR_INDIGO;
#define COLOR_IVORY { 255, 255, 240, 255 }
#define COLOR_IVORY_FLT { 1.f, 1.f, 0.941176471f, 1.f }
#define COLOR_IVORY_INT 0xFFFFF0FF
static _CONSTEXPR color_t color_ivory = COLOR_IVORY;
#define COLOR_KHAKI { 240, 230, 140, 255 }
#define COLOR_KHAKI_FLT { 0.941176471f, 0.901960784f, 0.549019608f, 1.f }
#define COLOR_KHAKI_INT 0xF0E68CFF
static _CONSTEXPR color_t color_khaki = COLOR_KHAKI;
#define COLOR_LAVENDER { 230, 230, 250, 255 }
#define COLOR_LAVENDER_FLT { 0.901960784f, 0.901960784f, 0.980392157f, 1.f }
#define COLOR_LAVENDER_INT 0xE6E6FAFF
static _CONSTEXPR color_t color_lavender = COLOR_LAVENDER;
#define COLOR_LAVENDER_BLUSH { 255, 240, 245, 255 }
#define COLOR_LAVENDER_BLUSH_FLT { 1.f, 0.941176471f, 0.960784314f, 1.f }
#define COLOR_LAVENDER_BLUSH_INT 0xFFF0F5FF
static _CONSTEXPR color_t color_lavender_blush = COLOR_LAVENDER_BLUSH;
#define COLOR_LAWN_GREEN { 124, 252, 0, 255 }
#define COLOR_LAWN_GREEN_FLT { 0.48627451f, 0.988235294f, 0.f, 1.f }
#define COLOR_LAWN_GREEN_INT 0x7CFC00FF
static _CONSTEXPR color_t color_lawn_green = COLOR_LAWN_GREEN;
#define COLOR_LEMON_CHIFFON { 255, 250, 205, 255 }
#define COLOR_LEMON_CHIFFON_FLT { 1.f, 0.980392157f, 0.803921569f, 1.f }
#define COLOR_LEMON_CHIFFON_INT 0xFFFACDFF
static _CONSTEXPR color_t color_lemon_chiffon = COLOR_LEMON_CHIFFON;
#define COLOR_LIGHT_BLUE { 173, 216, 230, 255 }
#define COLOR_LIGHT_BLUE_FLT { 0.678431373f, 0.847058824f, 0.901960784f, 1.f }
#define COLOR_LIGHT_BLUE_INT 0xADD8E6FF
static _CONSTEXPR color_t color_light_blue = COLOR_LIGHT_BLUE;
#define COLOR_LIGHT_CORAL { 240, 128, 128, 255 }
#define COLOR_LIGHT_CORAL_FLT { 0.941176471f, 0.501960784f, 0.501960784f, 1.f }
#define COLOR_LIGHT_CORAL_INT 0xF08080FF
static _CONSTEXPR color_t color_light_coral = COLOR_LIGHT_CORAL;
#define COLOR_LIGHT_CYAN { 224, 255, 255, 255 }
#define COLOR_LIGHT_CYAN_FLT { 0.878431373f, 1.f, 1.f, 1.f }
#define COLOR_LIGHT_CYAN_INT 0xE0FFFFFF
static _CONSTEXPR color_t color_light_cyan = COLOR_LIGHT_CYAN;
#define COLOR_LIGHT_GOLDENROD { 250, 250, 210, 255 }
#define COLOR_LIGHT_GOLDENROD_FLT { 0.980392157f, 0.980392157f, 0.823529412f, 1.f }
#define COLOR_LIGHT_GOLDENROD_INT 0xFAFAD2FF
static _CONSTEXPR color_t color_light_goldenrod = COLOR_LIGHT_GOLDENROD;
#define COLOR_LIGHT_GRAY { 211, 211, 211, 255 }
#define COLOR_LIGHT_GRAY_FLT { 0.82745098f, 0.82745098f, 0.82745098f, 1.f }
#define COLOR_LIGHT_GRAY_INT 0xD3D3D3FF
static _CONSTEXPR color_t color_light_gray = COLOR_LIGHT_GRAY;
#define COLOR_LIGHT_GREEN { 144, 238, 144, 255 }
#define COLOR_LIGHT_GREEN_FLT { 0.564705882f, 0.933333333f, 0.564705882f, 1.f }
#define COLOR_LIGHT_GREEN_INT 0x90EE90FF
static _CONSTEXPR color_t color_light_green = COLOR_LIGHT_GREEN;
#define COLOR_LIGHT_PINK { 255, 182, 193, 255 }
#define COLOR_LIGHT_PINK_FLT { 1.f, 0.71372549f, 0.756862745f, 1.f }
#define COLOR_LIGHT_PINK_INT 0xFFB6C1FF
static _CONSTEXPR color_t color_light_pink = COLOR_LIGHT_PINK;
#define COLOR_LIGHT_SALMON { 255, 160, 122, 255 }
#define COLOR_LIGHT_SALMON_FLT { 1.f, 0.62745098f, 0.478431373f, 1.f }
#define COLOR_LIGHT_SALMON_INT 0xFFA07AFF
static _CONSTEXPR color_t color_light_salmon = COLOR_LIGHT_SALMON;
#define COLOR_LIGHT_SEA_GREEN { 32, 178, 170, 255 }
#define COLOR_LIGHT_SEA_GREEN_FLT { 0.125490196f, 0.698039216f, 0.666666667f, 1.f }
#define COLOR_LIGHT_SEA_GREEN_INT 0x20B2AAFF
static _CONSTEXPR color_t color_light_sea_green = COLOR_LIGHT_SEA_GREEN;
#define COLOR_LIGHT_SKY_BLUE { 135, 206, 250, 255 }
#define COLOR_LIGHT_SKY_BLUE_FLT { 0.529411765f, 0.807843137f, 0.980392157f, 1.f }
#define COLOR_LIGHT_SKY_BLUE_INT 0x87CEFAFF
static _CONSTEXPR color_t color_light_sky_blue = COLOR_LIGHT_SKY_BLUE;
#define COLOR_LIGHT_SLATE_GRAY { 119, 136, 153, 255 }
#define COLOR_LIGHT_SLATE_GRAY_FLT { 0.466666667f, 0.533333333f, 0.6f, 1.f }
#define COLOR_LIGHT_SLATE_GRAY_INT 0x778899FF
static _CONSTEXPR color_t color_light_slate_gray = COLOR_LIGHT_SLATE_GRAY;
#define COLOR_LIGHT_STEEL_BLUE { 176, 196, 222, 255 }
#define COLOR_LIGHT_STEEL_BLUE_FLT { 0.690196078f, 0.768627451f, 0.870588235f, 1.f }
#define COLOR_LIGHT_STEEL_BLUE_INT 0xB0C4DEFF
static _CONSTEXPR color_t color_light_steel_blue = COLOR_LIGHT_STEEL_BLUE;
#define COLOR_LIGHT_YELLOW { 255, 255, 224, 255 }
#define COLOR_LIGHT_YELLOW_FLT { 1.f, 1.f, 0.878431373f, 1.f }
#define COLOR_LIGHT_YELLOW_INT 0xFFFFE0FF
static _CONSTEXPR color_t color_light_yellow = COLOR_LIGHT_YELLOW;
#define COLOR_LIME { 0, 255, 0, 255 }
#define COLOR_LIME_FLT { 0.f, 1.f, 0.f, 1.f }
#define COLOR_LIME_INT 0x00FF00FF
static _CONSTEXPR color_t color_lime = COLOR_LIME;
#define COLOR_LIME_GREEN { 50, 205, 50, 255 }
#define COLOR_LIME_GREEN_FLT { 0.196078431f, 0.803921569f, 0.196078431f, 1.f }
#define COLOR_LIME_GREEN_INT 0x32CD32FF
static _CONSTEXPR color_t color_lime_green = COLOR_LIME_GREEN;
#define COLOR_LINEN { 250, 240, 230, 255 }
#define COLOR_LINEN_FLT { 0.980392157f, 0.941176471f, 0.901960784f, 1.f }
#define COLOR_LINEN_INT 0xFAF0E6FF
static _CONSTEXPR color_t color_linen = COLOR_LINEN;
#define COLOR_MAGENTA { 255, 0, 255, 255 }
#define COLOR_MAGENTA_FLT { 1.f, 0.f, 1.f, 1.f }
#define COLOR_MAGENTA_INT 0xFF00FFFF
static _CONSTEXPR color_t color_magenta = COLOR_MAGENTA;
#define COLOR_MAROON { 176, 48, 96, 255 }
#define COLOR_MAROON_FLT { 0.690196078f, 0.188235294f, 0.376470588f, 1.f }
#define COLOR_MAROON_INT 0xB03060FF
static _CONSTEXPR color_t color_maroon = COLOR_MAROON;
#define COLOR_WEB_MAROON { 128, 0, 0, 255 }
#define COLOR_WEB_MAROON_FLT { 0.501960784f, 0.f, 0.f, 1.f }
#define COLOR_WEB_MAROON_INT 0x800000FF
static _CONSTEXPR color_t color_web_maroon = COLOR_WEB_MAROON;
#define COLOR_MEDIUM_AQUAMARINE { 102, 205, 170, 255 }
#define COLOR_MEDIUM_AQUAMARINE_FLT { 0.4f, 0.803921569f, 0.666666667f, 1.f }
#define COLOR_MEDIUM_AQUAMARINE_INT 0x66CDAAFF
static _CONSTEXPR color_t color_medium_aquamarine = COLOR_MEDIUM_AQUAMARINE;
#define COLOR_MEDIUM_BLUE { 0, 0, 205, 255 }
#define COLOR_MEDIUM_BLUE_FLT { 0.f, 0.f, 0.803921569f, 1.f }
#define COLOR_MEDIUM_BLUE_INT 0x0000CDFF
static _CONSTEXPR color_t color_medium_blue = COLOR_MEDIUM_BLUE;
#define COLOR_MEDIUM_ORCHID { 186, 85, 211, 255 }
#define COLOR_MEDIUM_ORCHID_FLT { 0.729411765f, 0.333333333f, 0.82745098f, 1.f }
#define COLOR_MEDIUM_ORCHID_INT 0xBA55D3FF
static _CONSTEXPR color_t color_medium_orchid = COLOR_MEDIUM_ORCHID;
#define COLOR_MEDIUM_PURPLE { 147, 112, 219, 255 }
#define COLOR_MEDIUM_PURPLE_FLT { 0.576470588f, 0.439215686f, 0.858823529f, 1.f }
#define COLOR_MEDIUM_PURPLE_INT 0x9370DBFF
static _CONSTEXPR color_t color_medium_purple = COLOR_MEDIUM_PURPLE;
#define COLOR_MEDIUM_SEA_GREEN { 60, 179, 113, 255 }
#define COLOR_MEDIUM_SEA_GREEN_FLT { 0.235294118f, 0.701960784f, 0.443137255f, 1.f }
#define COLOR_MEDIUM_SEA_GREEN_INT 0x3CB371FF
static _CONSTEXPR color_t color_medium_sea_green = COLOR_MEDIUM_SEA_GREEN;
#define COLOR_MEDIUM_SLATE_BLUE { 123, 104, 238, 255 }
#define COLOR_MEDIUM_SLATE_BLUE_FLT { 0.482352941f, 0.407843137f, 0.933333333f, 1.f }
#define COLOR_MEDIUM_SLATE_BLUE_INT 0x7B68EEFF
static _CONSTEXPR color_t color_medium_slate_blue = COLOR_MEDIUM_SLATE_BLUE;
#define COLOR_MEDIUM_SPRING_GREEN { 0, 250, 154, 255 }
#define COLOR_MEDIUM_SPRING_GREEN_FLT { 0.f, 0.980392157f, 0.603921569f, 1.f }
#define COLOR_MEDIUM_SPRING_GREEN_INT 0x00FA9AFF
static _CONSTEXPR color_t color_medium_spring_green = COLOR_MEDIUM_SPRING_GREEN;
#define COLOR_MEDIUM_TURQUOISE { 72, 209, 204, 255 }
#define COLOR_MEDIUM_TURQUOISE_FLT { 0.282352941f, 0.819607843f, 0.8f, 1.f }
#define COLOR_MEDIUM_TURQUOISE_INT 0x48D1CCFF
static _CONSTEXPR color_t color_medium_turquoise = COLOR_MEDIUM_TURQUOISE;
#define COLOR_MEDIUM_VIOLET_RED { 199, 21, 133, 255 }
#define COLOR_MEDIUM_VIOLET_RED_FLT { 0.780392157f, 0.0823529412f, 0.521568627f, 1.f }
#define COLOR_MEDIUM_VIOLET_RED_INT 0xC71585FF
static _CONSTEXPR color_t color_medium_violet_red = COLOR_MEDIUM_VIOLET_RED;
#define COLOR_MIDNIGHT_BLUE { 25, 25, 112, 255 }
#define COLOR_MIDNIGHT_BLUE_FLT { 0.0980392157f, 0.0980392157f, 0.439215686f, 1.f }
#define COLOR_MIDNIGHT_BLUE_INT 0x191970FF
static _CONSTEXPR color_t color_midnight_blue = COLOR_MIDNIGHT_BLUE;
#define COLOR_MINT_CREAM { 245, 255, 250, 255 }
#define COLOR_MINT_CREAM_FLT { 0.960784314f, 1.f, 0.980392157f, 1.f }
#define COLOR_MINT_CREAM_INT 0xF5FFFAFF
static _CONSTEXPR color_t color_mint_cream = COLOR_MINT_CREAM;
#define COLOR_MISTY_ROSE { 255, 228, 225, 255 }
#define COLOR_MISTY_ROSE_FLT { 1.f, 0.894117647f, 0.882352941f, 1.f }
#define COLOR_MISTY_ROSE_INT 0xFFE4E1FF
static _CONSTEXPR color_t color_misty_rose = COLOR_MISTY_ROSE;
#define COLOR_MOCCASIN { 255, 228, 181, 255 }
#define COLOR_MOCCASIN_FLT { 1.f, 0.894117647f, 0.709803922f, 1.f }
#define COLOR_MOCCASIN_INT 0xFFE4B5FF
static _CONSTEXPR color_t color_moccasin = COLOR_MOCCASIN;
#define COLOR_NAVAJO_WHITE { 255, 222, 173, 255 }
#define COLOR_NAVAJO_WHITE_FLT { 1.f, 0.870588235f, 0.678431373f, 1.f }
#define COLOR_NAVAJO_WHITE_INT 0xFFDEADFF
static _CONSTEXPR color_t color_navajo_white = COLOR_NAVAJO_WHITE;
#define COLOR_NAVY_BLUE { 0, 0, 128, 255 }
#define COLOR_NAVY_BLUE_FLT { 0.f, 0.f, 0.501960784f, 1.f }
#define COLOR_NAVY_BLUE_INT 0x000080FF
static _CONSTEXPR color_t color_navy_blue = COLOR_NAVY_BLUE;
#define COLOR_OLD_LACE { 253, 245, 230, 255 }
#define COLOR_OLD_LACE_FLT { 0.992156863f, 0.960784314f, 0.901960784f, 1.f }
#define COLOR_OLD_LACE_INT 0xFDF5E6FF
static _CONSTEXPR color_t color_old_lace = COLOR_OLD_LACE;
#define COLOR_OLIVE { 128, 128, 0, 255 }
#define COLOR_OLIVE_FLT { 0.501960784f, 0.501960784f, 0.f, 1.f }
#define COLOR_OLIVE_INT 0x808000FF
static _CONSTEXPR color_t color_olive = COLOR_OLIVE;
#define COLOR_OLIVE_DRAB { 107, 142, 35, 255 }
#define COLOR_OLIVE_DRAB_FLT { 0.419607843f, 0.556862745f, 0.137254902f, 1.f }
#define COLOR_OLIVE_DRAB_INT 0x6B8E23FF
static _CONSTEXPR color_t color_olive_drab = COLOR_OLIVE_DRAB;
#define COLOR_ORANGE { 255, 165, 0, 255 }
#define COLOR_ORANGE_FLT { 1.f, 0.647058824f, 0.f, 1.f }
#define COLOR_ORANGE_INT 0xFFA500FF
static _CONSTEXPR color_t color_orange = COLOR_ORANGE;
#define COLOR_ORANGE_RED { 255, 69, 0, 255 }
#define COLOR_ORANGE_RED_FLT { 1.f, 0.270588235f, 0.f, 1.f }
#define COLOR_ORANGE_RED_INT 0xFF4500FF
static _CONSTEXPR color_t color_orange_red = COLOR_ORANGE_RED;
#define COLOR_ORCHID { 218, 112, 214, 255 }
#define COLOR_ORCHID_FLT { 0.854901961f, 0.439215686f, 0.839215686f, 1.f }
#define COLOR_ORCHID_INT 0xDA70D6FF
static _CONSTEXPR color_t color_orchid = COLOR_ORCHID;
#define COLOR_PALE_GOLDENROD { 238, 232, 170, 255 }
#define COLOR_PALE_GOLDENROD_FLT { 0.933333333f, 0.909803922f, 0.666666667f, 1.f }
#define COLOR_PALE_GOLDENROD_INT 0xEEE8AAFF
static _CONSTEXPR color_t color_pale_goldenrod = COLOR_PALE_GOLDENROD;
#define COLOR_PALE_GREEN { 152, 251, 152, 255 }
#define COLOR_PALE_GREEN_FLT { 0.596078431f, 0.984313725f, 0.596078431f, 1.f }
#define COLOR_PALE_GREEN_INT 0x98FB98FF
static _CONSTEXPR color_t color_pale_green = COLOR_PALE_GREEN;
#define COLOR_PALE_TURQUOISE { 175, 238, 238, 255 }
#define COLOR_PALE_TURQUOISE_FLT { 0.68627451f, 0.933333333f, 0.933333333f, 1.f }
#define COLOR_PALE_TURQUOISE_INT 0xAFEEEEFF
static _CONSTEXPR color_t color_pale_turquoise = COLOR_PALE_TURQUOISE;
#define COLOR_PALE_VIOLET_RED { 219, 112, 147, 255 }
#define COLOR_PALE_VIOLET_RED_FLT { 0.858823529f, 0.439215686f, 0.576470588f, 1.f }
#define COLOR_PALE_VIOLET_RED_INT 0xDB7093FF
static _CONSTEXPR color_t color_pale_violet_red = COLOR_PALE_VIOLET_RED;
#define COLOR_PAPAYA_WHIP { 255, 239, 213, 255 }
#define COLOR_PAPAYA_WHIP_FLT { 1.f, 0.937254902f, 0.835294118f, 1.f }
#define COLOR_PAPAYA_WHIP_INT 0xFFEFD5FF
static _CONSTEXPR color_t color_papaya_whip = COLOR_PAPAYA_WHIP;
#define COLOR_PEACH_PUFF { 255, 218, 185, 255 }
#define COLOR_PEACH_PUFF_FLT { 1.f, 0.854901961f, 0.725490196f, 1.f }
#define COLOR_PEACH_PUFF_INT 0xFFDAB9FF
static _CONSTEXPR color_t color_peach_puff = COLOR_PEACH_PUFF;
#define COLOR_PERU { 205, 133, 63, 255 }
#define COLOR_PERU_FLT { 0.803921569f, 0.521568627f, 0.247058824f, 1.f }
#define COLOR_PERU_INT 0xCD853FFF
static _CONSTEXPR color_t color_peru = COLOR_PERU;
#define COLOR_PINK { 255, 192, 203, 255 }
#define COLOR_PINK_FLT { 1.f, 0.752941176f, 0.796078431f, 1.f }
#define COLOR_PINK_INT 0xFFC0CBFF
static _CONSTEXPR color_t color_pink = COLOR_PINK;
#define COLOR_PLUM { 221, 160, 221, 255 }
#define COLOR_PLUM_FLT { 0.866666667f, 0.62745098f, 0.866666667f, 1.f }
#define COLOR_PLUM_INT 0xDDA0DDFF
static _CONSTEXPR color_t color_plum = COLOR_PLUM;
#define COLOR_POWDER_BLUE { 176, 224, 230, 255 }
#define COLOR_POWDER_BLUE_FLT { 0.690196078f, 0.878431373f, 0.901960784f, 1.f }
#define COLOR_POWDER_BLUE_INT 0xB0E0E6FF
static _CONSTEXPR color_t color_powder_blue = COLOR_POWDER_BLUE;
#define COLOR_PURPLE { 160, 32, 240, 255 }
#define COLOR_PURPLE_FLT { 0.62745098f, 0.125490196f, 0.941176471f, 1.f }
#define COLOR_PURPLE_INT 0xA020F0FF
static _CONSTEXPR color_t color_purple = COLOR_PURPLE;
#define COLOR_WEB_PURPLE { 128, 0, 128, 255 }
#define COLOR_WEB_PURPLE_FLT { 0.501960784f, 0.f, 0.501960784f, 1.f }
#define COLOR_WEB_PURPLE_INT 0x800080FF
static _CONSTEXPR color_t color_web_purple = COLOR_WEB_PURPLE;
#define COLOR_REBECCA_PURPLE { 102, 51, 153, 255 }
#define COLOR_REBECCA_PURPLE_FLT { 0.4f, 0.2f, 0.6f, 1.f }
#define COLOR_REBECCA_PURPLE_INT 0x663399FF
static _CONSTEXPR color_t color_rebecca_purple = COLOR_REBECCA_PURPLE;
#define COLOR_RED { 255, 0, 0, 255 }
#define COLOR_RED_FLT { 1.f, 0.f, 0.f, 1.f }
#define COLOR_RED_INT 0xFF0000FF
static _CONSTEXPR color_t color_red = COLOR_RED;
#define COLOR_ROSY_BROWN { 188, 143, 143, 255 }
#define COLOR_ROSY_BROWN_FLT { 0.737254902f, 0.560784314f, 0.560784314f, 1.f }
#define COLOR_ROSY_BROWN_INT 0xBC8F8FFF
static _CONSTEXPR color_t color_rosy_brown = COLOR_ROSY_BROWN;
#define COLOR_ROYAL_BLUE { 65, 105, 225, 255 }
#define COLOR_ROYAL_BLUE_FLT { 0.254901961f, 0.411764706f, 0.882352941f, 1.f }
#define COLOR_ROYAL_BLUE_INT 0x4169E1FF
static _CONSTEXPR color_t color_royal_blue = COLOR_ROYAL_BLUE;
#define COLOR_SADDLE_BROWN { 139, 69, 19, 255 }
#define COLOR_SADDLE_BROWN_FLT { 0.545098039f, 0.270588235f, 0.0745098039f, 1.f }
#define COLOR_SADDLE_BROWN_INT 0x8B4513FF
static _CONSTEXPR color_t color_saddle_brown = COLOR_SADDLE_BROWN;
#define COLOR_SALMON { 250, 128, 114, 255 }
#define COLOR_SALMON_FLT { 0.980392157f, 0.501960784f, 0.447058824f, 1.f }
#define COLOR_SALMON_INT 0xFA8072FF
static _CONSTEXPR color_t color_salmon = COLOR_SALMON;
#define COLOR_SANDY_BROWN { 244, 164, 96, 255 }
#define COLOR_SANDY_BROWN_FLT { 0.956862745f, 0.643137255f, 0.376470588f, 1.f }
#define COLOR_SANDY_BROWN_INT 0xF4A460FF
static _CONSTEXPR color_t color_sandy_brown = COLOR_SANDY_BROWN;
#define COLOR_SEA_GREEN { 46, 139, 87, 255 }
#define COLOR_SEA_GREEN_FLT { 0.180392157f, 0.545098039f, 0.341176471f, 1.f }
#define COLOR_SEA_GREEN_INT 0x2E8B57FF
static _CONSTEXPR color_t color_sea_green = COLOR_SEA_GREEN;
#define COLOR_SEASHELL { 255, 245, 238, 255 }
#define COLOR_SEASHELL_FLT { 1.f, 0.960784314f, 0.933333333f, 1.f }
#define COLOR_SEASHELL_INT 0xFFF5EEFF
static _CONSTEXPR color_t color_seashell = COLOR_SEASHELL;
#define COLOR_SIENNA { 160, 82, 45, 255 }
#define COLOR_SIENNA_FLT { 0.62745098f, 0.321568627f, 0.176470588f, 1.f }
#define COLOR_SIENNA_INT 0xA0522DFF
static _CONSTEXPR color_t color_sienna = COLOR_SIENNA;
#define COLOR_SILVER { 192, 192, 192, 255 }
#define COLOR_SILVER_FLT { 0.752941176f, 0.752941176f, 0.752941176f, 1.f }
#define COLOR_SILVER_INT 0xC0C0C0FF
static _CONSTEXPR color_t color_silver = COLOR_SILVER;
#define COLOR_SKY_BLUE { 135, 206, 235, 255 }
#define COLOR_SKY_BLUE_FLT { 0.529411765f, 0.807843137f, 0.921568627f, 1.f }
#define COLOR_SKY_BLUE_INT 0x87CEEBFF
static _CONSTEXPR color_t color_sky_blue = COLOR_SKY_BLUE;
#define COLOR_SLATE_BLUE { 106, 90, 205, 255 }
#define COLOR_SLATE_BLUE_FLT { 0.415686275f, 0.352941176f, 0.803921569f, 1.f }
#define COLOR_SLATE_BLUE_INT 0x6A5ACDFF
static _CONSTEXPR color_t color_slate_blue = COLOR_SLATE_BLUE;
#define COLOR_SLATE_GRAY { 112, 128, 144, 255 }
#define COLOR_SLATE_GRAY_FLT { 0.439215686f, 0.501960784f, 0.564705882f, 1.f }
#define COLOR_SLATE_GRAY_INT 0x708090FF
static _CONSTEXPR color_t color_slate_gray = COLOR_SLATE_GRAY;
#define COLOR_SNOW { 255, 250, 250, 255 }
#define COLOR_SNOW_FLT { 1.f, 0.980392157f, 0.980392157f, 1.f }
#define COLOR_SNOW_INT 0xFFFAFAFF
static _CONSTEXPR color_t color_snow = COLOR_SNOW;
#define COLOR_SPRING_GREEN { 0, 255, 127, 255 }
#define COLOR_SPRING_GREEN_FLT { 0.f, 1.f, 0.498039216f, 1.f }
#define COLOR_SPRING_GREEN_INT 0x00FF7FFF
static _CONSTEXPR color_t color_spring_green = COLOR_SPRING_GREEN;
#define COLOR_STEEL_BLUE { 70, 130, 180, 255 }
#define COLOR_STEEL_BLUE_FLT { 0.274509804f, 0.509803922f, 0.705882353f, 1.f }
#define COLOR_STEEL_BLUE_INT 0x4682B4FF
static _CONSTEXPR color_t color_steel_blue = COLOR_STEEL_BLUE;
#define COLOR_TAN { 210, 180, 140, 255 }
#define COLOR_TAN_FLT { 0.823529412f, 0.705882353f, 0.549019608f, 1.f }
#define COLOR_TAN_INT 0xD2B48CFF
static _CONSTEXPR color_t color_tan = COLOR_TAN;
#define COLOR_TEAL { 0, 128, 128, 255 }
#define COLOR_TEAL_FLT { 0.f, 0.501960784f, 0.501960784f, 1.f }
#define COLOR_TEAL_INT 0x008080FF
static _CONSTEXPR color_t color_teal = COLOR_TEAL;
#define COLOR_THISTLE { 216, 191, 216, 255 }
#define COLOR_THISTLE_FLT { 0.847058824f, 0.749019608f, 0.847058824f, 1.f }
#define COLOR_THISTLE_INT 0xD8BFD8FF
static _CONSTEXPR color_t color_thistle = COLOR_THISTLE;
#define COLOR_TOMATO { 255, 99, 71, 255 }
#define COLOR_TOMATO_FLT { 1.f, 0.388235294f, 0.278431373f, 1.f }
#define COLOR_TOMATO_INT 0xFF6347FF
static _CONSTEXPR color_t color_tomato = COLOR_TOMATO;
#define COLOR_TRANSPARENT { 0, 0, 0, 0 }
#define COLOR_TRANSPARENT_FLT { 0.f, 0.f, 0.f, 0.f }
#define COLOR_TRANSPARENT_INT 0x00000000
static _CONSTEXPR color_t color_transparent = COLOR_TRANSPARENT;
#define COLOR_TURQUOISE { 64, 224, 208, 255 }
#define COLOR_TURQUOISE_FLT { 0.250980392f, 0.878431373f, 0.815686275f, 1.f }
#define COLOR_TURQUOISE_INT 0x40E0D0FF
static _CONSTEXPR color_t color_turquoise = COLOR_TURQUOISE;
#define COLOR_VIOLET { 238, 130, 238, 255 }
#define COLOR_VIOLET_FLT { 0.933333333f, 0.509803922f, 0.933333333f, 1.f }
#define COLOR_VIOLET_INT 0xEE82EEFF
static _CONSTEXPR color_t color_violet = COLOR_VIOLET;
#define COLOR_WHEAT { 245, 222, 179, 255 }
#define COLOR_WHEAT_FLT { 0.960784314f, 0.870588235f, 0.701960784f, 1.f }
#define COLOR_WHEAT_INT 0xF5DEB3FF
static _CONSTEXPR color_t color_wheat = COLOR_WHEAT;
#define COLOR_WHITE { 255, 255, 255, 255 }
#define COLOR_WHITE_FLT { 1.f, 1.f, 1.f, 1.f }
#define COLOR_WHITE_INT 0xFFFFFFFF
static _CONSTEXPR color_t color_white = COLOR_WHITE;
#define COLOR_WHITE_SMOKE { 245, 245, 245, 255 }
#define COLOR_WHITE_SMOKE_FLT { 0.960784314f, 0.960784314f, 0.960784314f, 1.f }
#define COLOR_WHITE_SMOKE_INT 0xF5F5F5FF
static _CONSTEXPR color_t color_white_smoke = COLOR_WHITE_SMOKE;
#define COLOR_YELLOW { 255, 255, 0, 255 }
#define COLOR_YELLOW_FLT { 1.f, 1.f, 0.f, 1.f }
#define COLOR_YELLOW_INT 0xFFFF00FF
static _CONSTEXPR color_t color_yellow = COLOR_YELLOW;
#define COLOR_YELLOW_GREEN { 154, 205, 50, 255 }
#define COLOR_YELLOW_GREEN_FLT { 0.603921569f, 0.803921569f, 0.196078431f, 1.f }
#define COLOR_YELLOW_GREEN_INT 0x9ACD32FF
static _CONSTEXPR color_t color_yellow_green = COLOR_YELLOW_GREEN;
#endif

typedef color_t color_rgba_t;

typedef struct color_rgbaf_t {
    float r, g, b, a;
} color_rgbaf_t;

typedef struct color_hsva_t {
    float h, s, v, a;
} color_hsva_t;

typedef struct color_hsla_t {
    float h, s, l, a;
} color_hsla_t;

typedef struct color_lab_t {
    float l, a, b, alpha;
} color_lab_t;

typedef struct color_xyz_t {
    float x, y, z, a;
} color_xyz_t;

typedef struct color_yuv_t {
    float y, u, v, a;
} color_yuv_t;

typedef struct color_cmyk_t {
    float c, m, y, k, a;
} color_cmyk_t;

typedef struct color_rgb_t565 {
    uint16_t rgb565;
    uint8_t a;
} color_rgb_t565;

typedef uint32_t color_int_t;

color_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
color_t rgb(uint8_t r, uint8_t g, uint8_t b);
color_t rgbaf(float r, float g, float b, float a);
color_t rgbf(float r, float g, float b);
color_t hsva(float h, float s, float v, float a);
color_t hsla(float h, float s, float l, float a);
color_t lab(float l, float a, float b, float alpha);
color_t xyz(float x, float y, float z, float a);
color_t yuv(float y, float u, float v, float a);
color_t cmyk(float c, float m, float y, float k, float a);
color_t rgb_565(uint16_t rgb565, uint8_t a);

color_rgbaf_t rgba_to_rgbaf(color_t rgba);
color_t rgbaf_to_rgba(color_rgbaf_t rgbaf);
color_hsva_t rgba_to_hsva(color_t rgba);
color_t hsva_to_rgba(color_hsva_t hsva);
color_hsva_t rgbaf_to_hsva(color_rgbaf_t rgbaf);
color_rgbaf_t hsva_to_rgbaf(color_hsva_t hsva);
color_hsla_t rgba_to_hsla(color_t rgba);
color_t hsla_to_rgba(color_hsla_t hsla);
color_hsla_t rgbaf_to_hsla(color_rgbaf_t rgbaf);
color_rgbaf_t hsla_to_rgbaf(color_hsla_t hsla);
color_lab_t rgba_to_lab(color_t rgba);
color_t lab_to_rgba(color_lab_t lab);
color_xyz_t rgba_to_xyz(color_t rgba);
color_t xyz_to_rgba(color_xyz_t xyz);
color_xyz_t lab_to_xyz(color_lab_t lab);
color_lab_t xyz_to_lab(color_xyz_t xyz);
color_yuv_t rgba_to_yuv(color_t rgba);
color_t yuv_to_rgba(color_yuv_t yuv);
color_cmyk_t rgba_to_cmyk(color_t rgba);
color_t cmyk_to_rgba(color_cmyk_t cmyk);
color_rgb_t565 rgba_to_rgb565(color_t rgba);
color_t rgb565_to_rgba(color_rgb_t565 rgb565);

bool color_cmp(color_t a, color_t b);
float color_distance_lab(color_lab_t a, color_lab_t b);
float color_distance(color_t a, color_t b);
color_t color_lerp(color_t a, color_t b, float t);

// Luminance and brightness
float color_luminance(color_t color);             // Perceptual luminance (0-1)
float color_relative_luminance(color_t color);    // sRGB relative luminance
float color_brightness(color_t color);            // Simple average brightness
int color_is_dark(color_t color);                 // Returns 1 if dark, 0 if light

// Color properties
float color_saturation(color_t color);   // Get saturation level
float color_hue(color_t color);          // Get hue (0-360 degrees)
color_t color_grayscale(color_t color);  // Convert to grayscale
color_t color_invert(color_t color);     // Invert colors
color_t color_complement(color_t color); // Complementary color

// Additional analysis functions
int color_dominant_channel(color_t color);        // Get the dominant color channel
int color_is_grayscale(color_t color);            // Check if color is grayscale
float color_temperature_estimate(color_t color);  // Get color temperature estimate
int color_is_warm(color_t color);                 // Check if color is "warm" (reddish/yellowish)
int color_is_cool(color_t color);                 // Check if color is "cool" (bluish/greenish)
float color_purity(color_t color);                // Calculate color purity (0-1)
float color_energy(color_t color);                // Calculate color energy (0-1)

// Brightness and contrast
color_t color_brightness_adjust(color_t color, float amount);   // -1.0 to 1.0
color_t color_contrast_adjust(color_t color, float amount);     // -1.0 to 1.0
color_t color_gamma_adjust(color_t color, float gamma);         // Gamma correction

// Saturation and hue
color_t color_saturate(color_t color, float amount);    // 0.0 to 2.0+
color_t color_desaturate(color_t color, float amount);  // 0.0 to 1.0
color_t color_hue_shift(color_t color, float degrees);  // -360 to 360

// Temperature and tint
color_t color_temperature_adjust(color_t color, float kelvin);  // 1000-40000K
color_t color_tint(color_t color, float amount);                // -1.0 to 1.0

// Additional adjustments
color_t color_exposure_adjust(color_t color, float stops);
color_t color_highlights_adjust(color_t color, float amount);
color_t color_shadows_adjust(color_t color, float amount);
color_t color_whites_adjust(color_t color, float amount);
color_t color_blacks_adjust(color_t color, float amount);
color_t color_clarity_adjust(color_t color, float amount);
color_t color_vibrance_adjust(color_t color, float amount);

// Blending modes
color_t color_multiply(color_t a, color_t b);
color_t color_screen(color_t a, color_t b);
color_t color_overlay(color_t a, color_t b);
color_t color_soft_light(color_t a, color_t b);
color_t color_hard_light(color_t a, color_t b);
color_t color_color_dodge(color_t a, color_t b);
color_t color_color_burn(color_t a, color_t b);
color_t color_darken(color_t a, color_t b);
color_t color_lighten(color_t a, color_t b);
color_t color_difference(color_t a, color_t b);
color_t color_exclusion(color_t a, color_t b);

// Alpha blending
color_t color_alpha_blend(color_t fg, color_t bg);
color_t color_premultiply_alpha(color_t color);
color_t color_unpremultiply_alpha(color_t color);

// Color harmony
void color_analogous(color_t base, color_t* colors, int count);                       // Adjacent hues
void color_triadic(color_t base, color_t* color1, color_t* color2);                   // 120° apart
void color_tetradic(color_t base, color_t* color1, color_t* color2, color_t* color3); // Rectangle scheme
void color_split_complementary(color_t base, color_t* color1, color_t* color2);       // Split complement
void color_monochromatic(color_t base, color_t* colors, int count);                   // Same hue, different lightness

// Palette generation
void color_generate_gradient(color_t start, color_t end, color_t* colors, int count);
// void color_generate_palette_from_image(color_t* pixels, int count, color_t* palette, int palette_size);

// Color space conversions with custom white points
color_xyz_t rgba_to_xyz_custom(color_t rgba, float wx, float wy, float wz);
color_lab_t rgba_to_lab_custom(color_t rgba, float wx, float wy, float wz);

// Delta E calculations (color difference)
float color_delta_e_76(color_lab_t a, color_lab_t b);     // CIE76 formula
float color_delta_e_94(color_lab_t a, color_lab_t b);     // CIE94 formula
float color_delta_e_2000(color_lab_t a, color_lab_t b);   // CIEDE2000 formula

// Levels adjustment
color_t color_levels(color_t color, float black_point, float white_point, float gamma);
color_t color_curves(color_t color, float* curve_r, float* curve_g, float* curve_b);

// Color balance
color_t color_shadow_highlight(color_t color, float shadow, float highlight);
color_t color_color_balance(color_t color, float cyan_red, float magenta_green, float yellow_blue);

// Vibrance and saturation
color_t color_vibrance(color_t color, float amount);      // Smart saturation
color_t color_selective_color(color_t color, int channel, float cyan, float magenta, float yellow, float black);

// Quantization
color_t color_quantize(color_t color, int bits_per_channel);
color_t color_posterize(color_t color, int levels);
void color_reduce_palette(color_t* colors, int count, color_t* palette, int palette_size);

// Dithering
color_t color_dither_floyd_steinberg(color_t color, color_t* palette, int palette_size, int x, int y);
color_t color_dither_ordered(color_t color, color_t* palette, int palette_size, int x, int y);

// Color blindness simulation
color_t color_protanopia(color_t color);      // Red-blind
color_t color_deuteranopia(color_t color);    // Green-blind
color_t color_tritanopia(color_t color);      // Blue-blind
color_t color_achromatopsia(color_t color);   // Complete color blindness

// Accessibility
float color_contrast_ratio(color_t a, color_t b);          // WCAG contrast ratio
int color_wcag_aa_compliant(color_t fg, color_t bg);       // AA compliance
int color_wcag_aaa_compliant(color_t fg, color_t bg);      // AAA compliance

// Color matching
color_t color_match_closest(color_t target, color_t* palette, int palette_size);
// int color_find_dominant(color_t* pixels, int count);
// void color_histogram(color_t* pixels, int count, int* histogram);

// Color comparison
float color_similarity(color_t a, color_t b);
int color_is_similar(color_t a, color_t b, float threshold);

// Color grading
color_t color_lift_gamma_gain(color_t color, color_rgbaf_t lift, color_rgbaf_t gamma, color_rgbaf_t gain);
color_t color_shadow_midtone_highlight(color_t color, color_rgbaf_t shadow, color_rgbaf_t midtone, color_rgbaf_t highlight);

// Color spaces for specific industries
color_t color_rec709_to_rec2020(color_t color);    // HDR conversion
color_t color_prophoto_to_srgb(color_t color);     // Photography
color_t color_adobe_rgb_to_srgb(color_t color);    // Print/web

// Fast approximations
color_t color_fast_grayscale(color_t color);       // Integer-only grayscale
color_t color_fast_sepia(color_t color);           // Fast sepia tone
color_t color_fast_invert(color_t color);          // Bitwise invert

// Batch operations
// void color_batch_adjust_brightness(color_t* colors, int count, float brightness);
// void color_batch_apply_lut(color_t* colors, int count, color_t* lut);

typedef color_t* image_t;

image_t image_empty(unsigned int w, unsigned int h, color_t color);
image_t image_load_from_path(const char *path);
image_t image_load_from_memory(const void *data, size_t length);
bool image_export(const char *path, const image_t img);
void image_destroy(image_t img);

int image_width(const image_t img);
int image_height(const image_t img);
bool image_size(const image_t img, int *w, int *h);
bool image_pset(image_t img, int x, int y, color_t color);
color_t image_pget(const image_t img, int x, int y);

bool image_fill(image_t img, color_t color);
bool image_flood(image_t img, int x, int y, color_t color);
bool image_paste(image_t dst, const image_t src, int x, int y);
bool image_clipped_paste(image_t dst, const image_t src, int x, int y, int rx, int ry, int rw, int rh);
bool image_resize(image_t *src, int nw, int nh);
bool image_rotate(image_t *src, float angle);
bool image_clip(image_t *src, int rx, int ry, int rw, int rh);

image_t image_dupe(image_t src);
image_t image_resized(image_t src, int nw, int nh);
image_t image_rotated(image_t src, float angle);
image_t image_clipped(image_t src, int rx, int ry, int rw, int rh);

bool image_draw_line(image_t img, int x0, int y0, int x1, int y1, color_t color);
bool image_draw_circle(image_t img, int xc, int yc, int r, color_t color, int fill);
bool image_draw_rectangle(image_t img, int x, int y, int w, int h, color_t color, int fill);
bool image_draw_triangle(image_t img, int x0, int y0, int x1, int y1, int x2, int y2, color_t color, int fill);

#ifdef __cplusplus
}
#endif
#endif // GENERIC_IMAGE_HEADER

#ifdef GENERIC_IMAGE_IMPLEMENTATION
#ifdef _WIN32
#include <io.h>
#include <dirent.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// INCLUDES
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define QOI_IMPLEMENTATION
#include "qoi.h"

#define _RGBA(R, G, B, A) (((unsigned int)(R) << 24) | ((unsigned int)(B) << 16) | ((unsigned int)(G) << 8) | (A))
#define _RADIANS(N) (((double)(N)) * (M_PI / 180.))
#define _MIN(A, B) ((A) < (B) ? (A) : (B))
#define _MAX(A, B) ((A) > (B) ? (A) : (B))
#define _CLAMP(x, low, high) _MIN(_MAX(x, low), high)
#define _SWAP(A, B) ((A)^=(B)^=(A)^=(B))
#define _CLAMP_UINT8(V) ((uint8_t)_CLAMP((V), 0, 255))
#define _CLAMP_FLOAT(V) ((float)_CLAMP((V), 0.0f, 1.0f))
#define _CLAMP_FLOAT_RANGE(V, _MINV, _MAXV) (fminf(fmaxf(V, _MINV), _MAXV))
#define _FMIN3(A, B, C) (fminf(fminf((A), (B)), (C)))
#define _FMAX3(A, B, C) (fmaxf(fmaxf((A), (B)), (C)))
static _CONSTEXPR color_t _black = { 0.0f, 0.0f, 0.0f, 1.0f };

color_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (color_t){ r, g, b, a };   
}

color_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return rgba(r, g, b, 255);
}

color_t rgbaf(float r, float g, float b, float a) {
    return rgbaf_to_rgba((color_rgbaf_t){ r, g, b, a });
}
color_t rgbf(float r, float g, float b) {
    return rgbaf(r, g, b, 1.f);
}

color_t hsva(float h, float s, float v, float a) {
    return hsva_to_rgba((color_hsva_t){ h, s, v, a });
}

color_t hsla(float h, float s, float l, float a) {
    return hsla_to_rgba((color_hsla_t){ h, s, l, a });
}

color_t lab(float l, float a, float b, float alpha) {
    return lab_to_rgba((color_lab_t){ l, a, b, alpha });
}

color_t xyz(float x, float y, float z, float a) {
    return xyz_to_rgba((color_xyz_t){ x, y, z, a });
}

color_t yuv(float y, float u, float v, float a) {
    return yuv_to_rgba((color_yuv_t){ y, u, v, a });
}

color_t cmyk(float c, float m, float y, float k, float a) {
    return cmyk_to_rgba((color_cmyk_t){ c, m, y, k, a });
}

color_t rgb_565(uint16_t rgb565, uint8_t a) {
    return rgb565_to_rgba((color_rgb_t565){ rgb565, a });
}

// Basic RGBA <-> RGBAf conversions
color_rgbaf_t rgba_to_rgbaf(color_t rgba) {
    color_rgbaf_t result;
    result.r = rgba.r / 255.0f;
    result.g = rgba.g / 255.0f;
    result.b = rgba.b / 255.0f;
    result.a = rgba.a / 255.0f;
    return result;
}

color_t rgbaf_to_rgba(color_rgbaf_t rgbaf) {
    color_t result;
    result.r = _CLAMP_UINT8(rgbaf.r);
    result.g = _CLAMP_UINT8(rgbaf.g);
    result.b = _CLAMP_UINT8(rgbaf.b);
    result.a = _CLAMP_UINT8(rgbaf.a);
    return result;
}

// HSV conversions
color_hsva_t rgba_to_hsva(color_t rgba) {
    color_rgbaf_t rgbaf = rgba_to_rgbaf(rgba);
    return rgbaf_to_hsva(rgbaf);
}

color_hsva_t rgbaf_to_hsva(color_rgbaf_t rgbaf) {
    color_hsva_t hsva;
    float max = _FMAX3(rgbaf.r, rgbaf.g, rgbaf.b);
    float min = _FMIN3(rgbaf.r, rgbaf.g, rgbaf.b);
    float delta = max - min;
    
    // Value
    hsva.v = max;
    
    // Saturation
    hsva.s = (max == 0) ? 0 : delta / max;
    
    // Hue
    if (delta == 0) {
        hsva.h = 0; // achromatic
    } else {
        if (max == rgbaf.r) {
            hsva.h = (rgbaf.g - rgbaf.b) / delta + (rgbaf.g < rgbaf.b ? 6 : 0);
        } else if (max == rgbaf.g) {
            hsva.h = (rgbaf.b - rgbaf.r) / delta + 2;
        } else {
            hsva.h = (rgbaf.r - rgbaf.g) / delta + 4;
        }
        hsva.h /= 6.0f;
    }
    
    hsva.a = rgbaf.a;
    return hsva;
}

color_t hsva_to_rgba(color_hsva_t hsva) {
    color_rgbaf_t rgbaf = hsva_to_rgbaf(hsva);
    return rgbaf_to_rgba(rgbaf);
}

color_rgbaf_t hsva_to_rgbaf(color_hsva_t hsva) {
    color_rgbaf_t rgbaf;
    
    float c = hsva.v * hsva.s;
    float x = c * (1 - fabsf(fmodf(hsva.h * 6.0f, 2.0f) - 1));
    float m = hsva.v - c;
    
    float r1, g1, b1;
    
    if (hsva.h < 1.0f/6.0f) {
        r1 = c; g1 = x; b1 = 0;
    } else if (hsva.h < 2.0f/6.0f) {
        r1 = x; g1 = c; b1 = 0;
    } else if (hsva.h < 3.0f / 6.0f) {
        r1 = 0; g1 = c; b1 = x;
    } else if (hsva.h < 4.0f/6.0f) {
        r1 = 0; g1 = x; b1 = c;
    } else if (hsva.h < 5.0f/6.0f) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }

    rgbaf.r = r1 + m;
    rgbaf.g = g1 + m;
    rgbaf.b = b1 + m;
    rgbaf.a = hsva.a;
    return rgbaf;
}

// HSL conversions
color_hsla_t rgba_to_hsla(color_t rgba) {
    color_rgbaf_t rgbaf = rgba_to_rgbaf(rgba);
    return rgbaf_to_hsla(rgbaf);
}

color_hsla_t rgbaf_to_hsla(color_rgbaf_t rgbaf) {
    color_hsla_t hsla;
    float max = _FMAX3(rgbaf.r, rgbaf.g, rgbaf.b);
    float min = _FMIN3(rgbaf.r, rgbaf.g, rgbaf.b);
    float delta = max - min;
    
    // Lightness
    hsla.l = (max + min) / 2.0f;
    
    if (delta == 0)
        hsla.h = hsla.s = 0; // achromatic
    else {
        // Saturation
        hsla.s = hsla.l > 0.5f ? delta / (2.0f - max - min) : delta / (max + min);
        
        // Hue
        if (max == rgbaf.r)
            hsla.h = (rgbaf.g - rgbaf.b) / delta + (rgbaf.g < rgbaf.b ? 6 : 0);
        else if (max == rgbaf.g)
            hsla.h = (rgbaf.b - rgbaf.r) / delta + 2;
        else
            hsla.h = (rgbaf.r - rgbaf.g) / delta + 4;
        hsla.h /= 6.0f;
    }
    
    hsla.a = rgbaf.a;
    return hsla;
}

color_t hsla_to_rgba(color_hsla_t hsla) {
    color_rgbaf_t rgbaf = hsla_to_rgbaf(hsla);
    return rgbaf_to_rgba(rgbaf);
}

static inline float hue_to_rgb(float p, float q, float t) {
    if (t < 0)
        t += 1;
    if (t > 1)
        t -= 1;
    if (t < 1.0f/6.0f)
        return p + (q - p) * 6 * t;
    if (t < 1.0f/2.0f)
        return q;
    if (t < 2.0f/3.0f)
        return p + (q - p) * (2.0f/3.0f - t) * 6;
    return p;
}

color_rgbaf_t hsla_to_rgbaf(color_hsla_t hsla) {
    color_rgbaf_t rgbaf;
    
    if (hsla.s == 0)
        rgbaf.r = rgbaf.g = rgbaf.b = hsla.l; // achromatic
    else {
        float q = hsla.l < 0.5f ? hsla.l * (1 + hsla.s) : hsla.l + hsla.s - hsla.l * hsla.s;
        float p = 2 * hsla.l - q;
        
        rgbaf.r = hue_to_rgb(p, q, hsla.h + 1.0f/3.0f);
        rgbaf.g = hue_to_rgb(p, q, hsla.h);
        rgbaf.b = hue_to_rgb(p, q, hsla.h - 1.0f/3.0f);
    }
    
    rgbaf.a = hsla.a;
    return rgbaf;
}

// XYZ conversions (sRGB D65 white point)
color_xyz_t rgba_to_xyz(color_t rgba) {
    color_rgbaf_t rgbaf = rgba_to_rgbaf(rgba);
    
    // Apply gamma correction (sRGB to linear RGB)
    float r = (rgbaf.r > 0.04045f) ? powf((rgbaf.r + 0.055f) / 1.055f, 2.4f) : rgbaf.r / 12.92f;
    float g = (rgbaf.g > 0.04045f) ? powf((rgbaf.g + 0.055f) / 1.055f, 2.4f) : rgbaf.g / 12.92f;
    float b = (rgbaf.b > 0.04045f) ? powf((rgbaf.b + 0.055f) / 1.055f, 2.4f) : rgbaf.b / 12.92f;
    
    color_xyz_t xyz;
    
    // sRGB to XYZ matrix (D65)
    xyz.x = r * 0.4124564f + g * 0.3575761f + b * 0.1804375f;
    xyz.y = r * 0.2126729f + g * 0.7151522f + b * 0.0721750f;
    xyz.z = r * 0.0193339f + g * 0.1191920f + b * 0.9503041f;
    xyz.a = rgbaf.a;
    
    return xyz;
}

color_t xyz_to_rgba(color_xyz_t xyz) {
    // XYZ to sRGB matrix (D65)
    float r = xyz.x *  3.2404542f + xyz.y * -1.5371385f + xyz.z * -0.4985314f;
    float g = xyz.x * -0.9692660f + xyz.y *  1.8760108f + xyz.z *  0.0415560f;
    float b = xyz.x *  0.0556434f + xyz.y * -0.2040259f + xyz.z *  1.0572252f;
    
    // Apply gamma correction (linear RGB to sRGB)
    r = (r > 0.0031308f) ? 1.055f * powf(r, 1.0f/2.4f) - 0.055f : 12.92f * r;
    g = (g > 0.0031308f) ? 1.055f * powf(g, 1.0f/2.4f) - 0.055f : 12.92f * g;
    b = (b > 0.0031308f) ? 1.055f * powf(b, 1.0f/2.4f) - 0.055f : 12.92f * b;
    
    color_rgbaf_t rgbaf = {
        _CLAMP_FLOAT(r),
        _CLAMP_FLOAT(g),
        _CLAMP_FLOAT(b),
        xyz.a
    };
    return rgbaf_to_rgba(rgbaf);
}

// Lab conversions
color_lab_t xyz_to_lab(color_xyz_t xyz) {
    // D65 white point
    const float xn = 0.95047f;
    const float yn = 1.00000f;
    const float zn = 1.08883f;
    
    float x = xyz.x / xn;
    float y = xyz.y / yn;
    float z = xyz.z / zn;
    
    // Apply Lab transformation
    x = (x > 0.008856f) ? cbrtf(x) : (7.787f * x + 16.0f/116.0f);
    y = (y > 0.008856f) ? cbrtf(y) : (7.787f * y + 16.0f/116.0f);
    z = (z > 0.008856f) ? cbrtf(z) : (7.787f * z + 16.0f/116.0f);
    
    color_lab_t lab;
    lab.l = 116.0f * y - 16.0f;
    lab.a = 500.0f * (x - y);
    lab.b = 200.0f * (y - z);
    lab.alpha = xyz.a;
    
    return lab;
}

color_xyz_t lab_to_xyz(color_lab_t lab) {
    // D65 white point
    const float xn = 0.95047f;
    const float yn = 1.00000f;
    const float zn = 1.08883f;
    
    float fy = (lab.l + 16.0f) / 116.0f;
    float fx = lab.a / 500.0f + fy;
    float fz = fy - lab.b / 200.0f;
    
    float x = (fx > 0.206897f) ? fx*fx*fx : (fx - 16.0f/116.0f) / 7.787f;
    float y = (fy > 0.206897f) ? fy*fy*fy : (fy - 16.0f/116.0f) / 7.787f;
    float z = (fz > 0.206897f) ? fz*fz*fz : (fz - 16.0f/116.0f) / 7.787f;
    
    color_xyz_t xyz;
    xyz.x = x * xn;
    xyz.y = y * yn;
    xyz.z = z * zn;
    xyz.a = lab.alpha;
    
    return xyz;
}

color_lab_t rgba_to_lab(color_t rgba) {
    color_xyz_t xyz = rgba_to_xyz(rgba);
    return xyz_to_lab(xyz);
}

color_t lab_to_rgba(color_lab_t lab) {
    color_xyz_t xyz = lab_to_xyz(lab);
    return xyz_to_rgba(xyz);
}

// YUV conversions (BT.601)
color_yuv_t rgba_to_yuv(color_t rgba) {
    color_rgbaf_t rgbaf = rgba_to_rgbaf(rgba);
    
    color_yuv_t yuv;
    yuv.y = 0.299f * rgbaf.r + 0.587f * rgbaf.g + 0.114f * rgbaf.b;
    yuv.u = -0.14713f * rgbaf.r - 0.28886f * rgbaf.g + 0.436f * rgbaf.b;
    yuv.v = 0.615f * rgbaf.r - 0.51499f * rgbaf.g - 0.10001f * rgbaf.b;
    yuv.a = rgbaf.a;
    
    return yuv;
}

color_t yuv_to_rgba(color_yuv_t yuv) {
    color_rgbaf_t rgbaf;
    
    rgbaf.r = yuv.y + 1.13983f * yuv.v;
    rgbaf.g = yuv.y - 0.39465f * yuv.u - 0.58060f * yuv.v;
    rgbaf.b = yuv.y + 2.03211f * yuv.u;
    rgbaf.a = yuv.a;
    
    // Clamp values
    rgbaf.r = _CLAMP_FLOAT(rgbaf.r);
    rgbaf.g = _CLAMP_FLOAT(rgbaf.g);
    rgbaf.b = _CLAMP_FLOAT(rgbaf.b);

    return rgbaf_to_rgba(rgbaf);
}

// CMYK conversions
color_cmyk_t rgba_to_cmyk(color_t rgba) {
    color_rgbaf_t rgbaf = rgba_to_rgbaf(rgba);
    
    color_cmyk_t cmyk;
    // Find K (black)
    cmyk.k = 1.0f - _FMAX3(rgbaf.r, rgbaf.g, rgbaf.b);

    if (cmyk.k == 1.0f) // Pure black
        cmyk.c = cmyk.m = cmyk.y = 0.0f;
    else {
        float inv_k = 1.0f - cmyk.k;
        cmyk.c = (1.0f - rgbaf.r - cmyk.k) / inv_k;
        cmyk.m = (1.0f - rgbaf.g - cmyk.k) / inv_k;
        cmyk.y = (1.0f - rgbaf.b - cmyk.k) / inv_k;
    }
    
    cmyk.a = rgbaf.a;
    return cmyk;
}

color_t cmyk_to_rgba(color_cmyk_t cmyk) {
    color_rgbaf_t rgbaf;
    
    float inv_k = 1.0f - cmyk.k;
    rgbaf.r = (1.0f - cmyk.c) * inv_k;
    rgbaf.g = (1.0f - cmyk.m) * inv_k;
    rgbaf.b = (1.0f - cmyk.y) * inv_k;
    rgbaf.a = cmyk.a;
    
    return rgbaf_to_rgba(rgbaf);
}

// RGB565 conversions
color_rgb_t565 rgba_to_rgb565(color_t rgba) {
    color_rgb_t565 rgb565;
    
    // Convert to 5-6-5 format
    uint16_t r = (rgba.r >> 3) & 0x1F;
    uint16_t g = (rgba.g >> 2) & 0x3F;
    uint16_t b = (rgba.b >> 3) & 0x1F;
    
    rgb565.rgb565 = (r << 11) | (g << 5) | b;
    rgb565.a = rgba.a;
    
    return rgb565;
}

color_t rgb565_to_rgba(color_rgb_t565 rgb565) {
    color_t rgba;
    
    // Extract components
    uint16_t r = (rgb565.rgb565 >> 11) & 0x1F;
    uint16_t g = (rgb565.rgb565 >> 5) & 0x3F;
    uint16_t b = rgb565.rgb565 & 0x1F;
    
    // Scale back to 8-bit
    rgba.r = (r << 3) | (r >> 2);
    rgba.g = (g << 2) | (g >> 4);
    rgba.b = (b << 3) | (b >> 2);
    rgba.a = rgb565.a;
    
    return rgba;
}

float color_distance_lab(color_lab_t a, color_lab_t b) {
    float dl = a.l - b.l;
    float da = a.a - b.a;
    float db = a.b - b.b;
    return sqrtf(dl*dl + da*da + db*db);
}

float color_distance(color_t a, color_t b) {
    int dr = a.r - b.r;
    int dg = a.g - b.g;
    int db = a.b - b.b;
    return sqrtf(dr*dr + dg*dg + db*db);
}

color_t color_lerp(color_t a, color_t b, float t) {
    t = _CLAMP_FLOAT(t);
    color_t result;
    result.r = (uint8_t)(a.r + t * (b.r - a.r));
    result.g = (uint8_t)(a.g + t * (b.g - a.g));
    result.b = (uint8_t)(a.b + t * (b.b - a.b));
    result.a = (uint8_t)(a.a + t * (b.a - a.a));
    return result;
}

// Perceptual luminance using sRGB coefficients
float color_luminance(color_t color) {
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    return 0.299f * rgbaf.r + 0.587f * rgbaf.g + 0.114f * rgbaf.b;
}

// Relative luminance for WCAG calculations (uses gamma-corrected values)
float color_relative_luminance(color_t color) {
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    
    // Apply gamma correction (sRGB to linear RGB)
    float r = (rgbaf.r > 0.04045f) ? powf((rgbaf.r + 0.055f) / 1.055f, 2.4f) : rgbaf.r / 12.92f;
    float g = (rgbaf.g > 0.04045f) ? powf((rgbaf.g + 0.055f) / 1.055f, 2.4f) : rgbaf.g / 12.92f;
    float b = (rgbaf.b > 0.04045f) ? powf((rgbaf.b + 0.055f) / 1.055f, 2.4f) : rgbaf.b / 12.92f;
    
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// Simple average brightness
float color_brightness(color_t color) {
    return (color.r + color.g + color.b) / (3.0f * 255.0f);
}

// Check if color is considered dark (using perceptual luminance)
int color_is_dark(color_t color) {
    return color_luminance(color) < 0.5f;
}

// Get saturation level (0-1)
float color_saturation(color_t color) {
    color_hsva_t hsva = rgba_to_hsva(color);
    return hsva.s;
}

// Get hue in degrees (0-360)
float color_hue(color_t color) {
    color_hsva_t hsva = rgba_to_hsva(color);
    return hsva.h * 360.0f;
}

// Convert to grayscale using perceptual luminance
color_t color_grayscale(color_t color) {
    uint8_t gray = (uint8_t)(color_luminance(color) * 255.0f);
    color_t result = {gray, gray, gray, color.a};
    return result;
}

// Invert RGB channels (keeping alpha)
color_t color_invert(color_t color) {
    color_t result = {
        255 - color.r,
        255 - color.g,
        255 - color.b,
        color.a
    };
    return result;
}

// Get complementary color (opposite hue)
color_t color_complement(color_t color) {
    color_hsva_t hsva = rgba_to_hsva(color);
    hsva.h = fmodf(hsva.h + 0.5f, 1.0f);  // Add 180 degrees (0.5 in normalized hue)
    return hsva_to_rgba(hsva);
}

// Additional analysis functions

// Get the dominant color channel
int color_dominant_channel(color_t color) {
    if (color.r >= color.g && color.r >= color.b) return 0; // Red
    if (color.g >= color.b) return 1; // Green
    return 2; // Blue
}

// Check if color is grayscale (all channels equal within tolerance)
int color_is_grayscale(color_t color) {
    int max_diff = abs(color.r - color.g);
    max_diff = fmax(max_diff, abs(color.r - color.b));
    max_diff = fmax(max_diff, abs(color.g - color.b));
    return max_diff <= 2; // Allow small tolerance for rounding errors
}

// Get color temperature estimate (very rough approximation)
float color_temperature_estimate(color_t color) {
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    
    // Rough approximation based on blue/red ratio
    if (rgbaf.r == 0.0f) return 6500.0f; // Default daylight
    
    float ratio = rgbaf.b / rgbaf.r;
    if (ratio > 1.0f) {
        // More blue than red - cooler
        return 6500.0f + (ratio - 1.0f) * 3500.0f; // Up to 10000K
    } else {
        // More red than blue - warmer
        return 6500.0f - (1.0f - ratio) * 3500.0f; // Down to 3000K
    }
}

// Check if color is "warm" (reddish/yellowish)
int color_is_warm(color_t color) {
    return color.r > color.b && (color.r + color.g) > color.b * 1.5f;
}

// Check if color is "cool" (blueish/greenish)
int color_is_cool(color_t color) {
    return color.b > color.r && (color.b + color.g) > color.r * 1.5f;
}

// Get color purity (distance from gray)
float color_purity(color_t color) {
    uint8_t gray = (uint8_t)(color_luminance(color) * 255.0f);
    int dr = abs(color.r - gray);
    int dg = abs(color.g - gray);
    int db = abs(color.b - gray);
    return sqrtf(dr*dr + dg*dg + db*db) / (255.0f * sqrtf(3.0f));
}

// Get color energy (sum of all channels)
float color_energy(color_t color) {
    return (color.r + color.g + color.b) / (3.0f * 255.0f);
}

// Adjust brightness (-1.0 to 1.0, where 0 = no change)
color_t color_brightness_adjust(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    rgbaf.r = _CLAMP_FLOAT(rgbaf.r + amount);
    rgbaf.g = _CLAMP_FLOAT(rgbaf.g + amount);
    rgbaf.b = _CLAMP_FLOAT(rgbaf.b + amount);
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust contrast (-1.0 to 1.0, where 0 = no change)
color_t color_contrast_adjust(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    float factor = (1.0f + amount) / (1.0f - amount);
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    rgbaf.r = _CLAMP_FLOAT((rgbaf.r - 0.5f) * factor + 0.5f);
    rgbaf.g = _CLAMP_FLOAT((rgbaf.g - 0.5f) * factor + 0.5f);
    rgbaf.b = _CLAMP_FLOAT((rgbaf.b - 0.5f) * factor + 0.5f);
    
    return rgbaf_to_rgba(rgbaf);
}

// Apply gamma correction
color_t color_gamma_adjust(color_t color, float gamma) {
    if (gamma <= 0.0f) gamma = 0.01f; // Prevent division by zero
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    rgbaf.r = powf(rgbaf.r, 1.0f / gamma);
    rgbaf.g = powf(rgbaf.g, 1.0f / gamma);
    rgbaf.b = powf(rgbaf.b, 1.0f / gamma);
    
    return rgbaf_to_rgba(rgbaf);
}

// Increase saturation (0.0 to 2.0+, where 1.0 = no change)
color_t color_saturate(color_t color, float amount) {
    if (amount < 0.0f) amount = 0.0f;
    
    color_hsva_t hsva = rgba_to_hsva(color);
    hsva.s = _CLAMP_FLOAT(hsva.s * amount);
    
    return hsva_to_rgba(hsva);
}

// Decrease saturation (0.0 to 1.0, where 0.0 = grayscale, 1.0 = no change)
color_t color_desaturate(color_t color, float amount) {
    amount = _CLAMP_FLOAT(amount);
    
    color_hsva_t hsva = rgba_to_hsva(color);
    hsva.s *= amount;
    
    return hsva_to_rgba(hsva);
}

// Shift hue by degrees (-360 to 360)
color_t color_hue_shift(color_t color, float degrees) {
    float shift = fmodf(degrees / 360.0f, 1.0f);
    if (shift < 0.0f) shift += 1.0f;
    
    color_hsva_t hsva = rgba_to_hsva(color);
    hsva.h = fmodf(hsva.h + shift, 1.0f);
    
    return hsva_to_rgba(hsva);
}

// Adjust color temperature (1000K to 40000K, 6500K = daylight)
color_t color_temperature_adjust(color_t color, float kelvin) {
    kelvin = _CLAMP_FLOAT_RANGE(kelvin, 1000.0f, 40000.0f);
    
    // Temperature to RGB conversion (rough approximation)
    float temp = kelvin / 100.0f;
    float r, g, b;
    
    if (temp <= 66.0f) {
        r = 255.0f;
        g = temp;
        g = 99.4708025861f * logf(g) - 161.1195681661f;
    } else {
        r = temp - 60.0f;
        r = 329.698727446f * powf(r, -0.1332047592f);
        g = temp - 60.0f;
        g = 288.1221695283f * powf(g, -0.0755148492f);
    }
    
    if (temp >= 66.0f) {
        b = 255.0f;
    } else if (temp <= 19.0f) {
        b = 0.0f;
    } else {
        b = temp - 10.0f;
        b = 138.5177312231f * logf(b) - 305.0447927307f;
    }
    
    // Normalize and blend with original color
    r = _CLAMP_FLOAT_RANGE(r, 0.0f, 255.0f) / 255.0f;
    g = _CLAMP_FLOAT_RANGE(g, 0.0f, 255.0f) / 255.0f;
    b = _CLAMP_FLOAT_RANGE(b, 0.0f, 255.0f) / 255.0f;
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    rgbaf.r *= r;
    rgbaf.g *= g;
    rgbaf.b *= b;
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust tint (magenta-green balance, -1.0 to 1.0)
color_t color_tint(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    
    if (amount > 0.0f) {
        // Add magenta (increase red and blue, decrease green)
        rgbaf.r += amount * 0.1f;
        rgbaf.b += amount * 0.1f;
        rgbaf.g -= amount * 0.05f;
    } else {
        // Add green (increase green, decrease red and blue)
        rgbaf.g += (-amount) * 0.1f;
        rgbaf.r -= (-amount) * 0.05f;
        rgbaf.b -= (-amount) * 0.05f;
    }
    
    rgbaf.r = _CLAMP_FLOAT(rgbaf.r);
    rgbaf.g = _CLAMP_FLOAT(rgbaf.g);
    rgbaf.b = _CLAMP_FLOAT(rgbaf.b);
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust exposure in stops (-3.0 to 3.0, where 0 = no change)
color_t color_exposure_adjust(color_t color, float stops) {
    stops = _CLAMP_FLOAT_RANGE(stops, -3.0f, 3.0f);
    float factor = powf(2.0f, stops);
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    rgbaf.r = _CLAMP_FLOAT(rgbaf.r * factor);
    rgbaf.g = _CLAMP_FLOAT(rgbaf.g * factor);
    rgbaf.b = _CLAMP_FLOAT(rgbaf.b * factor);
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust highlights (-1.0 to 1.0, affects bright areas)
color_t color_highlights_adjust(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    float lum = color_luminance(color);
    
    // Only affect pixels above 50% luminance
    if (lum > 0.5f) {
        float weight = (lum - 0.5f) * 2.0f; // 0 to 1
        rgbaf.r = _CLAMP_FLOAT(rgbaf.r + amount * weight * 0.2f);
        rgbaf.g = _CLAMP_FLOAT(rgbaf.g + amount * weight * 0.2f);
        rgbaf.b = _CLAMP_FLOAT(rgbaf.b + amount * weight * 0.2f);
    }
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust shadows (-1.0 to 1.0, affects dark areas)
color_t color_shadows_adjust(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    float lum = color_luminance(color);
    
    // Only affect pixels below 50% luminance
    if (lum < 0.5f) {
        float weight = (0.5f - lum) * 2.0f; // 0 to 1
        rgbaf.r = _CLAMP_FLOAT(rgbaf.r + amount * weight * 0.2f);
        rgbaf.g = _CLAMP_FLOAT(rgbaf.g + amount * weight * 0.2f);
        rgbaf.b = _CLAMP_FLOAT(rgbaf.b + amount * weight * 0.2f);
    }
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust whites (-1.0 to 1.0, affects very bright areas)
color_t color_whites_adjust(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    float lum = color_luminance(color);
    
    // Only affect pixels above 80% luminance
    if (lum > 0.8f) {
        float weight = (lum - 0.8f) * 5.0f; // 0 to 1
        rgbaf.r = _CLAMP_FLOAT(rgbaf.r + amount * weight * 0.3f);
        rgbaf.g = _CLAMP_FLOAT(rgbaf.g + amount * weight * 0.3f);
        rgbaf.b = _CLAMP_FLOAT(rgbaf.b + amount * weight * 0.3f);
    }
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust blacks (-1.0 to 1.0, affects very dark areas)
color_t color_blacks_adjust(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    float lum = color_luminance(color);
    
    // Only affect pixels below 20% luminance
    if (lum < 0.2f) {
        float weight = (0.2f - lum) * 5.0f; // 0 to 1
        rgbaf.r = _CLAMP_FLOAT(rgbaf.r + amount * weight * 0.3f);
        rgbaf.g = _CLAMP_FLOAT(rgbaf.g + amount * weight * 0.3f);
        rgbaf.b = _CLAMP_FLOAT(rgbaf.b + amount * weight * 0.3f);
    }
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust clarity/structure (-1.0 to 1.0, enhances local contrast)
color_t color_clarity_adjust(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    
    // Simplified clarity - boost contrast in mid-tones
    color_rgbaf_t rgbaf = rgba_to_rgbaf(color);
    float lum = color_luminance(color);
    
    // Maximum effect at 50% luminance
    float weight = 1.0f - fabsf(lum - 0.5f) * 2.0f;
    weight = _CLAMP_FLOAT(weight);
    
    float contrast_factor = 1.0f + amount * weight * 0.5f;
    rgbaf.r = _CLAMP_FLOAT((rgbaf.r - 0.5f) * contrast_factor + 0.5f);
    rgbaf.g = _CLAMP_FLOAT((rgbaf.g - 0.5f) * contrast_factor + 0.5f);
    rgbaf.b = _CLAMP_FLOAT((rgbaf.b - 0.5f) * contrast_factor + 0.5f);
    
    return rgbaf_to_rgba(rgbaf);
}

// Adjust vibrance (-1.0 to 1.0, smart saturation that protects skin tones)
color_t color_vibrance_adjust(color_t color, float amount) {
    amount = _CLAMP_FLOAT_RANGE(amount, -1.0f, 1.0f);
    
    color_hsva_t hsva = rgba_to_hsva(color);
    
    // Reduce effect on skin tones (hue around 0.05-0.15, or 18-54 degrees)
    float skin_protection = 1.0f;
    if (hsva.h >= 0.05f && hsva.h <= 0.15f) {
        skin_protection = 0.3f; // Reduce vibrance effect on skin tones
    }
    
    // Reduce effect on already saturated colors
    float saturation_protection = 1.0f - hsva.s * 0.5f;
    
    float final_amount = amount * skin_protection * saturation_protection;
    hsva.s = _CLAMP_FLOAT(hsva.s + final_amount * 0.5f);
    
    return hsva_to_rgba(hsva);
}

// Helper function to blend alpha
static inline uint8_t blend_alpha(uint8_t a1, uint8_t a2) {
    return 255 - (((255 - a1) * (255 - a2)) / 255);
}

// Multiply: a * b / 255
color_t color_multiply(color_t a, color_t b) {
    color_t result;
    result.r = (a.r * b.r) / 255;
    result.g = (a.g * b.g) / 255;
    result.b = (a.b * b.b) / 255;
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Screen: 255 - ((255-a) * (255-b) / 255)
color_t color_screen(color_t a, color_t b) {
    color_t result;
    result.r = 255 - (((255 - a.r) * (255 - b.r)) / 255);
    result.g = 255 - (((255 - a.g) * (255 - b.g)) / 255);
    result.b = 255 - (((255 - a.b) * (255 - b.b)) / 255);
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Overlay: combines multiply and screen
color_t color_overlay(color_t a, color_t b) {
    color_t result;
    
    // For each channel: if base < 128, multiply*2, else screen*2-255
    result.r = (a.r < 128) ? (2 * a.r * b.r) / 255 : 255 - (2 * (255 - a.r) * (255 - b.r)) / 255;
    result.g = (a.g < 128) ? (2 * a.g * b.g) / 255 : 255 - (2 * (255 - a.g) * (255 - b.g)) / 255;
    result.b = (a.b < 128) ? (2 * a.b * b.b) / 255 : 255 - (2 * (255 - a.b) * (255 - b.b)) / 255;
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Soft Light: complex blend mode
color_t color_soft_light(color_t a, color_t b) {
    color_t result;
    
    // Simplified soft light formula
    for (int i = 0; i < 3; i++) {
        uint8_t base = ((uint8_t*)&a)[i];
        uint8_t blend = ((uint8_t*)&b)[i];
        
        if (blend < 128) {
            ((uint8_t*)&result)[i] = (2 * base * blend) / 255 + (base * base * (255 - 2 * blend)) / (255 * 255);
        } else {
            int sqrt_base = (int)(255 * sqrt(base / 255.0));
            ((uint8_t*)&result)[i] = (2 * base * (255 - blend)) / 255 + sqrt_base * (2 * blend - 255) / 255;
        }
    }
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Hard Light: reverse of overlay
color_t color_hard_light(color_t a, color_t b) {
    color_t result;
    
    // For each channel: if blend < 128, multiply*2, else screen*2-255
    result.r = (b.r < 128) ? (2 * a.r * b.r) / 255 : 255 - (2 * (255 - a.r) * (255 - b.r)) / 255;
    result.g = (b.g < 128) ? (2 * a.g * b.g) / 255 : 255 - (2 * (255 - a.g) * (255 - b.g)) / 255;
    result.b = (b.b < 128) ? (2 * a.b * b.b) / 255 : 255 - (2 * (255 - a.b) * (255 - b.b)) / 255;
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Color Dodge: base / (255 - blend)
color_t color_color_dodge(color_t a, color_t b) {
    color_t result;
    
    result.r = (b.r == 255) ? 255 : (a.r * 255) / (255 - b.r);
    result.g = (b.g == 255) ? 255 : (a.g * 255) / (255 - b.g);
    result.b = (b.b == 255) ? 255 : (a.b * 255) / (255 - b.b);
    
    // Clamp to 255
    result.r = (result.r > 255) ? 255 : result.r;
    result.g = (result.g > 255) ? 255 : result.g;
    result.b = (result.b > 255) ? 255 : result.b;
    
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Color Burn: 255 - (255 - base) / blend
color_t color_color_burn(color_t a, color_t b) {
    color_t result;
    
    result.r = (b.r == 0) ? 0 : 255 - ((255 - a.r) * 255) / b.r;
    result.g = (b.g == 0) ? 0 : 255 - ((255 - a.g) * 255) / b.g;
    result.b = (b.b == 0) ? 0 : 255 - ((255 - a.b) * 255) / b.b;
    
    // Clamp to valid range
    result.r = (result.r > 255) ? 255 : (result.r < 0) ? 0 : result.r;
    result.g = (result.g > 255) ? 255 : (result.g < 0) ? 0 : result.g;
    result.b = (result.b > 255) ? 255 : (result.b < 0) ? 0 : result.b;
    
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Darken: min(a, b)
color_t color_darken(color_t a, color_t b) {
    color_t result;
    result.r = (a.r < b.r) ? a.r : b.r;
    result.g = (a.g < b.g) ? a.g : b.g;
    result.b = (a.b < b.b) ? a.b : b.b;
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Lighten: max(a, b)
color_t color_lighten(color_t a, color_t b) {
    color_t result;
    result.r = (a.r > b.r) ? a.r : b.r;
    result.g = (a.g > b.g) ? a.g : b.g;
    result.b = (a.b > b.b) ? a.b : b.b;
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Difference: |a - b|
color_t color_difference(color_t a, color_t b) {
    color_t result;
    result.r = (a.r > b.r) ? a.r - b.r : b.r - a.r;
    result.g = (a.g > b.g) ? a.g - b.g : b.g - a.g;
    result.b = (a.b > b.b) ? a.b - b.b : b.b - a.b;
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Exclusion: a + b - 2*a*b/255
color_t color_exclusion(color_t a, color_t b) {
    color_t result;
    result.r = a.r + b.r - (2 * a.r * b.r) / 255;
    result.g = a.g + b.g - (2 * a.g * b.g) / 255;
    result.b = a.b + b.b - (2 * a.b * b.b) / 255;
    result.a = blend_alpha(a.a, b.a);
    return result;
}

// Alpha blending: standard Porter-Duff "over" operation
color_t color_alpha_blend(color_t fg, color_t bg) {
    color_t result;
    
    // Convert alpha to 0-1 range for calculations
    float fg_alpha = fg.a / 255.0f;
    float bg_alpha = bg.a / 255.0f;
    
    // Calculate output alpha: fg_alpha + bg_alpha * (1 - fg_alpha)
    float out_alpha = fg_alpha + bg_alpha * (1.0f - fg_alpha);
    
    if (out_alpha == 0) {
        // Completely transparent
        result.r = result.g = result.b = result.a = 0;
        return result;
    }
    
    // Alpha blend RGB channels: (fg * fg_alpha + bg * bg_alpha * (1 - fg_alpha)) / out_alpha
    result.r = (uint8_t)((fg.r * fg_alpha + bg.r * bg_alpha * (1.0f - fg_alpha)) / out_alpha);
    result.g = (uint8_t)((fg.g * fg_alpha + bg.g * bg_alpha * (1.0f - fg_alpha)) / out_alpha);
    result.b = (uint8_t)((fg.b * fg_alpha + bg.b * bg_alpha * (1.0f - fg_alpha)) / out_alpha);
    result.a = (uint8_t)(out_alpha * 255);
    
    return result;
}

// Premultiply alpha: multiply RGB channels by alpha
color_t color_premultiply_alpha(color_t color) {
    color_t result;
    
    float alpha = color.a / 255.0f;
    result.r = (uint8_t)(color.r * alpha);
    result.g = (uint8_t)(color.g * alpha);
    result.b = (uint8_t)(color.b * alpha);
    result.a = color.a;
    
    return result;
}

// Unpremultiply alpha: divide RGB channels by alpha
color_t color_unpremultiply_alpha(color_t color) {
    color_t result;
    
    if (color.a == 0) {
        // Completely transparent - RGB values are undefined
        result.r = result.g = result.b = result.a = 0;
        return result;
    }
    
    float alpha = color.a / 255.0f;
    result.r = (uint8_t)(color.r / alpha);
    result.g = (uint8_t)(color.g / alpha);
    result.b = (uint8_t)(color.b / alpha);
    result.a = color.a;
    
    // Clamp to prevent overflow
    result.r = (result.r > 255) ? 255 : result.r;
    result.g = (result.g > 255) ? 255 : result.g;
    result.b = (result.b > 255) ? 255 : result.b;
    
    return result;
}

// Normalize hue to 0-360 range
static float normalize_hue(float hue) {
    while (hue < 0) hue += 360;
    while (hue >= 360) hue -= 360;
    return hue;
}

// Analogous colors: adjacent hues (±30° typically)
void color_analogous(color_t base, color_t* colors, int count) {
    if (count <= 0) return;
    
    color_hsva_t base_hsv = rgba_to_hsva(base);
    float step = 60.0f / (count + 1); // Spread across ±30°
    
    for (int i = 0; i < count; i++) {
        color_hsva_t variant = base_hsv;
        variant.h = normalize_hue(base_hsv.h + (i + 1 - count/2.0f) * step);
        colors[i] = hsva_to_rgba(variant);
    }
}

// Triadic colors: 120° apart
void color_triadic(color_t base, color_t* color1, color_t* color2) {
    if (!color1 && !color2)
        return;

    color_hsva_t base_hsv = rgba_to_hsva(base);
    
    // First triadic color: +120°
    color_hsva_t triadic1 = base_hsv;
    triadic1.h = normalize_hue(base_hsv.h + 120);
    if (color1)
        *color1 = hsva_to_rgba(triadic1);
    
    // Second triadic color: +240°
    color_hsva_t triadic2 = base_hsv;
    triadic2.h = normalize_hue(base_hsv.h + 240);
    if (color2)
        *color2 = hsva_to_rgba(triadic2);
}

// Tetradic colors: rectangle scheme (90° apart)
void color_tetradic(color_t base, color_t* color1, color_t* color2, color_t* color3) {
    if (!color1 && !color2 && !color3)
        return;
    
    color_hsva_t base_hsv = rgba_to_hsva(base);
    // Four colors: +90°, +180°, +270°
    color_hsva_t tetradic = base_hsv;
    tetradic.h = normalize_hue(base_hsv.h + 90);
    if (color1)
        *color1 = hsva_to_rgba(tetradic);
    tetradic.h = normalize_hue(base_hsv.h + 180);
    if (color2)
        *color2 = hsva_to_rgba(tetradic);
    tetradic.h = normalize_hue(base_hsv.h + 270);
    if (color3)
        *color3 = hsva_to_rgba(tetradic);
}

// Split complementary: base complement ±30°
void color_split_complementary(color_t base, color_t* color1, color_t* color2) {
    if (!color1 && !color2)
        return;

    color_hsva_t base_hsv = rgba_to_hsva(base);
    float complement_hue = normalize_hue(base_hsv.h + 180);
    
    // First split complement: complement - 30°
    color_hsva_t split1 = base_hsv;
    split1.h = normalize_hue(complement_hue - 30);
    if (color1)
        *color1 = hsva_to_rgba(split1);
    
    // Second split complement: complement + 30°
    color_hsva_t split2 = base_hsv;
    split2.h = normalize_hue(complement_hue + 30);
    if (color2)
        *color2 = hsva_to_rgba(split2);
}

// Monochromatic colors: same hue, different saturation/value
void color_monochromatic(color_t base, color_t* colors, int count) {
    if (count <= 0) return;
    
    color_hsva_t base_hsv = rgba_to_hsva(base);
    
    for (int i = 0; i < count; i++) {
        color_hsva_t variant = base_hsv;
        float factor = (i + 1.0f) / (count + 1.0f);
        
        // Vary saturation and value
        variant.s = base_hsv.s * (0.3f + 0.7f * factor);
        variant.v = base_hsv.v * (0.4f + 0.6f * factor);
        
        // Clamp values
        variant.s = (variant.s > 1.0f) ? 1.0f : variant.s;
        variant.v = (variant.v > 1.0f) ? 1.0f : variant.v;
        
        colors[i] = hsva_to_rgba(variant);
    }
}

// Generate gradient between two colors
void color_generate_gradient(color_t start, color_t end, color_t* colors, int count) {
    if (count <= 0) return;
    
    for (int i = 0; i < count; i++) {
        float t = (count == 1) ? 0.5f : (float)i / (count - 1);
        
        // Linear interpolation in RGB space
        colors[i].r = (uint8_t)(start.r + t * (end.r - start.r));
        colors[i].g = (uint8_t)(start.g + t * (end.g - start.g));
        colors[i].b = (uint8_t)(start.b + t * (end.b - start.b));
        colors[i].a = (uint8_t)(start.a + t * (end.a - start.a));
    }
}

// K-means clustering for color quantization
void color_generate_palette_from_image(color_t* pixels, int count, color_t* palette, int palette_size) {
    if (count <= 0 || palette_size <= 0) return;
    
    // Initialize palette with first few pixels (or random selection)
    for (int i = 0; i < palette_size && i < count; i++) {
        palette[i] = pixels[i * count / palette_size];
    }
    
    // If we have fewer pixels than palette size, just copy all pixels
    if (count <= palette_size) {
        for (int i = 0; i < count; i++) {
            palette[i] = pixels[i];
        }
        return;
    }
    
    // K-means iterations
    for (int iter = 0; iter < 10; iter++) {
        // Arrays to accumulate color values for each cluster
        int cluster_r[palette_size];
        int cluster_g[palette_size];
        int cluster_b[palette_size];
        int cluster_a[palette_size];
        int cluster_count[palette_size];
        
        // Initialize accumulators
        for (int i = 0; i < palette_size; i++) {
            cluster_r[i] = cluster_g[i] = cluster_b[i] = cluster_a[i] = cluster_count[i] = 0;
        }
        
        // Assign each pixel to nearest palette color
        for (int p = 0; p < count; p++) {
            int closest_idx = 0;
            float min_distance = color_distance(pixels[p], palette[0]);
            
            for (int i = 1; i < palette_size; i++) {
                float distance = color_distance(pixels[p], palette[i]);
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_idx = i;
                }
            }
            
            // Add pixel to cluster
            cluster_r[closest_idx] += pixels[p].r;
            cluster_g[closest_idx] += pixels[p].g;
            cluster_b[closest_idx] += pixels[p].b;
            cluster_a[closest_idx] += pixels[p].a;
            cluster_count[closest_idx]++;
        }
        
        // Update palette colors to cluster centroids
        for (int i = 0; i < palette_size; i++) {
            if (cluster_count[i] > 0) {
                palette[i].r = cluster_r[i] / cluster_count[i];
                palette[i].g = cluster_g[i] / cluster_count[i];
                palette[i].b = cluster_b[i] / cluster_count[i];
                palette[i].a = cluster_a[i] / cluster_count[i];
            }
        }
    }
    
    // Sort palette by brightness (optional, for consistent ordering)
    for (int i = 0; i < palette_size - 1; i++) {
        for (int j = i + 1; j < palette_size; j++) {
            int brightness_i = palette[i].r + palette[i].g + palette[i].b;
            int brightness_j = palette[j].r + palette[j].g + palette[j].b;
            
            if (brightness_i > brightness_j) {
                color_t temp = palette[i];
                palette[i] = palette[j];
                palette[j] = temp;
            }
        }
    }
}

// Gamma correction for sRGB
static float gamma_correct(float value) {
    if (value <= 0.04045f) {
        return value / 12.92f;
    } else {
        return powf((value + 0.055f) / 1.055f, 2.4f);
    }
}

// Inverse gamma correction for sRGB
static float inverse_gamma_correct(float value) {
    if (value <= 0.0031308f) {
        return 12.92f * value;
    } else {
        return 1.055f * powf(value, 1.0f / 2.4f) - 0.055f;
    }
}

// Convert RGBA to XYZ with custom white point
color_xyz_t rgba_to_xyz_custom(color_t rgba, float wx, float wy, float wz) {
    color_xyz_t xyz;
    
    // Convert to 0-1 range and apply gamma correction
    float r = gamma_correct(rgba.r / 255.0f);
    float g = gamma_correct(rgba.g / 255.0f);
    float b = gamma_correct(rgba.b / 255.0f);
    
    // sRGB to XYZ matrix (D65 illuminant)
    // Then scale by custom white point
    float x = (0.4124564f * r + 0.3575761f * g + 0.1804375f * b) * wx;
    float y = (0.2126729f * r + 0.7151522f * g + 0.0721750f * b) * wy;
    float z = (0.0193339f * r + 0.1191920f * g + 0.9503041f * b) * wz;
    
    xyz.x = x;
    xyz.y = y;
    xyz.z = z;
    xyz.a = rgba.a / 255.0f;
    
    return xyz;
}

// XYZ to LAB conversion helper
static float xyz_to_lab_f(float t) {
    const float delta = 6.0f / 29.0f;
    const float delta_cubed = delta * delta * delta;
    
    if (t > delta_cubed) {
        return powf(t, 1.0f / 3.0f);
    } else {
        return t / (3.0f * delta * delta) + 4.0f / 29.0f;
    }
}

// Convert RGBA to LAB with custom white point
color_lab_t rgba_to_lab_custom(color_t rgba, float wx, float wy, float wz) {
    color_lab_t lab;
    
    // First convert to XYZ
    color_xyz_t xyz = rgba_to_xyz_custom(rgba, wx, wy, wz);
    
    // Normalize by white point
    float xn = xyz.x / wx;
    float yn = xyz.y / wy;
    float zn = xyz.z / wz;
    
    // Convert to LAB
    float fx = xyz_to_lab_f(xn);
    float fy = xyz_to_lab_f(yn);
    float fz = xyz_to_lab_f(zn);
    
    lab.l = 116.0f * fy - 16.0f;
    lab.a = 500.0f * (fx - fy);
    lab.b = 200.0f * (fy - fz);
    lab.alpha = rgba.a / 255.0f;
    
    return lab;
}

// Delta E CIE76 (simple Euclidean distance in LAB)
float color_delta_e_76(color_lab_t a, color_lab_t b) {
    float dl = a.l - b.l;
    float da = a.a - b.a;
    float db = a.b - b.b;
    
    return sqrtf(dl*dl + da*da + db*db);
}

// Delta E CIE94 (improved formula with weighting)
float color_delta_e_94(color_lab_t a, color_lab_t b) {
    float dl = a.l - b.l;
    float da = a.a - b.a;
    float db = a.b - b.b;
    
    float c1 = sqrtf(a.a * a.a + a.b * a.b);
    float c2 = sqrtf(b.a * b.a + b.b * b.b);
    float dc = c1 - c2;
    
    float dh_squared = da*da + db*db - dc*dc;
    float dh = (dh_squared < 0) ? 0 : sqrtf(dh_squared);
    
    // Weighting factors for CIE94
    float kl = 1.0f;
    float kc = 1.0f;
    float kh = 1.0f;
    
    float sl = 1.0f;
    float sc = 1.0f + 0.045f * c1;
    float sh = 1.0f + 0.015f * c1;
    
    float dl_term = dl / (kl * sl);
    float dc_term = dc / (kc * sc);
    float dh_term = dh / (kh * sh);
    
    return sqrtf(dl_term*dl_term + dc_term*dc_term + dh_term*dh_term);
}

// Delta E CIEDE2000 (most perceptually accurate)
float color_delta_e_2000(color_lab_t a, color_lab_t b) {
    float l1 = a.l, a1 = a.a, b1 = a.b;
    float l2 = b.l, a2 = b.a, b2 = b.b;
    
    float lbar = (l1 + l2) / 2.0f;
    float c1 = sqrtf(a1*a1 + b1*b1);
    float c2 = sqrtf(a2*a2 + b2*b2);
    float cbar = (c1 + c2) / 2.0f;
    
    float cbar7 = powf(cbar, 7.0f);
    float g = 0.5f * (1.0f - sqrtf(cbar7 / (cbar7 + powf(25.0f, 7.0f))));
    
    float ap1 = (1.0f + g) * a1;
    float ap2 = (1.0f + g) * a2;
    float cp1 = sqrtf(ap1*ap1 + b1*b1);
    float cp2 = sqrtf(ap2*ap2 + b2*b2);
    
    float hp1 = (b1 == 0 && ap1 == 0) ? 0 : atan2f(b1, ap1) * 180.0f / M_PI;
    float hp2 = (b2 == 0 && ap2 == 0) ? 0 : atan2f(b2, ap2) * 180.0f / M_PI;
    
    if (hp1 < 0) hp1 += 360.0f;
    if (hp2 < 0) hp2 += 360.0f;
    
    float dl = l2 - l1;
    float dc = cp2 - cp1;
    float dhp = 0;
    
    if (cp1 * cp2 != 0) {
        dhp = hp2 - hp1;
        if (dhp > 180.0f) dhp -= 360.0f;
        if (dhp < -180.0f) dhp += 360.0f;
    }
    
    float dh = 2.0f * sqrtf(cp1 * cp2) * sinf(dhp * M_PI / 360.0f);
    
    float cpbar = (cp1 + cp2) / 2.0f;
    float hpbar = 0;
    
    if (cp1 * cp2 != 0) {
        hpbar = (hp1 + hp2) / 2.0f;
        if (fabsf(hp1 - hp2) > 180.0f) {
            if (hpbar < 180.0f) hpbar += 180.0f;
            else hpbar -= 180.0f;
        }
    }
    
    float t = 1.0f - 0.17f * cosf((hpbar - 30.0f) * M_PI / 180.0f) +
              0.24f * cosf(2.0f * hpbar * M_PI / 180.0f) +
              0.32f * cosf((3.0f * hpbar + 6.0f) * M_PI / 180.0f) -
              0.20f * cosf((4.0f * hpbar - 63.0f) * M_PI / 180.0f);
    
    float dt = 30.0f * expf(-powf((hpbar - 275.0f) / 25.0f, 2.0f));
    float cpbar7 = powf(cpbar, 7.0f);
    float rc = 2.0f * sqrtf(cpbar7 / (cpbar7 + powf(25.0f, 7.0f)));
    
    float sl = 1.0f + (0.015f * powf(lbar - 50.0f, 2.0f)) / sqrtf(20.0f + powf(lbar - 50.0f, 2.0f));
    float sc = 1.0f + 0.045f * cpbar;
    float sh = 1.0f + 0.015f * cpbar * t;
    float rt = -sinf(2.0f * dt * M_PI / 180.0f) * rc;
    
    float kl = 1.0f, kc = 1.0f, kh = 1.0f;
    
    float dl_term = dl / (kl * sl);
    float dc_term = dc / (kc * sc);
    float dh_term = dh / (kh * sh);
    
    return sqrtf(dl_term*dl_term + dc_term*dc_term + dh_term*dh_term + rt * dc_term * dh_term);
}

// Levels adjustment: black point, white point, gamma
color_t color_levels(color_t color, float black_point, float white_point, float gamma) {
    color_t result;
    
    // Normalize inputs to 0-1 range
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    
    // Apply levels adjustment to each channel
    r = _CLAMP_FLOAT((r - black_point) / (white_point - black_point));
    g = _CLAMP_FLOAT((g - black_point) / (white_point - black_point));
    b = _CLAMP_FLOAT((b - black_point) / (white_point - black_point));
    
    // Apply gamma correction
    r = powf(r, 1.0f / gamma);
    g = powf(g, 1.0f / gamma);
    b = powf(b, 1.0f / gamma);
    
    result.r = _CLAMP_UINT8(r * 255.0f);
    result.g = _CLAMP_UINT8(g * 255.0f);
    result.b = _CLAMP_UINT8(b * 255.0f);
    result.a = color.a;
    
    return result;
}

// Curves adjustment using lookup tables
color_t color_curves(color_t color, float* curve_r, float* curve_g, float* curve_b) {
    color_t result;
    
    // Apply curves (assuming curves are 256-element arrays)
    result.r = _CLAMP_UINT8(curve_r[color.r] * 255.0f);
    result.g = _CLAMP_UINT8(curve_g[color.g] * 255.0f);
    result.b = _CLAMP_UINT8(curve_b[color.b] * 255.0f);
    result.a = color.a;
    
    return result;
}

// Shadow/Highlight adjustment
color_t color_shadow_highlight(color_t color, float shadow, float highlight) {
    color_t result;
    
    // Convert to 0-1 range
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    
    // Calculate luminance
    float lum = 0.299f * r + 0.587f * g + 0.114f * b;
    
    // Apply shadow/highlight adjustments
    float shadow_factor = 1.0f + shadow * (1.0f - lum);
    float highlight_factor = 1.0f + highlight * lum;
    
    r *= shadow_factor * highlight_factor;
    g *= shadow_factor * highlight_factor;
    b *= shadow_factor * highlight_factor;
    
    result.r = _CLAMP_UINT8(r * 255.0f);
    result.g = _CLAMP_UINT8(g * 255.0f);
    result.b = _CLAMP_UINT8(b * 255.0f);
    result.a = color.a;
    
    return result;
}

// Color balance adjustment
color_t color_color_balance(color_t color, float cyan_red, float magenta_green, float yellow_blue) {
    color_t result;
    
    // Convert to 0-1 range
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    
    // Apply color balance adjustments
    r += cyan_red;
    g += magenta_green;
    b += yellow_blue;
    
    result.r = _CLAMP_UINT8(r * 255.0f);
    result.g = _CLAMP_UINT8(g * 255.0f);
    result.b = _CLAMP_UINT8(b * 255.0f);
    result.a = color.a;
    
    return result;
}

// Vibrance adjustment (smart saturation)
color_t color_vibrance(color_t color, float amount) {
    color_t result;
    
    // Convert to HSV for saturation manipulation
    color_hsva_t hsv = rgba_to_hsva(color);
    
    // Calculate existing saturation level
    float sat_factor = 1.0f - hsv.s;
    
    // Apply vibrance (more effect on less saturated colors)
    hsv.s += amount * sat_factor;
    hsv.s = _CLAMP_FLOAT(hsv.s);
    
    result = hsva_to_rgba(hsv);
    result.a = color.a;
    
    return result;
}

// Selective color adjustment
color_t color_selective_color(color_t color, int channel, float cyan, float magenta, float yellow, float black) {
    color_t result = color;
    
    // Convert to CMY for selective color work
    float c = 1.0f - color.r / 255.0f;
    float m = 1.0f - color.g / 255.0f;
    float y = 1.0f - color.b / 255.0f;
    
    // Determine which color channel to adjust based on dominant color
    float max_cmy = (c > m) ? ((c > y) ? c : y) : ((m > y) ? m : y);
    
    // Apply selective adjustments based on channel
    switch (channel) {
        case 0: // Reds (low cyan)
            if (c < 0.5f) {
                c += cyan * (1.0f - c);
                m += magenta * (1.0f - m);
                y += yellow * (1.0f - y);
            }
            break;
        case 1: // Yellows (high yellow)
            if (y > 0.5f) {
                c += cyan * (1.0f - c);
                m += magenta * (1.0f - m);
                y += yellow * (1.0f - y);
            }
            break;
        case 2: // Greens (high yellow, low magenta)
            if (y > 0.3f && m < 0.5f) {
                c += cyan * (1.0f - c);
                m += magenta * (1.0f - m);
                y += yellow * (1.0f - y);
            }
            break;
        case 3: // Cyans (high cyan)
            if (c > 0.5f) {
                c += cyan * (1.0f - c);
                m += magenta * (1.0f - m);
                y += yellow * (1.0f - y);
            }
            break;
        case 4: // Blues (high cyan, high magenta)
            if (c > 0.3f && m > 0.3f) {
                c += cyan * (1.0f - c);
                m += magenta * (1.0f - m);
                y += yellow * (1.0f - y);
            }
            break;
        case 5: // Magentas (high magenta)
            if (m > 0.5f) {
                c += cyan * (1.0f - c);
                m += magenta * (1.0f - m);
                y += yellow * (1.0f - y);
            }
            break;
    }
    
    // Apply black adjustment
    float k = black;
    c = _CLAMP_FLOAT(c + k);
    m = _CLAMP_FLOAT(m + k);
    y = _CLAMP_FLOAT(y + k);
    
    // Convert back to RGB
    result.r = _CLAMP_UINT8((1.0f - c) * 255.0f);
    result.g = _CLAMP_UINT8((1.0f - m) * 255.0f);
    result.b = _CLAMP_UINT8((1.0f - y) * 255.0f);
    
    return result;
}

// Quantize color to fewer bits per channel
color_t color_quantize(color_t color, int bits_per_channel) {
    color_t result;
    
    int levels = (1 << bits_per_channel) - 1;
    float scale = 255.0f / levels;
    
    result.r = (uint8_t)((color.r / (int)scale) * scale);
    result.g = (uint8_t)((color.g / (int)scale) * scale);
    result.b = (uint8_t)((color.b / (int)scale) * scale);
    result.a = color.a;
    
    return result;
}

// Posterize to specific number of levels
color_t color_posterize(color_t color, int levels) {
    color_t result;
    
    float scale = 255.0f / (levels - 1);
    
    result.r = (uint8_t)((int)(color.r / scale) * scale);
    result.g = (uint8_t)((int)(color.g / scale) * scale);
    result.b = (uint8_t)((int)(color.b / scale) * scale);
    result.a = color.a;
    
    return result;
}

// Reduce palette using median cut algorithm
void color_reduce_palette(color_t* colors, int count, color_t* palette, int palette_size) {
    // Simple implementation - use existing palette generation
    color_generate_palette_from_image(colors, count, palette, palette_size);
}

// Find closest color in palette
static int find_closest_color(color_t color, color_t* palette, int palette_size) {
    int closest = 0;
    float min_distance = color_distance(color, palette[0]);
    
    for (int i = 1; i < palette_size; i++) {
        float distance = color_distance(color, palette[i]);
        if (distance < min_distance) {
            min_distance = distance;
            closest = i;
        }
    }
    
    return closest;
}

// Floyd-Steinberg dithering
color_t color_dither_floyd_steinberg(color_t color, color_t* palette, int palette_size, int x, int y) {
    // Find closest palette color
    int closest_idx = find_closest_color(color, palette, palette_size);
    color_t closest = palette[closest_idx];
    
    // Calculate error (this would normally be distributed to neighboring pixels)
    // For a single pixel function, we just return the closest color
    return closest;
}

// Ordered dithering using Bayer matrix
color_t color_dither_ordered(color_t color, color_t* palette, int palette_size, int x, int y) {
    // 4x4 Bayer matrix
    static const int bayer_matrix[4][4] = {
        { 0,  8,  2, 10},
        {12,  4, 14,  6},
        { 3, 11,  1,  9},
        {15,  7, 13,  5}
    };
    
    // Get threshold from Bayer matrix
    int threshold = bayer_matrix[y % 4][x % 4];
    float dither_value = (threshold / 15.0f - 0.5f) * 32.0f; // Scale dither amount
    
    // Apply dither to color
    color_t dithered;
    dithered.r = _CLAMP_UINT8(color.r + dither_value);
    dithered.g = _CLAMP_UINT8(color.g + dither_value);
    dithered.b = _CLAMP_UINT8(color.b + dither_value);
    dithered.a = color.a;
    
    // Find closest palette color
    int closest_idx = find_closest_color(dithered, palette, palette_size);
    return palette[closest_idx];
}

// Protanopia (red-blind) - affects L-cones
// Uses Brettel, Viénot and Mollon transformation matrix
color_t color_protanopia(color_t color) {
    // Transformation matrix for protanopia
    // Row 1: [0.567, 0.433, 0.000]
    // Row 2: [0.558, 0.442, 0.000] 
    // Row 3: [0.000, 0.242, 0.758]
    
    int r = (567 * color.r + 433 * color.g) / 1000;
    int g = (558 * color.r + 442 * color.g) / 1000;
    int b = (242 * color.g + 758 * color.b) / 1000;
    
    color_t result;
    result.r = _CLAMP_UINT8(r);
    result.g = _CLAMP_UINT8(g);
    result.b = _CLAMP_UINT8(b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Deuteranopia (green-blind) - affects M-cones
// Uses Brettel, Viénot and Mollon transformation matrix
color_t color_deuteranopia(color_t color) {
    // Transformation matrix for deuteranopia
    // Row 1: [0.625, 0.375, 0.000]
    // Row 2: [0.700, 0.300, 0.000]
    // Row 3: [0.000, 0.300, 0.700]
    
    int r = (625 * color.r + 375 * color.g) / 1000;
    int g = (700 * color.r + 300 * color.g) / 1000;
    int b = (300 * color.g + 700 * color.b) / 1000;
    
    color_t result;
    result.r = _CLAMP_UINT8(r);
    result.g = _CLAMP_UINT8(g);
    result.b = _CLAMP_UINT8(b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Tritanopia (blue-blind) - affects S-cones
// Uses Brettel, Viénot and Mollon transformation matrix
color_t color_tritanopia(color_t color) {
    // Transformation matrix for tritanopia
    // Row 1: [0.950, 0.050, 0.000]
    // Row 2: [0.000, 0.433, 0.567]
    // Row 3: [0.000, 0.475, 0.525]
    
    int r = (950 * color.r + 50 * color.g) / 1000;
    int g = (433 * color.g + 567 * color.b) / 1000;
    int b = (475 * color.g + 525 * color.b) / 1000;
    
    color_t result;
    result.r = _CLAMP_UINT8(r);
    result.g = _CLAMP_UINT8(g);
    result.b = _CLAMP_UINT8(b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Achromatopsia (complete color blindness) - monochromacy
// Converts to grayscale using luminance formula
color_t color_achromatopsia(color_t color) {
    // Standard luminance formula: 0.299*R + 0.587*G + 0.114*B
    // Using integer arithmetic: (299*R + 587*G + 114*B) / 1000
    
    int luminance = (299 * color.r + 587 * color.g + 114 * color.b) / 1000;
    uint8_t gray = _CLAMP_UINT8(luminance);
    
    color_t result;
    result.r = gray;
    result.g = gray;
    result.b = gray;
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Helper function to calculate relative luminance according to WCAG
// Uses the sRGB color space formula
static float relative_luminance(color_t color) {
    // Convert RGB values to 0-1 range
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    
    // Apply gamma correction (sRGB to linear RGB)
    r = (r <= 0.03928f) ? r / 12.92f : powf((r + 0.055f) / 1.055f, 2.4f);
    g = (g <= 0.03928f) ? g / 12.92f : powf((g + 0.055f) / 1.055f, 2.4f);
    b = (b <= 0.03928f) ? b / 12.92f : powf((b + 0.055f) / 1.055f, 2.4f);
    
    // Calculate relative luminance using ITU-R BT.709 coefficients
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// Calculate WCAG contrast ratio between two colors
float color_contrast_ratio(color_t a, color_t b) {
    float lum_a = relative_luminance(a);
    float lum_b = relative_luminance(b);
    
    // Ensure lighter color is in numerator
    float lighter = (lum_a > lum_b) ? lum_a : lum_b;
    float darker = (lum_a > lum_b) ? lum_b : lum_a;
    
    // WCAG contrast ratio formula: (L1 + 0.05) / (L2 + 0.05)
    return (lighter + 0.05f) / (darker + 0.05f);
}

// Check if color combination meets WCAG AA compliance
// AA requires contrast ratio of at least 4.5:1 for normal text, 3:1 for large text
int color_wcag_aa_compliant(color_t fg, color_t bg) {
    float ratio = color_contrast_ratio(fg, bg);
    return ratio >= 4.5f;
}

// Check if color combination meets WCAG AAA compliance  
// AAA requires contrast ratio of at least 7:1 for normal text, 4.5:1 for large text
int color_wcag_aaa_compliant(color_t fg, color_t bg) {
    float ratio = color_contrast_ratio(fg, bg);
    return ratio >= 7.0f;
}

// Find the closest color in a palette to a target color
color_t color_match_closest(color_t target, color_t* palette, int palette_size) {
    if (palette_size <= 0 || palette == NULL) {
        return target;  // Return target if no palette provided
    }
    
    color_t closest = palette[0];
    float min_distance = color_distance(target, palette[0]);
    
    for (int i = 1; i < palette_size; i++) {
        float distance = color_distance(target, palette[i]);
        if (distance < min_distance) {
            min_distance = distance;
            closest = palette[i];
        }
    }
    
    return closest;
}

// Find the most frequently occurring color in an array of pixels
// Returns index of the dominant color, or -1 if count is 0
int color_find_dominant(color_t* pixels, int count) {
    if (count <= 0 || pixels == NULL) {
        return -1;
    }
    
    int max_count = 0;
    int dominant_index = 0;
    
    // Simple O(n²) approach - for better performance with large arrays,
    // consider using a hash table or sorting approach
    for (int i = 0; i < count; i++) {
        int current_count = 0;
        
        // Count occurrences of pixels[i]
        for (int j = 0; j < count; j++) {
            if (pixels[i].rgba == pixels[j].rgba) {
                current_count++;
            }
        }
        
        if (current_count > max_count) {
            max_count = current_count;
            dominant_index = i;
        }
    }
    
    return dominant_index;
}

// Generate a color histogram from an array of pixels
// histogram array should be pre-allocated with size for all possible colors
// For practical use, this creates a histogram of unique colors found
void color_histogram(color_t* pixels, int count, int* histogram) {
    if (count <= 0 || pixels == NULL || histogram == NULL) {
        return;
    }
    
    // First, find all unique colors and their indices
    color_t unique_colors[count];  // Maximum possible unique colors
    int unique_count = 0;
    
    // Initialize histogram
    for (int i = 0; i < count; i++) {
        histogram[i] = 0;
    }
    
    // Build histogram of unique colors
    for (int i = 0; i < count; i++) {
        // Check if this color is already in our unique list
        int found_index = -1;
        for (int j = 0; j < unique_count; j++) {
            if (unique_colors[j].rgba == pixels[i].rgba) {
                found_index = j;
                break;
            }
        }
        
        if (found_index == -1) {
            // New unique color
            unique_colors[unique_count] = pixels[i];
            histogram[unique_count] = 1;
            unique_count++;
        } else {
            // Increment count for existing color
            histogram[found_index]++;
        }
    }
    
    // Zero out unused histogram entries
    for (int i = unique_count; i < count; i++) {
        histogram[i] = 0;
    }
}

// Calculate color similarity as a normalized value (0.0 = identical, 1.0 = maximum difference)
float color_similarity(color_t a, color_t b) {
    // Use Euclidean distance in RGB space, normalized to 0-1 range
    float dr = (a.r - b.r) / 255.0f;
    float dg = (a.g - b.g) / 255.0f;
    float db = (a.b - b.b) / 255.0f;
    
    // Calculate distance and normalize by maximum possible distance (sqrt(3))
    float distance = sqrtf(dr * dr + dg * dg + db * db);
    return distance / sqrtf(3.0f);
}

// Check if two colors are similar within a given threshold
// threshold: 0.0 (identical) to 1.0 (maximum difference)
int color_is_similar(color_t a, color_t b, float threshold) {
    return color_similarity(a, b) <= threshold;
}

#define _C2F(V) ((float)(V) / 255.0f)
#define _F2C(V) ((uint8_t)((V) * 255.0f + 0.5f))

// Lift-Gamma-Gain color grading (common in film/video post-production)
// lift: affects shadows/blacks, gamma: affects midtones, gain: affects highlights/whites
color_t color_lift_gamma_gain(color_t color, color_rgbaf_t lift, color_rgbaf_t gamma, color_rgbaf_t gain) {
    // Convert to 0-1 range
    float r = _C2F(color.r);
    float g = _C2F(color.g);
    float b = _C2F(color.b);
    
    // Apply lift (additive in shadows)
    r = r + lift.r * (1.0f - r);
    g = g + lift.g * (1.0f - g);
    b = b + lift.b * (1.0f - b);
    
    // Apply gamma (power function for midtones)
    if (gamma.r != 0.0f) r = powf(r, 1.0f / gamma.r);
    if (gamma.g != 0.0f) g = powf(g, 1.0f / gamma.g);
    if (gamma.b != 0.0f) b = powf(b, 1.0f / gamma.b);
    
    // Apply gain (multiplicative in highlights)
    r *= gain.r;
    g *= gain.g;
    b *= gain.b;
    
    color_t result;
    result.r = _F2C(r);
    result.g = _F2C(g);
    result.b = _F2C(b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Shadow-Midtone-Highlight color grading (alternative approach)
// Uses luminance-based weighting to apply corrections
color_t color_shadow_midtone_highlight(color_t color, color_rgbaf_t shadow, color_rgbaf_t midtone, color_rgbaf_t highlight) {
    // Convert to 0-1 range
    float r = _C2F(color.r);
    float g = _C2F(color.g);
    float b = _C2F(color.b);
    
    // Calculate luminance for weighting
    float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
    
    // Create weighting functions for shadow, midtone, and highlight
    // Shadow weight: stronger in dark areas
    float shadow_weight = 1.0f - powf(luminance, 2.0f);
    
    // Midtone weight: bell curve centered at 0.5
    float midtone_weight = 4.0f * luminance * (1.0f - luminance);
    
    // Highlight weight: stronger in bright areas
    float highlight_weight = powf(luminance, 2.0f);
    
    // Apply weighted color corrections
    r += shadow.r * shadow_weight + midtone.r * midtone_weight + highlight.r * highlight_weight;
    g += shadow.g * shadow_weight + midtone.g * midtone_weight + highlight.g * highlight_weight;
    b += shadow.b * shadow_weight + midtone.b * midtone_weight + highlight.b * highlight_weight;
    
    color_t result;
    result.r = _F2C(r);
    result.g = _F2C(g);
    result.b = _F2C(b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Helper function to apply 3x3 color space transformation matrix
static color_t apply_color_matrix(color_t color, const float matrix[3][3]) {
    // Convert to 0-1 range
    float r = _C2F(color.r);
    float g = _C2F(color.g);
    float b = _C2F(color.b);
    
    // Apply gamma correction (sRGB to linear) before color space conversion
    r = (r <= 0.04045f) ? r / 12.92f : powf((r + 0.055f) / 1.055f, 2.4f);
    g = (g <= 0.04045f) ? g / 12.92f : powf((g + 0.055f) / 1.055f, 2.4f);
    b = (b <= 0.04045f) ? b / 12.92f : powf((b + 0.055f) / 1.055f, 2.4f);
    
    // Apply matrix transformation
    float new_r = matrix[0][0] * r + matrix[0][1] * g + matrix[0][2] * b;
    float new_g = matrix[1][0] * r + matrix[1][1] * g + matrix[1][2] * b;
    float new_b = matrix[2][0] * r + matrix[2][1] * g + matrix[2][2] * b;
    
    // Apply inverse gamma correction (linear to sRGB)
    new_r = (new_r <= 0.0031308f) ? new_r * 12.92f : 1.055f * powf(new_r, 1.0f / 2.4f) - 0.055f;
    new_g = (new_g <= 0.0031308f) ? new_g * 12.92f : 1.055f * powf(new_g, 1.0f / 2.4f) - 0.055f;
    new_b = (new_b <= 0.0031308f) ? new_b * 12.92f : 1.055f * powf(new_b, 1.0f / 2.4f) - 0.055f;
    
    color_t result;
    result.r = _F2C(new_r);
    result.g = _F2C(new_g);
    result.b = _F2C(new_b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Convert Rec.709 (standard HD) to Rec.2020 (ultra HD/HDR)
// Used in broadcast and streaming for HDR content
color_t color_rec709_to_rec2020(color_t color) {
    // Transformation matrix from Rec.709 to Rec.2020
    // Based on ITU-R BT.2087 standard
    static const float matrix[3][3] = {
        {0.627404f, 0.329283f, 0.043313f},
        {0.069097f, 0.919540f, 0.011362f},
        {0.016391f, 0.088013f, 0.895595f}
    };
    
    return apply_color_matrix(color, matrix);
}

// Convert ProPhoto RGB to sRGB
// ProPhoto RGB has a much wider gamut, commonly used in professional photography
color_t color_prophoto_to_srgb(color_t color) {
    // Convert to 0-1 range
    float r = _C2F(color.r);
    float g = _C2F(color.g);
    float b = _C2F(color.b);
    
    // Apply ProPhoto RGB gamma correction (gamma 1.8) to linear
    r = powf(r, 1.8f);
    g = powf(g, 1.8f);
    b = powf(b, 1.8f);
    
    // ProPhoto RGB to XYZ transformation matrix
    float x = 0.7976749f * r + 0.1351917f * g + 0.0313534f * b;
    float y = 0.2880402f * r + 0.7118741f * g + 0.0000857f * b;
    float z = 0.0000000f * r + 0.0000000f * g + 0.8252100f * b;
    
    // XYZ to sRGB transformation matrix
    float new_r = 3.2404542f * x - 1.5371385f * y - 0.4985314f * z;
    float new_g = -0.9692660f * x + 1.8760108f * y + 0.0415560f * z;
    float new_b = 0.0556434f * x - 0.2040259f * y + 1.0572252f * z;
    
    // Apply sRGB gamma correction (linear to sRGB)
    new_r = (new_r <= 0.0031308f) ? new_r * 12.92f : 1.055f * powf(new_r, 1.0f / 2.4f) - 0.055f;
    new_g = (new_g <= 0.0031308f) ? new_g * 12.92f : 1.055f * powf(new_g, 1.0f / 2.4f) - 0.055f;
    new_b = (new_b <= 0.0031308f) ? new_b * 12.92f : 1.055f * powf(new_b, 1.0f / 2.4f) - 0.055f;
    
    color_t result;
    result.r = _F2C(new_r);
    result.g = _F2C(new_g);
    result.b = _F2C(new_b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Convert Adobe RGB to sRGB
// Adobe RGB has wider gamut than sRGB, commonly used in print and professional workflows
color_t color_adobe_rgb_to_srgb(color_t color) {
    // Convert to 0-1 range
    float r = _C2F(color.r);
    float g = _C2F(color.g);
    float b = _C2F(color.b);
    
    // Apply Adobe RGB gamma correction (gamma 2.2) to linear
    r = powf(r, 2.2f);
    g = powf(g, 2.2f);
    b = powf(b, 2.2f);
    
    // Adobe RGB to XYZ transformation matrix
    float x = 0.5767309f * r + 0.1855540f * g + 0.1881852f * b;
    float y = 0.2973769f * r + 0.6273491f * g + 0.0752741f * b;
    float z = 0.0270343f * r + 0.0706872f * g + 0.9911085f * b;
    
    // XYZ to sRGB transformation matrix
    float new_r = 3.2404542f * x - 1.5371385f * y - 0.4985314f * z;
    float new_g = -0.9692660f * x + 1.8760108f * y + 0.0415560f * z;
    float new_b = 0.0556434f * x - 0.2040259f * y + 1.0572252f * z;
    
    // Apply sRGB gamma correction (linear to sRGB)
    new_r = (new_r <= 0.0031308f) ? new_r * 12.92f : 1.055f * powf(new_r, 1.0f / 2.4f) - 0.055f;
    new_g = (new_g <= 0.0031308f) ? new_g * 12.92f : 1.055f * powf(new_g, 1.0f / 2.4f) - 0.055f;
    new_b = (new_b <= 0.0031308f) ? new_b * 12.92f : 1.055f * powf(new_b, 1.0f / 2.4f) - 0.055f;
    
    color_t result;
    result.r = _F2C(new_r);
    result.g = _F2C(new_g);
    result.b = _F2C(new_b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Fast grayscale conversion using integer arithmetic only
// Uses approximate luminance weights for speed
color_t color_fast_grayscale(color_t color) {
    // Fast integer approximation: (R + G + B) / 3
    // Or slightly better: (R*77 + G*151 + B*28) >> 8 (approximates 0.299, 0.587, 0.114)
    uint8_t gray = (uint8_t)((77 * color.r + 151 * color.g + 28 * color.b) >> 8);
    
    color_t result;
    result.r = gray;
    result.g = gray;
    result.b = gray;
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Fast sepia tone effect using integer arithmetic
// Creates warm brown tones typical of old photographs
color_t color_fast_sepia(color_t color) {
    // Calculate sepia values using integer arithmetic
    // Standard sepia transformation with bit shifting for speed
    int r = (color.r * 393 + color.g * 769 + color.b * 189) >> 10;  // / 1024
    int g = (color.r * 349 + color.g * 686 + color.b * 168) >> 10;  // / 1024  
    int b = (color.r * 272 + color.g * 534 + color.b * 131) >> 10;  // / 1024
    
    color_t result;
    result.r = _CLAMP_UINT8(r);
    result.g = _CLAMP_UINT8(g);
    result.b = _CLAMP_UINT8(b);
    result.a = color.a;  // Preserve alpha
    
    return result;
}

// Fast color inversion using bitwise operations
// Simply inverts each color component
color_t color_fast_invert(color_t color) {
    color_t result;
    result.r = 255 - color.r;  // Equivalent to: color.r ^ 0xFF
    result.g = 255 - color.g;  // Equivalent to: color.g ^ 0xFF
    result.b = 255 - color.b;  // Equivalent to: color.b ^ 0xFF
    result.a = color.a;        // Preserve alpha
    
    return result;
}

// Batch brightness adjustment for multiple colors
// brightness: 0.0 = black, 1.0 = original, 2.0 = double brightness
void color_batch_adjust_brightness(color_t* colors, int count, float brightness) {
    if (colors == NULL || count <= 0) {
        return;
    }
    
    // Convert brightness to integer for faster processing
    int brightness_int = (int)(brightness * 256.0f);  // 8.8 fixed point
    
    for (int i = 0; i < count; i++) {
        // Apply brightness using integer arithmetic
        int r = (colors[i].r * brightness_int) >> 8;
        int g = (colors[i].g * brightness_int) >> 8;
        int b = (colors[i].b * brightness_int) >> 8;
        
        colors[i].r = _CLAMP_UINT8(r);
        colors[i].g = _CLAMP_UINT8(g);
        colors[i].b = _CLAMP_UINT8(b);
        // Alpha unchanged
    }
}

// Batch apply look-up table (LUT) transformation
// lut should be a 256-entry array for each color channel
// This version assumes a simple 1D LUT for all channels
void color_batch_apply_lut(color_t* colors, int count, color_t* lut) {
    if (colors == NULL || count <= 0 || lut == NULL) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        // Use original color values as indices into the LUT
        // Assumes LUT has 256 entries (0-255)
        uint8_t r_index = colors[i].r;
        uint8_t g_index = colors[i].g;
        uint8_t b_index = colors[i].b;
        
        // Apply LUT transformation
        // This is a simplified version - real LUTs often use interpolation
        colors[i].r = lut[r_index].r;
        colors[i].g = lut[g_index].g;
        colors[i].b = lut[b_index].b;
        // Alpha unchanged
    }
}

static color_t* image_make(unsigned int w, unsigned int h) {
    static_assert(sizeof(color_t) == sizeof(uint32_t), "color_t must be 4 bytes");
    uint32_t *buffer = malloc((w * h + 2) * sizeof(uint32_t));
    if (!buffer)
        return NULL;
    buffer[0] = w;
    buffer[1] = h;
    return (color_t*)(buffer + 2);
}

image_t image_empty(unsigned int w, unsigned int h, color_t color) {
    if (w <= 0 || h <= 0)
        return NULL;
    image_t result = image_make(w, h);
    if (!result)
        return NULL;
    image_fill(result, _black);
    return result;
}

image_t image_load_from_path(const char *path) {
    image_t result = NULL;
    unsigned char *data = NULL;
    if (access(path, F_OK))
        return NULL;
    size_t sz = -1;
    FILE *fh = fopen(path, "rb");
    if (!fh)
        return NULL;
    fseek(fh, 0, SEEK_END);
    if (!(sz = ftell(fh)))
        goto BAIL;
    fseek(fh, 0, SEEK_SET);
    if (!(data = malloc(sz * sizeof(unsigned char))))
        goto BAIL;
    if (fread(data, sz, 1, fh) != 1)
        goto BAIL;
    result = image_load_from_memory(data, (int)sz);
BAIL:
    if (fh)
        fclose(fh);
    if (data)
        free(data);
    return result;
}

static int check_if_qoi(unsigned char *data) {
    return _RGBA(data[0], data[0], data[0], data[0]) ==  _RGBA('q', 'o', 'i', 'f');
}

image_t image_load_from_memory(const void *data, size_t data_size) {
    if (!data || data_size <= 0)
        return NULL;
    int _w, _h, c = 0;
    unsigned char *img_data = NULL;
    if (check_if_qoi((unsigned char*)data)) {
        qoi_desc desc;
        if (!(img_data = qoi_decode(data, (int)data_size, &desc, 4)))
            return NULL;
        _w = desc.width;
        _h = desc.height;
    } else
        if (!(img_data = stbi_load_from_memory(data, (int)data_size, &_w, &_h, &c, 4)))
            return NULL;
    if (_w <= 0 || _h <= 0 || c < 3) {
        free(img_data);
        return NULL;
    }

    image_t result = image_make(_w, _h);
    if (!result) {
        free(img_data);
        return NULL;
    }
    for (int x = 0; x < _w; x++)
        for (int y = 0; y < _h; y++) {
            unsigned char *p = img_data + (x + _w * y) * 4;
            result[y * _w + x] = (color_t) {
                .r = p[0],
                .g = p[1],
                .b = p[2],
                .a = p[3]
            };
        }
    free(img_data);
    return result;
}

bool image_export(const char *path, const image_t img) {
    if (!img || !path)
        return false;
    int w, h;
    image_size(img, &w, &h);
    char *buffer = malloc(w * h * 4);
    if (!buffer)
        return false;
    bool result = true;
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            color_t color = img[y * w + x];
            char *p = buffer + (x + w * y) * 4;
            p[0] = color.r;
            p[1] = color.g;
            p[2] = color.b;
            p[3] = color.a;
        }
    
    const char *_ext = strrchr(path, '.');
    if (!_ext || _ext[0] == '\0') {
        result = false;
        goto BAIL;
    }
    char *ext = strdup(_ext);
    for (char *p = ext; *p; p++)
        *p = tolower(*p);
    if (strncmp(".qoi", ext, 4) == 0) {
        result = false;
        qoi_desc desc = {
            .width = w,
            .height = h,
            .channels = 4,
            .colorspace = QOI_SRGB
        };
        int size = 0;
        void *data = qoi_encode(buffer, &desc, &size);
        if (!data || !size)
            goto BAIL;
        FILE *fh = fopen(path, "wb");
        if (!fh)
            goto BAIL;
        size_t written = fwrite(data, size, 1, fh);
        result = written != 1;
        if (fh)
            fclose(fh);
        if (data)
            free(data);
        goto BAIL;
    } else if (strncmp(".png", ext, 4) == 0) {
        if (stbi_write_png(path, w, h, 4, buffer, w * 4))
            goto BAIL;
    } else if (strncmp(".jpg", ext, 4) == 0 || strncmp(".jpeg", ext, 5) == 0) {
        if (stbi_write_jpg(path, w, h, 4, buffer, 100))
            goto BAIL;
    } else if (strncmp(".bmp", ext, 4) == 0) {
        if (stbi_write_bmp(path, w, h, 4, buffer))
            goto BAIL;
    } else if (strncmp(".tga", ext, 4) == 0) {
        if (stbi_write_tga(path, w, h, 4, buffer))
            goto BAIL;
    } else if (strncmp(".hdr", ext, 4) == 0) {
        if (stbi_write_hdr(path, w, h, 4 , (float*)buffer))
            goto BAIL;
    }
    result = false;
BAIL:
    if (ext)
        free(ext);
    if (buffer)
        free(buffer);
    return result;
}

static uint32_t* _raw(image_t img) {
    return img ? NULL : (uint32_t *)(img)-2;
}

void image_destroy(image_t img) {
    uint32_t *raw = _raw(img);
    if (raw)
        free(raw);
}

int image_width(const image_t img) {
    uint32_t *raw = _raw(img);
    return raw ? raw[0] : 0;
}

int image_height(const image_t img) {
    uint32_t *raw = _raw(img);
    return raw ? raw[1] : 0;
}

bool image_size(const image_t img, int *w, int *h) {
    if (!img || (!w && !h))
        return false;
    uint32_t *raw = _raw(img);
    int _w = raw ? raw[0] : 0;
    int _h = raw ? raw[1] : 0;
    if (w)
        *w = _w;
    if (h)
        *h = _h;
    return true;
}

bool image_pset(image_t img, int x, int y, color_t color) {
    if (!img || x < 0 || y < 0)
        return false;
    int w, h;
    image_size(img, &w, &h);
    if (x >= w || y >= h || w <= 0 || h <= 0)
        return false;
    img[y * w + x] = color;
    return true;
}

color_t image_pget(const image_t img, int x, int y) {
    if (!img || x < 0 || y < 0)
        return _black;
    int w, h;
    image_size(img, &w, &h);
    if (x >= w || y >= h || w <= 0 || h <= 0)
        return _black;
    return img[y * w + x];
}

bool image_fill(image_t img, color_t color) {
    if (!img)
        return false;
    int w, h;
    image_size(img, &w, &h);
    for (int i = 0; i < w * h; ++i)
        img[i] = color;
    return true;
}

bool color_cmp(color_t a, color_t b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static inline void _flood(image_t img, int x, int y, color_t _new, color_t _old) {
    if (color_cmp(_new, _old) || !color_cmp(image_pget(img, x, y), _old))
        return;

    int x1 = x;
    int w = image_width(img);
    while (x1 < w && color_cmp(image_pget(img, x1, y), _old)) {
        image_pset(img, x1, y, _new);
        x1++;
    }

    x1 = x - 1;
    while (x1 >= 0 && color_cmp(image_pget(img, x1, y), _old)) {
        image_pset(img, x1, y, _new);
        x1--;
    }

    x1 = x;
    while (x1 < w && color_cmp(image_pget(img, x1, y), _old)) {
        if(y > 0 && color_cmp(image_pget(img, x1, y - 1), _old))
            _flood(img, x1, y - 1, _new, _old);
        x1++;
    }

    x1 = x - 1;
    while(x1 >= 0 && color_cmp(image_pget(img, x1, y), _old)) {
        if(y > 0 && color_cmp(image_pget(img, x1, y - 1), _old))
            _flood(img, x1, y - 1, _new, _old);
        x1--;
    }

    x1 = x;
    int h = image_height(img);
    while(x1 < w && color_cmp(image_pget(img, x1, y), _old)) {
        if(y < h - 1 && color_cmp(image_pget(img, x1, y + 1), _old))
            _flood(img, x1, y + 1, _new, _old);
        x1++;
    }

    x1 = x - 1;
    while(x1 >= 0 && color_cmp(image_pget(img, x1, y), _old)) {
        if(y < h - 1 && color_cmp(image_pget(img, x1, y + 1), _old))
            _flood(img, x1, y + 1, _new, _old);
        x1--;
    }
}

bool image_flood(image_t img, int x, int y, color_t color) {
    if (!img || x < 0 || y < 0)
        return false;
    int w, h;
    image_size(img, &w, &h);
    if (x >= w || y >= h || w <= 0 || h <= 0)
        return false;
    _flood(img, x, y, color, image_pget(img, x, y));
    return true;
}

bool image_paste(image_t dst, const image_t src, int x, int y) {
    if (!src || !dst || x < 0 || y < 0)
        return false;
    int w, h;
    image_size(src, &w, &h);
    int dw, dh;
    image_size(dst, &dw, &dh);
    if (x >= dw || y >= dh || dw <= 0 || dh <= 0)
        return false;
    for (int ox = 0; ox < w; ++ox) {
        for (int oy = 0; oy < h; ++oy) {
            if (oy > h)
                break;
            image_pset(dst, x + ox, y + oy, image_pget(src, ox, oy));
        }
        if (ox > w)
            break;
    }
    return true;
}

bool image_clipped_paste(image_t dst, const image_t src, int x, int y, int rx, int ry, int rw, int rh) {
    if (!src || !dst || x < 0 || y < 0 || rw <= 0 || rh <= 0 || rx < 0 || ry < 0)
        return false;
    int w, h;
    image_size(src, &w, &h);
    if (rx + rw > w || ry + rh > h)
        return false;
    if (x + rw > image_width(dst) || y + rh > image_height(dst))
        return false;
    int dw, dh;
    image_size(dst, &dw, &dh);
    if (x > dw || y > dh)
        return false;
    for (int ox = 0; ox < rw; ++ox)
        for (int oy = 0; oy < rh; ++oy)
            image_pset(dst, ox + x, oy + y, image_pget(src, ox + rx, oy + ry));
    return true;
}

image_t image_dupe(image_t src) {
    if (!src)
        return NULL;
    int w, h;
    image_size(src, &w, &h);
    image_t result = image_make(w, h);
    if (!result)
        return NULL;
    memcpy(result, src, w * h * sizeof(uint32_t));
    return result;
}

image_t image_resized(image_t src, int nw, int nh) {
    if (!src || nw <= 0 || nh <= 0)
        return NULL;
    int w, h;
    image_size(src, &w, &h);
    image_t result = image_make(nw, nh);
    if (!result)
        return false;
    int x_ratio = (int)((w << 16) / nw) + 1;
    int y_ratio = (int)((h << 16) / nh) + 1;
    int x2, y2, i, j;
    for (i = 0; i < nh; ++i) {
        color_t *t = result + i * nw;
        y2 = ((i * y_ratio) >> 16);
        color_t *p = src + y2 * w;
        int rat = 0;
        for (j = 0; j < nw; ++j) {
            x2 = (rat >> 16);
            *t++ = p[x2];
            rat += x_ratio;
        }
    }
    return result;
}

bool image_resize(image_t *src, int nw, int nh) {
    if (!src || !*src || nw < 0 || nh < 0)
        return false;
    int w, h;
    image_size(*src, &w, &h);
    if (w == nw && h == nh)
        return false;
    image_t result = image_resized(*src, nw, nh);
    if (result)
        return false;
    image_destroy(*src);
    *src = result;
    return true;
}

image_t image_rotated(image_t src, float angle) {
    if (!src)
        return NULL;
    int w, h;
    image_size(src, &w, &h);

    float theta = _RADIANS(angle);
    float c = cosf(theta), s = sinf(theta);
    float r[3][2] = {
        { -h * s, h * c },
        {  w * c - h * s, h * c + w * s },
        {  w * c, w * s }
    };

    float mm[2][2] = {{
        _MIN(0, _MIN(r[0][0], _MIN(r[1][0], r[2][0]))),
        _MIN(0, _MIN(r[0][1], _MIN(r[1][1], r[2][1])))
    }, {
        (theta > 1.5708  && theta < 3.14159 ? 0.f : _MAX(r[0][0], _MAX(r[1][0], r[2][0]))),
        (theta > 3.14159 && theta < 4.71239 ? 0.f : _MAX(r[0][1], _MAX(r[1][1], r[2][1])))
    }};

    int dw = (int)ceil(fabsf(mm[1][0]) - mm[0][0]);
    int dh = (int)ceil(fabsf(mm[1][1]) - mm[0][1]);
    image_t result = image_make(dw, dh);
    if (!result)
        return NULL;
    int x, y, sx, sy;
    for (x = 0; x < dw; ++x)
        for (y = 0; y < dh; ++y) {
            sx = ((x + mm[0][0]) * c + (y + mm[0][1]) * s);
            sy = ((y + mm[0][1]) * c - (x + mm[0][0]) * s);
            if (sx < 0 || sx >= w || sy < 0 || sy >= h)
                continue;
            image_pset(result, x, y, image_pget(src, sx, sy));
        }
    return result;
}

bool image_rotate(image_t *src, float angle) {
    if (!src || !*src)
        return false;
    int w, h;
    image_size(*src, &w, &h);
    image_t result = image_rotated(*src, angle);
    if (!result)
        return false;
    image_destroy(*src);
    *src = result;
    return true;
}

image_t image_clipped(image_t src, int rx, int ry, int rw, int rh) {
    if (!src || rw <= 0 || rh <= 0 || rx < 0 || ry < 0)
        return NULL;
    int w, h;
    image_size(src, &w, &h);
    if (rx >= w || ry >= h || rw <= 0 || rh <= 0)
        return NULL;

    int ox = _CLAMP(rx, 0, w);
    int oy = _CLAMP(ry, 0, h);
    if (ox >= w || oy >= h)
        return NULL;
    int mx = _MIN(ox + rw, w);
    int my = _MIN(oy + rh, h);
    int iw = mx - ox;
    int ih = my - oy;
    if (iw <= 0 || ih <= 0)
        return NULL;
    color_t *result = image_make(rw, rh);
    if (!result)
        return NULL;
    for (int px = 0; px < iw; px++)
        for (int py = 0; py < ih; py++) {
            int cx = ox + px;
            int cy = oy + py;
            image_pset(result, px, py, image_pget(src, cx, cy));
        }
    return result;
}

bool image_clip(image_t *src, int rx, int ry, int rw, int rh) {
    if (!src || !*src || rw <= 0 || rh <= 0 || rx < 0 || ry < 0)
        return false;
    int w, h;
    image_size(*src, &w, &h);
    if (rx >= w || ry >= h || rw <= 0 || rh <= 0)
        return false;
    image_t result = image_clipped(*src, rx, ry, rw, rh);
    if (!result)
        return false;
    image_destroy(*src);
    *src = result;
    return true;
}

static inline void vline(image_t img, int w, int h, int x, int y0, int y1, color_t color) {
    if (y1 < y0) {
        y0 += y1;
        y1  = y0 - y1;
        y0 -= y1;
    }

    if (x < 0 || x >= w || y0 >= h)
        return;

    if (y0 < 0)
        y0 = 0;
    if (y1 >= h)
        y1 = h - 1;

    for(int y = y0; y <= y1; y++)
        image_pset(img, x, y, color);
}

static inline void hline(image_t img, int w, int h, int y, int x0, int x1, color_t color) {
    if (x1 < x0) {
        x0 += x1;
        x1  = x0 - x1;
        x0 -= x1;
    }

    if (y < 0 || y >= h || x0 >= w)
        return;

    if (x0 < 0)
        x0 = 0;
    if (x1 >= w)
        x1 = w - 1;

    for(int x = x0; x <= x1; x++)
        image_pset(img, x, y, color);
}

bool image_draw_line(image_t img, int x0, int y0, int x1, int y1, color_t color) {
    if (!img || x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0)
        return false;
    int w, h;
    image_size(img, &w, &h);
    if (x0 >= w || y0 >= h || x1 >= w || y1 >= h || w <= 0 || h <= 0)
        return false;

    if (x0 == x1)
        vline(img, w, h, x0, y0, y1, color);
    else if (y0 == y1)
        hline(img, w, h, y0, x0, x1, color);
    else {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = (dx > dy ? dx : -dy) / 2;

        while (image_pset(img, x0, y0, color), x0 != x1 || y0 != y1) {
            int e2 = err;
            if (e2 > -dx) { err -= dy; x0 += sx; }
            if (e2 <  dy) { err += dx; y0 += sy; }
        }
    }

    return true;
}

bool image_draw_circle(image_t img, int xc, int yc, int r, color_t color, int fill) {
    if (!img || xc < 0 || yc < 0 || r <= 0)
        return false;
    int w, h;
    image_size(img, &w, &h);
    if (xc >= w || yc >= h || xc < 0 || yc < 0 || w <= 0 || h <= 0)
        return false;

    int x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
    do {
        image_pset(img, xc - x, yc + y, color);    /*   I. Quadrant */
        image_pset(img, xc - y, yc - x, color);    /*  II. Quadrant */
        image_pset(img, xc + x, yc - y, color);    /* III. Quadrant */
        image_pset(img, xc + y, yc + x, color);    /*  IV. Quadrant */

        if (fill) {
            hline(img, w, h, yc - y, xc - x, xc + x, color);
            hline(img, w, h, yc + y, xc - x, xc + x, color);
        }

        r = err;
        if (r <= y)
            err += ++y * 2 + 1; /* e_xy+e_y < 0 */
        if (r > x || err > y)
            err += ++x * 2 + 1; /* e_xy+e_x > 0 or no 2nd y-step */
    } while (x < 0);
    
    return true;
}

bool image_draw_rectangle(image_t img, int x, int y, int w, int h, color_t color, int fill) {
    if (!img || w <= 0 || h <= 0)
        return false;
    int img_w, img_h;
    image_size(img, &img_w, &img_h);
    if (x >= img_w || y >= img_h || x < 0 || y < 0 || img_w <= 0 || img_h <= 0)
        return false;

    if (x < 0) {
        w += x;
        x  = 0;
    }
    if (y < 0) {
        h += y;
        y  = 0;
    }

    w += x;
    h += y;
    if (w < 0 || h < 0 || x > img_w || y > img_h)
        return false;

    if (w > img_w)
        w = img_w;
    if (h > img_h)
        h = img_h;

    if (fill) {
        for (; y < h; ++y)
            hline(img, img_w, img_h, y, x, w, color);
    } else {
        hline(img, img_w, img_h, y, x, w, color);
        hline(img, img_w, img_h, h, x, w, color);
        vline(img, img_w, img_h, x, y, h, color);
        vline(img, img_w, img_h, w, y, h, color);
    }

    return true;
}

bool image_draw_triangle(image_t img, int x0, int y0, int x1, int y1, int x2, int y2, color_t color, int fill) {
    if (!img || x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0)
        return false;
    int w, h;
    image_size(img, &w, &h);
    if (x0 >= w || y0 >= h || x1 >= w || y1 >= h || x2 >= w || y2 >= h || w <= 0 || h <= 0)
        return false;

    if (y0 ==  y1 && y0 ==  y2)
        return false;
    if (fill) {
        if (y0 > y1) {
            _SWAP(x0, x1);
            _SWAP(y0, y1);
        }
        if (y0 > y2) {
            _SWAP(x0, x2);
            _SWAP(y0, y2);
        }
        if (y1 > y2) {
            _SWAP(x1, x2);
            _SWAP(y1, y2);
        }

        int total_height = y2 - y0, i, j;
        for (i = 0; i < total_height; ++i) {
            int second_half = i > y1 - y0 || y1 == y0;
            int segment_height = second_half ? y2 - y1 : y1 - y0;
            float alpha = (float)i / total_height;
            float beta  = (float)(i - (second_half ? y1 - y0 : 0)) / segment_height;
            int ax = x0 + (x2 - x0) * alpha;
            int ay = y0 + (y2 - y0) * alpha;
            int bx = second_half ? x1 + (x2 - x1) : x0 + (x1 - x0) * beta;
            int by = second_half ? y1 + (y2 - y1) : y0 + (y1 - y0) * beta;
            if (ax > bx) {
                _SWAP(ax, bx);
                _SWAP(ay, by);
            }
            for (j = ax; j <= bx; ++j)
                image_pset(img, j, y0 + i, color);
        }
    } else {
        image_draw_line(img, x0, y0, x1, y1, color);
        image_draw_line(img, x1, y1, x2, y2, color);
        image_draw_line(img, x2, y2, x0, y0, color);
    }
    
    return true;
}
#endif
