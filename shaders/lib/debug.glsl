#ifndef DEBUG_GLSL
#define DEBUG_GLSL

// https://www.shadertoy.com/view/4sBSWW
float digit_bin(int x) {
    return x == 0
        ? 480599.0
        : x == 1
        ? 139810.0
        : x == 2
        ? 476951.0
        : x == 3
        ? 476999.0
        : x == 4
        ? 350020.0
        : x == 5
        ? 464711.0
        : x == 6
        ? 464727.0
        : x == 7
        ? 476228.0
        : x == 8
        ? 481111.0
        : x == 9
        ? 481095.0
        : 0.0;
}

float print_value(vec2 pos, float value, float max_digits, float decimals) {
    if(pos.y < 0.0 || pos.y >= 1.0) {
        return 0.0;
    }

    const bool is_neg = (value < 0.0);
    value = abs(value);

    const float digits = log2(abs(value)) / log2(10.0);
    const float max_index = max(floor(digits), 0.0);
    float index = max_digits - floor(pos.x);
    float char_bin = 0.0;
    if(index > -decimals - 1.01) {
        if(index > max_index) {
            if(is_neg && index < max_index + 1.5) {
                char_bin = 1792.0;
            }
        } else {
            if(index == -1.0) {
                if(decimals > 0.0) {
                    char_bin = 2.0;
                }
            } else {
                float reduced_range_value = value;
                if(index < 0.0) {
                    reduced_range_value = fract(value);
                    index += 1.0;
                }
                const float digit_value = abs(reduced_range_value / pow(10.0, index));
                char_bin = digit_bin(int(floor(mod(digit_value, 10.0))));
            }
        }
    }

    return floor(mod((char_bin / pow(2.0, floor(fract(pos.x) * 4.0) + (floor((1.0 - pos.y) * 5.0) * 4.0))), 2.0));
}

float print_value(vec2 coord, vec2 px_coord, vec2 font_size, float value, float max_digits, float decimals) {
    const vec2 uv = (coord - px_coord) / font_size;
    return print_value(uv, value, max_digits, decimals);
}

#endif

