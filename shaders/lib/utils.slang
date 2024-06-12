
float4 unpack_2_10_10_10(uint packed) {
    return float4(
        (float3(
            (packed >> 20) & 0x03FF,
            (packed >> 10) & 0x03FF,
            (packed >>  0) & 0x03FF
        ) / float(0x03FF)) * 2.0 - 1.0,
        (packed >> 30 == 0) ? 1.0 : -1.0
    );
}
