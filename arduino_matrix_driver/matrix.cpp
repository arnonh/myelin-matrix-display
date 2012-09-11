#include "matrix.h"

// ugly hack to let use repurpose code from acw_c_client
#define draw_buf strip.pixels

void dim(int factor) {
  for (unsigned char* ptr = draw_buf; ptr < draw_buf + BUF_SIZE; ++ptr) {
    *ptr = *ptr * factor / 256;
  }
}

void point(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
#ifdef HORIZONTAL_ADDRESSING
  // monolithic matrix display (oct 2012) - snaking horizontally from top left corner.  original format, used at burning man 2012.
  int pos = (y * WIDTH + (y & 1 ? (WIDTH - 1 - x) : x)) * 3;
#elif defined VERTICAL_ADDRESSING
  // modular matrix display (sep 2012) - snaking vertically from top left corner, so there's only one connection between each pair of panels, rather than 12.
  int pos = ((x & 1) ? ((x + 1) * HEIGHT - 1 - y) : (x * HEIGHT + y)) * 3;
#endif
  if (pos < 0 || pos > BUF_SIZE) return;

  unsigned char* ptr = draw_buf + pos;
  if (ptr < draw_buf || ptr > draw_buf + BUF_SIZE) return;
  if (RGB) {
    *ptr++ = r;
    *ptr++ = g;
    *ptr++ = b;
  } else {
    *ptr++ = (r + g + b) / 3;
  }
}

void point(int x, int y, uint32_t c) {
  point(x, y, (unsigned char)(c >> 16), (unsigned char)(c >> 8), (unsigned char)c);
}

uint32_t random_color() {
  return random(0, 0xFFFFFF);
}

void line(int x0, int y0, int x1, int y1, uint32_t c) {
  if (x0 == x1) {
    // vertical
    for (int y = min(y0, y1); y < max(y0, y1); ++y) point(x0, y, c);
  } else if (y0 == y1) {
    // horizontal
    for (int x = min(x0, x1); x < max(x0, x1); ++x) point(x, y0, c);
  } else if ((y1 - y0) > (x1 - x0)) {
    // more vertical than horizontal
    for (int y = y0; y < y1; ++y) point(x0 + (y - y0) * (x1 - x0) / (y1 - y0), y, c);
  } else {
    // more horizontal than vertical
    for (int x = x0; x < x1; ++x) point(x, y0 + (x - x0) * (y1 - y0) / (x1 - x0), c);
  }
}

void rect(int x0, int y0, int x1, int y1, uint32_t c) {
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x)
      point(x, y, c);
}

