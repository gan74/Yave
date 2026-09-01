#ifndef DEBUG_SLANG
#define DEBUG_SLANG

#include "semantics.slang"

// 4 * 5 bitmaps
#define CHAR_MINUS      0x000700u
#define CHAR_DOT        0x000002u
#define CHAR_COLON      0x002020u
#define CHAR_QUESTION   0x074602u

#define CHAR_SPACE      0x000000u

#define CHAR_A          0x075755u
#define CHAR_B          0x035353u
#define CHAR_C          0x071117u
#define CHAR_D          0x035553u
#define CHAR_E          0x071717u
#define CHAR_F          0x071711u
#define CHAR_G          0x071157u
#define CHAR_H          0x055755u
#define CHAR_I          0x022222u
#define CHAR_J          0x022223u
#define CHAR_K          0x055355u
#define CHAR_L          0x022226u
#define CHAR_M          0x057555u
#define CHAR_N          0x055575u
#define CHAR_O          0x075557u
#define CHAR_P          0x075711u
#define CHAR_Q          0x075574u
#define CHAR_R          0x075755u
#define CHAR_S          0x071747u
#define CHAR_T          0x072222u
#define CHAR_U          0x055557u
#define CHAR_V          0x055552u

#define CHAR_X          0x055255u
#define CHAR_Y          0x055222u
#define CHAR_Z          0x074217u

#define CHAR_0          0x075557u
#define CHAR_1          0x022222u
#define CHAR_2          0x074717u
#define CHAR_3          0x074747u
#define CHAR_4          0x055744u
#define CHAR_5          0x071747u
#define CHAR_6          0x071757u
#define CHAR_7          0x074444u
#define CHAR_8          0x075757u
#define CHAR_9          0x075747u

struct DebugPrinter {
    float2 font_size;
    float2 cursor;
    float2 pixel_coord;
    float start_x;
    float value;

    __init(uint2 pixel_coord, uint2 start_coord = uint2(0, 0)) {
        this.font_size = float2(8.0, 15.0);
        this.cursor = float2(start_coord);
        this.pixel_coord = float2(pixel_coord);
        this.start_x = float(start_coord.x);
        this.value = 0.0;
    }

    // https://www.shadertoy.com/view/4sBSWW
    static float char_mask(float2 pos, uint char_bits) {
        if(pos.x < 0.0 || pos.y < 0.0 || pos.x >= 1.0 || pos.y >= 1.0) {
            return 0.0;
        }
        const uint bits = uint(pos.x * 4.0) | (uint((1.0 - pos.y) * 5.0) << 2);
        return ((char_bits / (1 << bits)) & 0x01) != 0 ? 1.0 : 0.0;
    }

    static uint digit_bits(int x) {
        if(x < 0 || x > 9) {
            return 0;
        }
        const uint bits[10] = {CHAR_0, CHAR_1, CHAR_2, CHAR_3, CHAR_4, CHAR_5, CHAR_6, CHAR_7, CHAR_8, CHAR_9};
        return bits[x];
    }

    [mutating]
    void putc(uint char_bits) {
        const float2 uv = (pixel_coord - cursor) / font_size;
        value += char_mask(uv, char_bits);
        cursor.x += font_size.x;
    }

    [mutating]
    void puts<each T>(expand each T chars) where T == uint {
        expand putc(each chars);
    }

    [mutating]
    void putf(float v, float max_digits, float decimals) {
        const float2 uv = (pixel_coord - cursor) / font_size;
        value += print_value_impl(uv, v, max_digits, decimals);
        cursor.x += font_size.x * (max_digits + decimals);
    }

    [mutating]
    void next_line() {
        cursor.x = start_x;
        cursor.y += font_size.y;
    }

    static float print_value_impl(float2 pos, float value, float max_digits, float decimals) {
        if(pos.y < 0.0 || pos.y >= 1.0) {
            return 0.0;
        }

        const bool is_neg = (value < 0.0);
        value = abs(value);

        const float digits = log2(abs(value)) / log2(10.0);
        const float max_index = max(floor(digits), 0.0);
        float index = max_digits - floor(pos.x);
        uint char_bits = 0;
        if(index > -decimals - 1.01) {
            if(index > max_index) {
                if(is_neg && index < max_index + 1.5) {
                    char_bits = CHAR_MINUS;
                }
            } else {
                if(index == -1.0) {
                    if(decimals > 0.0) {
                        char_bits = CHAR_DOT;
                    }
                } else {
                    float reduced_range_value = value;
                    if(index < 0.0) {
                        reduced_range_value = fract(value);
                        index += 1.0;
                    }
                    const float digit_value = abs(reduced_range_value / pow(10.0, index));
                    char_bits = digit_bits(int(floor(fmod(digit_value, 10.0))));
                }
            }
        }

        return char_mask(float2(fract(pos.x), pos.y), char_bits);
    }

    static float print_value(float2 coord, float2 px_coord, float2 font_size, float value, float max_digits, float decimals) {
        const float2 uv = (coord - px_coord) / font_size;
        return print_value_impl(uv, value, max_digits, decimals);
    }
};

#endif
