#include "matrix.h"

void setup_animation() {
  set_frame_rate(30);
}

void draw_frame(int frame) {
  static int pos = 0;

  rect(0, 0, WIDTH, HEIGHT, wheel(pos));

  pos = (pos + 1) % (256 * 3);
}
