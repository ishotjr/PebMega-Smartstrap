#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

// pw-dos asset
#include "default_assets.h"

// note that due to rotation, width and height are actually inverted!
// (names are preserved to make expected parameters less confusing)
static uint16_t width = PW_DOS_HEIGHT;
// clipped slightly from 144 due to 272px width
static uint16_t height = PW_DOS_WIDTH - 8;

void setup() {
  GD.begin();
  LOAD_ASSETS();
}

void loop() {
  // background: Pebble GColorDarkCandyAppleRed
  GD.ClearColorRGB(0xAA0000);
  GD.Clear();
  
  // transparency off
  GD.BlendFunc(SRC_ALPHA, ZERO);

  // rotate and scale 2x
  GD.Begin(BITMAPS);
  // this is intentionally width for both parameters in order to rotate 
  // around center of "square"
  rotate_and_scale(DEGREES(270), width / 2, width / 2, F16(2.0));
  GD.cmd_setmatrix();
  // TODO: auto-center (vs. hard-coded based on known size/scale)
  GD.Vertex2ii((480 / 2) - ((width * 2) / 2), (272 / 2) - ((height * 2) / 2));
  GD.swap();
 
}

// Apply a rotation around pixel (x, y) and scale (or use 0 for no rotation/scaling)
// *note that scale is pre F16'd ( * 65536) to avoid floats
static void rotate_and_scale(uint16_t angle, uint16_t x, uint16_t y, int32_t scale) {
  // turns out scaling then rotating actually works out better :D
  
  // ignore if 0
  if (scale > 0) {
    // increase bitmap size as required
    // TODO: incorporate screen boundaries etc.?    
    // TODO: poss to "measure" bitmap vs. hard-coded values?
    GD.BitmapSize(NEAREST, BORDER, BORDER, (width * scale) / 65536L, 
      (height * scale) / 65536L);        
    GD.cmd_scale(scale, scale);    
  }
  
  // ignore if 0
  if (angle > 0) {
    GD.cmd_translate(F16(x), F16(y));
    GD.cmd_rotate(angle);
    // temporary offset to allow left side of screen to be visible despite being 8px short
    GD.cmd_translate(F16(-1 * x + 32), F16(-1 * y));
    // GD.cmd_translate(F16(-1 * x), F16(-1 * y));
  }
}
