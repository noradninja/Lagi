// Shared Neptune VDP2 shader helpers.
// These implement raw Saturn VRAM/CRAM access and color decoding.
// Keep screen/layer policy out of this file: Azel owns VDP2 state.

float4 rawTexel(uniform sampler2D raw, float texelIndex)
{
    const float x = fmod(texelIndex, 512.0f);
    const float y = floor(texelIndex / 512.0f);
    return tex2D(raw, float2(
        (x + 0.5f) / 512.0f,
        (y + 0.5f) / 258.0f));
}

float selectByte(float4 v, float lane)
{
    if (lane < 0.5f) return floor(v.r * 255.0f + 0.5f);
    if (lane < 1.5f) return floor(v.g * 255.0f + 0.5f);
    if (lane < 2.5f) return floor(v.b * 255.0f + 0.5f);
    return floor(v.a * 255.0f + 0.5f);
}

float readByte(uniform sampler2D raw, float byteAddress)
{
    const float texelIndex = floor(byteAddress * 0.25f);
    const float lane = byteAddress - texelIndex * 4.0f;
    return selectByte(rawTexel(raw, texelIndex), lane);
}

float readBE16(uniform sampler2D raw, float byteAddress)
{
    return readByte(raw, byteAddress) * 256.0f +
           readByte(raw, byteAddress + 1.0f);
}

float readLE16(uniform sampler2D raw, float byteAddress)
{
    return readByte(raw, byteAddress) +
           readByte(raw, byteAddress + 1.0f) * 256.0f;
}

// Native Azel rotation/coefficient work areas are copied to emulated VRAM
// from the little-endian Vita CPU. Decode a signed 16.16 value without first
// constructing a 32-bit integer in float (which would discard useful low
// bits near the signed range limits).
float readLEFixed16_16(uniform sampler2D raw, float byteAddress)
{
    const float lo = readLE16(raw, byteAddress);
    float hi = readLE16(raw, byteAddress + 2.0f);
    if (hi >= 32768.0f)
        hi -= 65536.0f;
    return hi + lo / 65536.0f;
}

float readLESigned16(uniform sampler2D raw, float byteAddress)
{
    float v = readLE16(raw, byteAddress);
    if (v >= 32768.0f)
        v -= 65536.0f;
    return v;
}

float4 rgb555(float c)
{
    const float r5 = fmod(c, 32.0f);
    const float g5 = fmod(floor(c / 32.0f), 32.0f);
    const float b5 = fmod(floor(c / 1024.0f), 32.0f);
    return float4(r5 / 31.0f, g5 / 31.0f, b5 / 31.0f, 1.0f);
}

