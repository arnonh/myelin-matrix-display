/**
 * Embedded slave lighting node with WS2801 and nRF24L01+
 * Copyright (C) 2012 Phillip Pearson
 * You may use this under a CC Attribution license
 */

#include <Adafruit_WS2801.h>
#include <EEPROM.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// in-project includes
#include "commands.h"
#include "printf.h"
#include "ws2801.h"

// we're going to try to push this many frames per second to the LEDs
#define FRAME_RATE 50
#define MILLIS_PER_FRAME (1000/FRAME_RATE)

// All my boards have an LED on pin 13 
#define LED 13

// nRF24L01+ radio; uses RF24 lib and hardware SPI pins plus CE on D8 and CSN on D10
RF24 radio(8, 10);

// address of the master station
#define MASTER_ADDRESS 0xF0F0F0F0E1LL

// GRAPHIC DATA

// current mode
mode_t current_mode = (mode_t)0;

// when the last frame was shown (actually when the last frame *should* have been shown -- we attempt to keep to the specified frame rate)
unsigned long last_frame_time = 0;

// each mode will cast this to its own *_payload_t
union {
  uint8_t bits[30];
  fade_mode_payload_t fade;
  strobe_mode_payload_t strobe;
} mode_params;

// if we've just received a "slave" type command (0, 1, 2-0, 2-1), hold off on free running for this many more frames
uint8_t free_running_disable_countdown = 0;

// buffer for incomplete received LED data
unsigned char backbuffer[BUFFER_SIZE];

// true if a rendered frame is in the backbuffer, waiting for a sync tick to be sent to the LEDs
bool rendered_frame_in_backbuffer = false;

// mode-defined
union {
  uint32_t bits;
  fade_mode_state_t fade;
  strobe_mode_state_t strobe;
  chase_mode_state_t chase;
} mode_state;

void setup(void)
{
  pinMode(LED, OUTPUT);

  // read radio address from eeprom (you must first set this using the set_mac_address sketch)
  uint64_t rf_address = 0;
  for (uint8_t b = 0; b < 5; ++b) {
    rf_address <<= 8;
    rf_address |= EEPROM.read(b);
  }

  ws2801_setup();

  Serial.begin(115200);
  printf_begin();

  radio.begin();
  radio.setRetries(15, 15);
  radio.openWritingPipe(MASTER_ADDRESS); // anything we send goes to the master
  radio.openReadingPipe(1, rf_address); // we listen on our own address
  radio.startListening();

  radio.printDetails();

  /*
  // test fade mode
  current_mode = MODE_FADE;
  mode_params.fade.n_fades = 2;
  mode_params.fade.fades[0].start.r = 0;
  mode_params.fade.fades[0].start.g = 255;
  mode_params.fade.fades[0].start.b = 0;
  mode_params.fade.fades[0].finish.r = 255;
  mode_params.fade.fades[0].finish.g = 0;
  mode_params.fade.fades[0].finish.b = 255;
  mode_params.fade.fades[0].speed = 1;
  mode_params.fade.fades[1].start.r = 255;
  mode_params.fade.fades[1].start.g = 0;
  mode_params.fade.fades[1].start.b = 255;
  mode_params.fade.fades[1].finish.r = 0;
  mode_params.fade.fades[1].finish.g = 255;
  mode_params.fade.fades[1].finish.b = 0;
  mode_params.fade.fades[1].speed = 2;
  */
  
  /*  
  // test strobe mode
  current_mode = MODE_STROBE;
  mode_params.strobe.n_colors = 4;
  mode_params.strobe.colors[0].r = 0;
  mode_params.strobe.colors[0].g = 0;
  mode_params.strobe.colors[0].b = 0;
  mode_params.strobe.colors[1].r = 255;
  mode_params.strobe.colors[1].g = 0;
  mode_params.strobe.colors[1].b = 0;
  mode_params.strobe.colors[2].r = 0;
  mode_params.strobe.colors[2].g = 255;
  mode_params.strobe.colors[2].b = 0;
  mode_params.strobe.colors[3].r = 0;
  mode_params.strobe.colors[3].g = 0;
  mode_params.strobe.colors[3].b = 255;
  */
  
  // test chase mode
  //current_mode = MODE_CHASE;
  
  // test rainbow mode
  current_mode = MODE_RAINBOW;
}

// do something weird so we know something is broken
static void fail() {
  for (ws2801_address_t pos = 0; pos < PIXEL_COUNT; ++pos) {
    ws2801_point(pos, 255, 0, 0);
  }
  ws2801_show();
}

// from Adafruit's WS2801 strip test code
// Create a 24 bit color value from R,G,B                                                                                                                                           
color_t Color(uint8_t r, uint8_t g, uint8_t b)
{
  color_t c;
  c.r = r;
  c.g = g;
  c.b = b;
  return c;
}

// from Adafruit's WS2801 strip test code
//Input a value 0 to 255 to get a color value.                                                                                                                                      
//The colours are a transition r - g -b - back to r                                                                                                                                 
color_t Wheel(uint8_t WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 0, 255 - WheelPos * 3);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, WheelPos * 3, 0);
  } else {
   WheelPos -= 170;
   return Color(0, 255 - WheelPos * 3, WheelPos * 3);
  }
}


