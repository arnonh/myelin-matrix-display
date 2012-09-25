#include "matrix.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static int udp_fd;

void setup_display() {
  // prepare UDP socket
  udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!udp_fd) {
    printf("setup_display(): unable to create socket\n");
    exit(1);
  }
}

void send_to_display(unsigned char* buffer, int buffer_len) {
  int expected_len = BUF_SIZE / OVERSAMPLING / OVERSAMPLING;
  if (buffer_len != expected_len) {
    printf("send_to_display(): incorrect buffer length %d; expected %d\n", buffer_len, expected_len);
    exit(1);
  }

  // reformat buffer
  unsigned char output[expected_len + 1];
  output[0] = 1; // protocol header
  memcpy(output + 1, buffer, expected_len); // ADDRESSING_HORIZONTAL_NORMAL

  // set output address
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(58082); // send on port 58082 - Disorient protocol

  // send udp frame to emulator on localhost
  inet_aton("127.0.0.1", &addr.sin_addr); // localhost
  sendto(udp_fd, output, expected_len + 1, 0, (const struct sockaddr *)&addr, sizeof(addr));

  // and to laptop
  inet_aton("192.168.1.78", &addr.sin_addr); // localhost
  sendto(udp_fd, output, expected_len + 1, 0, (const struct sockaddr *)&addr, sizeof(addr));

  // and to want it! matrix
  inet_aton("192.168.1.99", &addr.sin_addr); // localhost
  sendto(udp_fd, output, expected_len + 1, 0, (const struct sockaddr *)&addr, sizeof(addr));
}

// implemented by the particular animation we're compiling
extern void setup_animation();
extern void draw_frame(int frame);

static unsigned char draw_buf[BUF_SIZE];

int frame_rate = 30;

void set_frame_rate(int rate) {
  frame_rate = rate;
}

void dim(int factor) {
  for (unsigned char* ptr = draw_buf; ptr < draw_buf + BUF_SIZE; ++ptr) {
    *ptr = *ptr * factor / 256;
  }
}

unsigned char* ptr_for_point(int x, int y) {
  int pos = (RGB ? 3 : 0) * (y * WIDTH + x);
  if (pos > BUF_SIZE) return NULL;
  unsigned char* ptr = draw_buf + pos;
  if (ptr < draw_buf || ptr > draw_buf + BUF_SIZE) return NULL;
  return ptr;
}

void point(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
  unsigned char* ptr = ptr_for_point(x, y);
  if (ptr == NULL) return;
  if (RGB) {
    *ptr++ = r;
    *ptr++ = g;
    *ptr++ = b;
  } else {
    *ptr++ = (r + g + b) / 3;
  }
}

void point(int x, int y, color_t c) {
  point(x, y, (unsigned char)(c >> 16), (unsigned char)(c >> 8), (unsigned char)c);
}

void point_clip(int x, int y, color_t c) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
    point(x, y, c);
  }
}

color_t getpoint(int x, int y) {
  unsigned char* ptr = ptr_for_point(x, y);
  if (ptr == NULL) return 0; // everything off-canvas is black?
  return color(*ptr++, *ptr++, *ptr++);
}

void line(int x0, int y0, int x1, int y1, color_t c) {
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

void rect(int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b) {
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      point(x, y, r, g, b);
    }
  }
}

void rect(int x0, int y0, int x1, int y1, color_t c) {
  rect(x0, y0, x1, y1, (unsigned char)(c >> 16), (unsigned char)(c >> 8), (unsigned char)c);
}

// from wikipedia, distributable under cc-a-sa
void circle(int cx, int cy, int r, color_t c) {
  int f = 1 - r;
  int ddF_x = 1;
  int ddF_y = -2 * r;
  int x = 0;
  int y = r;

  point_clip(cx, cy + r, c);
  point_clip(cx, cy - r, c);
  point_clip(cx + r, cy, c);
  point_clip(cx - r, cy, c);

  while(x < y) {
    // ddF_x == 2 * x + 1;
    // ddF_y == -2 * y;
    // f == x*x + y*y - r*r + 2*x - y + 1;
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    point_clip(cx + x, cy + y, c);
    point_clip(cx - x, cy + y, c);
    point_clip(cx + x, cy - y, c);
    point_clip(cx - x, cy - y, c);
    point_clip(cx + y, cy + x, c);
    point_clip(cx - y, cy + x, c);
    point_clip(cx + y, cy - x, c);
    point_clip(cx - y, cy - x, c);
  }
}

