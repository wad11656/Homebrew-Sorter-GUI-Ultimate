#pragma once

#include <glib2d.h>
#include <intraFont.h>
#include <string>

extern intraFont *font;
extern char font_size_cache[256];

namespace G2D {
    void DrawRect(float x, float y, float width, float height, g2dColor colour);
    void DrawImage(g2dTexture *tex, float x, float y);
    void DrawImageScale(g2dTexture *tex, float x, float y, float w, float h);
    char *KeyboardGetText(const std::string &desc_msg, const std::string &initial_msg);
    void FontSetStyle(float size, unsigned int colour, unsigned int options);
    float GetTextHeight(void);
    float DrawText(float x, float y, const std::string &text);
}