static bool mode_fade() {
#define state mode_state.fade
#define payload mode_params.fade
#define current_fade (payload.fades[state.n_fade])

  color_t c;
  c.r = current_fade.start.r + (current_fade.finish.r - current_fade.start.r) * state.fade_pos / 256;
  c.g = current_fade.start.g + (current_fade.finish.g - current_fade.start.g) * state.fade_pos / 256;
  c.b = current_fade.start.b + (current_fade.finish.b - current_fade.start.b) * state.fade_pos / 256;
  ws2801_line(0, PIXEL_COUNT, c);

  // advance!
  if (state.fade_pos + current_fade.speed < 256) {
    state.fade_pos += current_fade.speed;
  } else {
    state.fade_pos = 0;
    if (++state.n_fade >= payload.n_fades) {
      state.n_fade = 0;
    }
  }
  
  return true;

#undef state
#undef payload
#undef current_fade
}

static bool mode_strobe() {
#define state mode_state.strobe
#define payload mode_params.strobe

  ws2801_line(0, PIXEL_COUNT, payload.colors[state.n_color]);

  // advance!
  state.n_color = (state.n_color + 1) % payload.n_colors;
  
  return true;

#undef state
#undef payload
}

static bool mode_chase() {
#define state mode_state.chase
#define payload mode_params.chase

 ws2801_point(state.pos, 0, 0, 0);
 state.pos = (state.pos + 1) % PIXEL_COUNT;
 ws2801_point(state.pos, random(0, 255), random(0, 255), random(0, 255));
 
 return true;
 
#undef state
#undef payload
}

static bool mode_rainbow() {
#define state mode_state.chase
#define payload mode_params.chase

 for (uint8_t p = 0; p < PIXEL_COUNT; ++p) {
   ws2801_point(p, Wheel((p + state.pos) % 256));
 }
 state.pos = (state.pos + 1) & 0xFF;
 
 return true;
 
#undef state
#undef payload
}

// render a frame in the current mode.  return true if a frame has been rendered into the backbuffer, or false if not.
static bool execute_mode() {
  switch (current_mode) {
    case MODE_FADE: return mode_fade();
    case MODE_STROBE: return mode_strobe();
    case MODE_CHASE: return mode_chase();
    case MODE_RAINBOW: return mode_rainbow();
  }
  return true;
}

void loop(void)
{
  if ( radio.available() )
  {
    // read a 32-byte packet
    uint8_t data[32];
    radio.read(data, sizeof(data));

    printf("received packet, command %d bank %d\r\n", data[0], data[1]);
    // 32-byte packet format:
    // data[0..1] = command
    // data[2..31] = payload
    switch (data[0]) {
      case 0: // command 0, x: copy data[2..31] into backbuffer + data[1] * 30
        ws2801_blit(data[1] * 10, data + 2, 10);
        break;
      case 1: // command 1, x: copy data[2..31] into backbuffer + data[1] * 30 and send buffer to LEDs
        ws2801_blit(data[1] * 10, data + 2, 10);
        ws2801_show();
        goto disable_free_running;
      case 2:
        switch (data[1]) {
        case 0: // command 2, 0: repeat data[2..31] over backbuffer and send buffer to LEDs
          for (ws2801_address_t block = 0; block < PIXEL_COUNT; block += 10) {
            ws2801_blit(block, data + 2, 10);
          }
          ws2801_show();
          goto disable_free_running;
        case 1: // command 2, 1: stretch data[2..31] over backbuffer (so data[2..4] sets the colour for LEDs 0-9, data[5..7] sets LEDs 10-19, etc)
          for (ws2801_address_t pos = 0; pos < PIXEL_COUNT; ++pos) {
            uint8_t* src = data + 2 + (pos * 10 / PIXEL_COUNT) * PIXEL_SIZE;
            ws2801_point(pos, src[0], src[1], src[2]);
          }
          ws2801_show();
          goto disable_free_running;
        default:
          goto invalid_command;
        }
        break;
      case 3: // MODE SELECT
        current_mode = (mode_t)data[1];
        memcpy(mode_params.bits, data + 2, 30);
        mode_state.bits = 0;
        goto enable_free_running;
      default:
        goto invalid_command;
    }
    goto command_done;

enable_free_running:
    free_running_disable_countdown = 0;
    goto command_done;

disable_free_running:
    free_running_disable_countdown = 100; //FRAME_RATE / 2; // half a second
    goto command_done;

invalid_command:
    // invalid commands cause all LEDs to go red, so we can tell when something is going wrong.
    fail();
    goto command_done;

command_done:
    ;
  } else {
    // prioritize getting data from the radio, but if there isn't anything, switch into free running mode
    long now = millis();
    if (now - last_frame_time < MILLIS_PER_FRAME) {
      // wait until it's time to show the next frame
    } else {
      if (free_running_disable_countdown) {
        Serial.println("no FR");
        --free_running_disable_countdown;
      } else {
        if (rendered_frame_in_backbuffer) {
          ws2801_show();
          Serial.print("Sent frame - ms=");
          Serial.println((int)(now - last_frame_time), DEC);
        } else {
          Serial.print("No frame - ms=");
          Serial.println((int)(now - last_frame_time), DEC);
        }
        rendered_frame_in_backbuffer = execute_mode();
        last_frame_time += MILLIS_PER_FRAME;
        if (now - last_frame_time > MILLIS_PER_FRAME) {
          // failed to keep in sync, so let's just cut our losses and 
          last_frame_time = now;
        }
      }
    }
  }
}