int randint(int min_inclusive, int max_inclusive) {
  return rand() % (max_inclusive - min_inclusive + 1) + min_inclusive;
}

color_t random_color() {
  return rand() & 0xFFFFFF;
}

color_t color_fade(color_t color1, color_t color2, int pos) {
  int r1 = (color1 >> 16) & 0xFF, g1 = (color1 >> 8) & 0xFF, b1 = color1 & 0xFF;
  int r2 = (color2 >> 16) & 0xFF, g2 = (color2 >> 8) & 0xFF, b2 = color2 & 0xFF;

  int antipos = 255 - pos;
  int r = (r1 * antipos + r2 * pos) / 510, g = (g1 * antipos + g2 * pos) / 510, b = (b1 * antipos + b2 * pos) / 510;

  return (r << 16) | (g << 8) | b;
}

/* wheel(0-767) generates an approximate rainbow.  Based on Wheel()
 * from the Adafruit_WS2801 library, which requires the following
 * text in all distributions:

  Example sketch for driving Adafruit WS2801 pixels!

  Designed specifically to work with the Adafruit RGB Pixels!
  12mm Bullet shape ----> https://www.adafruit.com/products/322
  12mm Flat shape   ----> https://www.adafruit.com/products/738
  36mm Square shape ----> https://www.adafruit.com/products/683

  These pixels use SPI to transmit the color data, and have built in
  high speed PWM drivers for 24 bit color per pixel
  2 pins are required to interface

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution

*/

color_t wheel(uint16_t pos) {
  if (pos < 256) {
    return color(255 - pos, pos, 0);
  } else if (pos < 512) {
    pos -= 256;
    return color(0, 255 - pos, pos);
  } else {
    pos -= 512;
    return color(pos, 0, 255 - pos);
  }
}

void blank() {
  bzero(draw_buf, sizeof(draw_buf));
}

int main() {
  sranddev();
  setup_display();
  setup_animation();
  for (int frame = 0; ; ++frame) {
    draw_frame(frame);
    printf("sending frame %d to display\n", frame);
#if OVERSAMPLING > 1
#define OUT_W (WIDTH / OVERSAMPLING)
#define OUT_H (HEIGHT / OVERSAMPLING)
    unsigned char downsampled_buf[OUT_W * OUT_H * 3];
    for (int y = 0; y < OUT_H; ++y) {
      for (int x = 0; x < OUT_W; ++x) {
	// average OVERSAMPLING x OVERSAMPLING block of pixels from draw_buf into one pixel in downsampled_buf
	int r = 0, g = 0, b = 0;
	for (int yd = 0; yd < OVERSAMPLING; ++yd) {
	  for (int xd = 0; xd < OVERSAMPLING; ++xd) {
	    color_t c = getpoint(x * OVERSAMPLING + xd, y * OVERSAMPLING + yd);
	    r += (c >> 16) & 0xFF;
	    g += (c >> 8) & 0xFF;
	    b += c & 0xFF;
	  }
	}
	unsigned char* ptr = downsampled_buf + (y * OUT_W + x) * 3;
	*ptr++ = r / OVERSAMPLING / OVERSAMPLING;
	*ptr++ = g / OVERSAMPLING / OVERSAMPLING;
	*ptr++ = b / OVERSAMPLING / OVERSAMPLING;
      }
    }
    send_to_display(downsampled_buf, sizeof(downsampled_buf));
#else
    send_to_display(draw_buf, sizeof(draw_buf));
#endif
    usleep(1000000 / frame_rate); //TODO: take into account generate/send time
  }
}
