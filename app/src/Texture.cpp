#include "Texture.h"
#include <pspgu.h>
#include <string.h>
#include <malloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>   // you already have this in ../libs/include

Texture* texLoadPNG(const char* path) {
    int w=0, h=0, comp=0;
    unsigned char* pix = stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);
    if (!pix) return nullptr;

    // Pad to next power-of-two (GU expects POT textures)
    int tw = 1; while (tw < w) tw <<= 1;
    int th = 1; while (th < h) th <<= 1;

    // 16-byte align for GU
    uint32_t* p2buf = (uint32_t*)memalign(16, tw * th * 4);
    if (!p2buf) { stbi_image_free(pix); return nullptr; }
    memset(p2buf, 0, tw * th * 4);

    for (int y = 0; y < h; ++y) {
        memcpy(p2buf + y * tw, pix + y * w * 4, w * 4);
    }
    stbi_image_free(pix);

    Texture* t = (Texture*)malloc(sizeof(Texture));
    if (!t) { free(p2buf); return nullptr; }
    t->width  = w;
    t->height = h;
    t->stride = tw;
    t->data   = p2buf;
    return t;
}

void texFree(Texture* t) {
    if (!t) return;
    if (t->data) free(t->data);
    free(t);
}

Texture* texLoadPNGFromMemory(const unsigned char* data, int len) {
    int w=0, h=0, comp=0;
    unsigned char* pix = stbi_load_from_memory(data, len, &w, &h, &comp, STBI_rgb_alpha);
    if (!pix) return nullptr;

    int tw = 1; while (tw < w) tw <<= 1;
    int th = 1; while (th < h) th <<= 1;

    uint32_t* p2buf = (uint32_t*)memalign(16, tw * th * 4);
    if (!p2buf) { stbi_image_free(pix); return nullptr; }
    memset(p2buf, 0, tw * th * 4);

    for (int y = 0; y < h; ++y) memcpy(p2buf + y * tw, pix + y * w * 4, w * 4);
    stbi_image_free(pix);

    Texture* t = (Texture*)malloc(sizeof(Texture));
    if (!t) { free(p2buf); return nullptr; }
    t->width  = w;
    t->height = h;
    t->stride = tw;
    t->data   = p2buf;
    return t;
}
