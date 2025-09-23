#include <stdlib.h>
#include <malloc.h>
#include <pspdisplay.h>
#include <psputils.h>
#include <png.h>
#include <pspgu.h>
#include <string.h>

#include "graphics.h"

static int initialized = 0;
static Color* vram_base = (Color*)(0x40000000 | 0x04000000);
static Color* disp_buffer = (Color*)0;
static Color* draw_buffer = (Color*)0;

static int power_of_two(int value) {
    int poweroftwo = 1;
    while (poweroftwo < value) {
        poweroftwo <<= 1;
    }
    return poweroftwo;
}

void initGraphics() {
    if (initialized) return;
    
    disp_buffer = vram_base;
    draw_buffer = vram_base + PSP_LINE_SIZE * SCREEN_HEIGHT;
    
    sceDisplaySetMode(0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceDisplaySetFrameBuf(disp_buffer, PSP_LINE_SIZE, PSP_DISPLAY_PIXEL_FORMAT_8888, 1);
    
    initialized = 1;
}

void clearScreen(Color color) {
    if (!initialized) return;
    
    int x, y;
    Color* vram = getVramDrawBuffer();
    
    for (y = 0; y < SCREEN_HEIGHT; y++) {
        for (x = 0; x < SCREEN_WIDTH; x++) {
            vram[PSP_LINE_SIZE * y + x] = color;
        }
    }
}

void flipScreen() {
    if (!initialized) return;
    
    sceDisplayWaitVblankStart();
    
    // Swap buffers
    Color* temp = disp_buffer;
    disp_buffer = draw_buffer;
    draw_buffer = temp;
    
    sceDisplaySetFrameBuf(disp_buffer, PSP_LINE_SIZE, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
}

Color* getVramDrawBuffer() {
    return draw_buffer;
}

void putPixel(Color color, int x, int y) {
    if (!initialized || x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    Color* vram = getVramDrawBuffer();
    vram[PSP_LINE_SIZE * y + x] = color;
}

void drawRect(Color color, int x, int y, int w, int h) {
    if (!initialized) return;
    
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > SCREEN_WIDTH ? SCREEN_WIDTH : x + w;
    int y1 = y + h > SCREEN_HEIGHT ? SCREEN_HEIGHT : y + h;
    
    int i, j;
    Color* vram = getVramDrawBuffer();
    
    for (j = y0; j < y1; j++) {
        for (i = x0; i < x1; i++) {
            vram[PSP_LINE_SIZE * j + i] = color;
        }
    }
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
    // Ignore PNG warnings
}

Image* loadImage(const char* filename) {
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    FILE *fp;
    
    if ((fp = fopen(filename, "rb")) == NULL) return NULL;
    
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, user_warning_fn);
    if (!png_ptr) {
        fclose(fp);
        return NULL;
    }
    
    png_set_error_fn(png_ptr, NULL, NULL, user_warning_fn);
    
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return NULL;
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }
    
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, sig_read);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
    
    // Set up transformations
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
    if (bit_depth == 16) png_set_strip_16(png_ptr);
    if (bit_depth < 8) png_set_packing(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB) png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    
    png_read_update_info(png_ptr, info_ptr);
    
    // Allocate Image structure
    Image* image = (Image*)malloc(sizeof(Image));
    if (!image) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }
    
    image->imageWidth = width;
    image->imageHeight = height;
    image->textureWidth = power_of_two(width);
    image->textureHeight = power_of_two(height);
    
    // Allocate texture data (aligned)
    image->data = (Color*)memalign(16, image->textureWidth * image->textureHeight * sizeof(Color));
    if (!image->data) {
        free(image);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }
    
    // Clear texture
    memset(image->data, 0, image->textureWidth * image->textureHeight * sizeof(Color));
    
    // Read image data
    Color* line = (Color*)malloc(width * sizeof(Color));
    if (!line) {
        free(image->data);
        free(image);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }
    
    int y;
    for (y = 0; y < height; y++) {
        png_read_row(png_ptr, (u8*)line, NULL);
        
        // Convert RGBA to ABGR and copy to texture
        int x;
        for (x = 0; x < width; x++) {
            u32 color = line[x];
            image->data[y * image->textureWidth + x] = 
                ((color & 0xff) << 16) |          // R -> B
                (color & 0xff00) |                 // G stays
                ((color & 0xff0000) >> 16) |       // B -> R  
                (color & 0xff000000);              // A stays
        }
    }
    
    free(line);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    
    return image;
}

void freeImage(Image* image) {
    if (image) {
        if (image->data) free(image->data);
        free(image);
    }
}

void blitImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy) {
    if (!initialized || !source || !source->data) return;
    
    Color* vram = getVramDrawBuffer();
    int x, y;
    
    for (y = 0; y < height; y++) {
        if (sy + y >= source->imageHeight || dy + y >= SCREEN_HEIGHT) break;
        if (dy + y < 0) continue;
        
        for (x = 0; x < width; x++) {
            if (sx + x >= source->imageWidth || dx + x >= SCREEN_WIDTH) break;
            if (dx + x < 0) continue;
            
            vram[(dy + y) * PSP_LINE_SIZE + (dx + x)] = 
                source->data[(sy + y) * source->textureWidth + (sx + x)];
        }
    }
}

void blitAlphaImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy) {
    if (!initialized || !source || !source->data) return;
    
    Color* vram = getVramDrawBuffer();
    int x, y;
    
    for (y = 0; y < height; y++) {
        if (sy + y >= source->imageHeight || dy + y >= SCREEN_HEIGHT) break;
        if (dy + y < 0) continue;
        
        for (x = 0; x < width; x++) {
            if (sx + x >= source->imageWidth || dx + x >= SCREEN_WIDTH) break;
            if (dx + x < 0) continue;
            
            Color color = source->data[(sy + y) * source->textureWidth + (sx + x)];
            u8 alpha = color >> 24;
            
            if (alpha == 0) continue;  // Fully transparent
            if (alpha == 255) {        // Fully opaque
                vram[(dy + y) * PSP_LINE_SIZE + (dx + x)] = color;
            } else {
                // Simple alpha blending
                Color dst = vram[(dy + y) * PSP_LINE_SIZE + (dx + x)];
                u8 inv_alpha = 255 - alpha;
                
                u8 r = (((color & 0xff) * alpha) + ((dst & 0xff) * inv_alpha)) >> 8;
                u8 g = ((((color >> 8) & 0xff) * alpha) + (((dst >> 8) & 0xff) * inv_alpha)) >> 8;
                u8 b = ((((color >> 16) & 0xff) * alpha) + (((dst >> 16) & 0xff) * inv_alpha)) >> 8;
                
                vram[(dy + y) * PSP_LINE_SIZE + (dx + x)] = 0xff000000 | (b << 16) | (g << 8) | r;
            }
        }
    }
}