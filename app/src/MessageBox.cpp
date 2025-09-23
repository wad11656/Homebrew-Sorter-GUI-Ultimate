#include "MessageBox.h"
#include <pspgu.h>
#include <pspdisplay.h>
#include <string>
#include <vector>
#include <cstring>

// Filled-rect helper (2-vertex sprite)
static void mbDrawRect(int x, int y, int w, int h, unsigned color) {
    struct V { unsigned color; short x, y, z; };
    V* v = (V*)sceGuGetMemory(2 * sizeof(V));
    v[0] = { color, (short)x,       (short)y,       0 };
    v[1] = { color, (short)(x + w), (short)(y + h), 0 };
    sceGuDisable(GU_TEXTURE_2D);
    sceGuShadeModel(GU_FLAT);
    sceGuAmbientColor(0xFFFFFFFF);
    sceGuDrawArray(GU_SPRITES,
                   GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                   2, 0, v);
}

// Trim to ~35 chars like the homebrew, add "..." if needed
static std::string mbTrim35(const std::string& s) {
    if (s.size() <= 35) return s;
    std::string out = s.substr(0, 35);
    out += "...";
    return out;
}

MessageBox::MessageBox(const char* message,
                       Texture* okIcon,
                       int screenW, int screenH,
                       float textScale,
                       int iconTargetH,
                       const char* okLabel,
                       int padX,   int padY,
                       int wrapTweakPx,
                       int forcedPxPerChar)
: _msg(message),
  _okLabel(okLabel),
  _icon(okIcon),
  _screenW(screenW), _screenH(screenH),
  _x(0), _y(0), _w(380), _h(140),
  _padX(padX), _padY(padY),
  _wrapTweakPx(wrapTweakPx),
  _forcedPxPerChar(forcedPxPerChar),
  _textScale(textScale),
  _iconTargetH(iconTargetH),
  _visible(true) {
    // center the panel
    _x = (_screenW - _w) / 2;
    _y = (_screenH - _h) / 2;
}

bool MessageBox::update() {
    if (!_visible) return false;

    SceCtrlData pad{};
    sceCtrlReadBufferPositive(&pad, 1);

    // Edge-detect locally; ONLY CROSS closes
    static unsigned last = 0;
    unsigned pressed = pad.Buttons & ~last;
    last = pad.Buttons;

    if (pressed & PSP_CTRL_CROSS) {
        _visible = false;
    }
    return _visible;
}

// very simple char-based wrapper with a better px/char estimate
static void wrapTextByChars(const char* text, int maxCharsPerLine, std::vector<std::string>& outLines) {
    outLines.clear();
    if (!text || !*text) return;

    const char* s = text;
    while (*s) {
        int take = 0, lastSpace = -1;
        while (s[take] && take < maxCharsPerLine) {
            if (s[take] == ' ') lastSpace = take;
            if (s[take] == '\n') { lastSpace = take; break; }
            ++take;
        }
        int cut = take;
        if (s[cut] == '\n') {
            // break at newline
        } else if (take == maxCharsPerLine && lastSpace >= 0) {
            cut = lastSpace; // wrap at last space
        }

        outLines.emplace_back(std::string(s, s + cut));

        if (s[cut] == '\n')      s += cut + 1;
        else if (cut == 0)       s += 1;            // force progress on long word
        else                     s += (cut < take ? cut + 1 : cut);

        while (*s == ' ') ++s;   // trim leading spaces for next line
    }
}

