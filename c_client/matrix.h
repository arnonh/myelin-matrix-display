#ifndef __MATRIX_H
#define __MATRIX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

// arduino compatibility
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

// want it! matrix
#define WIDTH 25
#define HEIGHT 12
#define RGB 1
extern int frame_rate;

#define PIXEL_COUNT (WIDTH * HEIGHT)
#define BUF_SIZE (PIXEL_COUNT * (RGB ? 3 : 1))

typedef uint32_t color_t;
inline color_t color(uint8_t r, uint8_t g, uint8_t b) {
  return (r << 16) | (g << 8) | b;
}

extern void blank();
extern void dim(int factor);
extern void point(int x, int y, unsigned char r, unsigned char g, unsigned char b);
extern void point(int x, int y, color_t c);
extern void line(int x0, int y0, int x1, int y1, color_t c);
extern void rect(int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b);
extern void rect(int x0, int y0, int x1, int y1, color_t c);
extern color_t random_color();
extern color_t color_fade(color_t color1, color_t color2, int pos);
extern void set_frame_rate(int rate);
extern color_t wheel(uint16_t pos);

#endif // __MATRIX_H
