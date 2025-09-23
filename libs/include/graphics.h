#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <psptypes.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define PSP_LINE_SIZE 512

typedef u32 Color;

typedef struct {
    int textureWidth;   // power of 2
    int textureHeight;  // power of 2
    int imageWidth;     // actual image width
    int imageHeight;    // actual image height
    Color* data;        // image data
} Image;

// Initialize graphics
void initGraphics();

// Screen functions
void clearScreen(Color color);
void flipScreen();
Color* getVramDrawBuffer();

// Image functions
Image* loadImage(const char* filename);
void freeImage(Image* image);
void blitImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy);
void blitAlphaImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy);

// Drawing functions
void drawRect(Color color, int x, int y, int w, int h);
void putPixel(Color color, int x, int y);

#endif