void MessageBox::render(intraFont* font) {
    if (!_visible) return;

    // Dim overlay
    sceGuDisable(GU_DEPTH_TEST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    mbDrawRect(0, 0, _screenW, _screenH, 0x88000000);

    // Panel + border
    const unsigned COLOR_PANEL    = 0xD0303030;
    const unsigned COLOR_BORDER   = 0xFFFFFFFF;
    const unsigned COLOR_TEXT     = 0xFFFFFFFF;
    const unsigned PROG_BAR_BG    = 0xFF666666;
    const unsigned PROG_BAR_FILL  = 0xFFFFFFFF; // white fill; simple and readable

    mbDrawRect(_x - 1, _y - 1, _w + 2, _h + 2, COLOR_BORDER);
    mbDrawRect(_x, _y, _w, _h, COLOR_PANEL);

    // Content box (respect padding; padY lowers the first line)
    const int innerX = _x + _padX;
    const int innerY = _y + _padY;
    const int innerW = _w - 2 * _padX;

    // px/char estimate (override if forcedPxPerChar > 0)
    int approxPxPerChar = (_forcedPxPerChar > 0)
                        ? _forcedPxPerChar
                        : (int)(14.0f * _textScale + 0.5f);
    if (approxPxPerChar < 6) approxPxPerChar = 6;
    // Give some slack so it wraps later if desired
    int maxChars = (innerW + _wrapTweakPx) / approxPxPerChar;
    if (maxChars < 8) maxChars = 8;

    // ---- Title / message (existing) ----
    std::vector<std::string> lines;
    wrapTextByChars(_msg ? _msg : "", maxChars, lines);

    int y = innerY;
    if (font) {
        intraFontSetStyle(font, _textScale, COLOR_TEXT, 0, 0.0f, INTRAFONT_ALIGN_LEFT);

        // line height ~ 24 px at 1.0 scale
        const int lineH = (int)(24.0f * _textScale + 0.5f);
        // reserve bottom area for icon/OK OR for the progress bar, whichever is larger
        const int bottomAreaH = (_progEnabled ? (lineH + 16 /*filename*/ + 10 /*gap*/ + 4 /*bar*/) : (_iconTargetH + 14));
        const int maxY = _y + _h - bottomAreaH - 6;

        for (size_t i = 0; i < lines.size(); ++i) {
            intraFontPrint(font, (float)innerX, (float)y, lines[i].c_str());
            y += lineH;
            if (y > maxY) break;
        }
    }

    // ---- Progress UI (new) ----
    if (_progEnabled) {
        // second line: per-file message centered (trim to ~35 like the homebrew)
        if (font) {
            std::string trimmed = mbTrim35(_progMsg);
            intraFontSetStyle(font, _textScale, COLOR_TEXT, 0, 0.0f, INTRAFONT_ALIGN_CENTER);
            float cx = _x + _w * 0.5f;
            float cy = (float)(y + 8); // a little gap after the title lines
            intraFontPrint(font, cx, cy, trimmed.c_str());
            y = (int)(cy + (int)(24.0f * _textScale + 0.5f)); // advance for bar
        }

        // progress bar (fixed 4px tall; ~318px wide like the homebrew)
        const int barH = 4;
        const int barW = (innerW > 30) ? (innerW - 30) : innerW; // gives ~318px with your defaults
        const int barX = innerX + (innerW - barW) / 2;
        const int barY = y + 6;

        // background
        mbDrawRect(barX, barY, barW, barH, PROG_BAR_BG);

        // filled portion
        float frac = 0.0f;
        if (_progSize > 0) {
            frac = (float)(_progOffset <= _progSize ? _progOffset : _progSize) / (float)_progSize;
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;
        }
        int fillW = (int)(barW * frac + 0.5f);
        if (fillW > 0) mbDrawRect(barX, barY, fillW, barH, PROG_BAR_FILL);

        // When progress is shown, we intentionally DO NOT draw the bottom icon/"OK"
        // to keep the dialog minimal and avoid implying that CROSS cancels the copy.
        return;
    }

    // ---- Original bottom icon + "OK" (only when not in progress mode) ----
    const float iconH = (float)_iconTargetH;
    float drawW = 0.0f;

    if (_icon && _icon->data && _icon->height > 0) {
        float scale = iconH / (float)_icon->height;
        drawW = _icon->width * scale;
    }

    // Approx text width for short labels like "OK"
    float fontPx = 24.0f * _textScale;
    float okTextW = (float)std::strlen(_okLabel ? _okLabel : "OK") * (fontPx * 0.55f);
    float gap = 6.0f;

    // Center the whole (icon + gap + text) horizontally
    float totalW = drawW + (drawW > 0 ? gap : 0) + okTextW;
    float startX = _x + (_w - totalW) * 0.5f;

    // Baseline Y so text vertically centers against the icon
    const int bottomAreaTop = _y + _h - (int)(iconH + 14);
    float iy0 = (float)bottomAreaTop;
    float iy1 = iy0 + iconH;
    float textBaseline = iy0 + iconH * 0.5f + fontPx * 0.35f; // nudge factor

    // Icon
    if (_icon && _icon->data && drawW > 0.0f) {
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888, 0, 0, GU_FALSE);
        sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
        sceGuTexImage(0, _icon->stride, _icon->stride, _icon->stride, _icon->data);
        sceGuTexFilter(GU_NEAREST, GU_NEAREST);
        sceGuTexWrap(GU_CLAMP, GU_CLAMP);

        float ix0 = startX;
        float ix1 = ix0 + drawW;

        struct V { float u, v; unsigned color; float x, y, z; };
        V* v = (V*)sceGuGetMemory(2 * sizeof(V));
        v[0].u = 0.0f;                 v[0].v = 0.0f;
        v[0].x = ix0;                  v[0].y = iy0;  v[0].z = 0.0f; v[0].color = 0xFFFFFFFF;
        v[1].u = (float)_icon->width;  v[1].v = (float)_icon->height;
        v[1].x = ix1;                  v[1].y = iy1;  v[1].z = 0.0f; v[1].color = 0xFFFFFFFF;

        sceGuDrawArray(GU_SPRITES,
                       GU_TEXTURE_32BITF | GU_VERTEX_32BITF |
                       GU_COLOR_8888    | GU_TRANSFORM_2D,
                       2, nullptr, v);

        sceGuDisable(GU_TEXTURE_2D);

        if (font) {
            intraFontSetStyle(font, _textScale, COLOR_TEXT, 0, 0.0f, INTRAFONT_ALIGN_LEFT);
            float okX = ix1 + gap;
            intraFontPrint(font, okX, textBaseline, _okLabel ? _okLabel : "OK");
        }
    } else {
        // No icon → center the label
        if (font) {
            intraFontSetStyle(font, _textScale, COLOR_TEXT, 0, 0.0f, INTRAFONT_ALIGN_CENTER);
            float cx = _x + _w * 0.5f;
            float cy = (float)bottomAreaTop + iconH * 0.5f + fontPx * 0.35f;
            intraFontPrint(font, cx, cy, _okLabel ? _okLabel : "OK");
        }
    }
}

// ---- New: progress API impl ----
void MessageBox::showProgress(const char* fileMessage, uint64_t offset, uint64_t size) {
    _progEnabled = true;
    _progMsg     = fileMessage ? fileMessage : "";
    _progOffset  = offset;
    _progSize    = (size == 0) ? 1 : size;
}
void MessageBox::updateProgress(uint64_t offset, uint64_t size, const char* fileMessage) {
    _progOffset = offset;
    _progSize   = (size == 0) ? 1 : size;
    if (fileMessage) _progMsg = fileMessage;
}
void MessageBox::hideProgress() {
    _progEnabled = false;
    _progMsg.clear();
    _progOffset = 0;
    _progSize   = 1;
}
