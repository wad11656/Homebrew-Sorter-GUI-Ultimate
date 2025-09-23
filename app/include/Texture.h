#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdint.h>

// Linear RGBA8888 texture, padded to power-of-two (PSP GU requirement)
struct Texture {
    int       width;   // real image width
    int       height;  // real image height
    int       stride;  // padded row width (power-of-two)
    uint32_t* data;    // pixels (aligned, linear, not swizzled)
};

// Load a PNG from a full PSP path (ms0:/... or ef0:/...)
// Returns nullptr on failure.
Texture* texLoadPNG(const char* fullPath);
Texture* texLoadPNGFromMemory(const unsigned char* data, int len);

// Free both the pixel buffer and the Texture object.
void texFree(Texture* t);

#endif // TEXTURE_H
