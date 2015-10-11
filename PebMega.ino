#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include <ArduinoPebbleSerial.h>

// pw-dos asset
#include "default_assets.h"


static const uint16_t SERVICE_ID = 0x1001;
static const uint16_t LED_ATTRIBUTE_ID = 0x0001;
static const size_t LED_ATTRIBUTE_LENGTH = 1;
static const uint16_t UPTIME_ATTRIBUTE_ID = 0x0002;
static const size_t UPTIME_ATTRIBUTE_LENGTH = 4;

static const uint16_t SERVICES[] = {SERVICE_ID};
static const uint8_t NUM_SERVICES = 1;

static uint8_t buffer[20];


// note that due to rotation, width and height are actually inverted!
// (names are preserved to make expected parameters less confusing)
static uint16_t width = PW_DOS_HEIGHT;
// clipped slightly from 144 due to 272px width
static uint16_t height = PW_DOS_WIDTH - 8;

static byte red = 0;
static byte green = 0;
static byte blue = 0;


void setup() {
  
  // set up software serial connection to Pebble
  const uint8_t PEBBLE_DATA_PIN = 3; //2;
  ArduinoPebbleSerial::begin_software(PEBBLE_DATA_PIN, buffer, sizeof(buffer), Baud57600, SERVICES,
                                      NUM_SERVICES);
  
  GD.begin();
  LOAD_ASSETS();
}

static void prv_handle_uptime_request(RequestType type, size_t length) {
  if (type != RequestTypeRead) {
    // unexpected request type
    return;
  }
  // write back the current uptime
  const uint32_t uptime = millis() / 1000;
  ArduinoPebbleSerial::write(true, (uint8_t *)&uptime, sizeof(uptime));
}

static void prv_handle_led_request(RequestType type, size_t length) {
  if (type != RequestTypeWrite) {
    // unexpected request type
    //Serial.print("unexpected RequestType");
    return;
  } else if (length != LED_ATTRIBUTE_LENGTH) {
    // unexpected request length
    //Serial.print("unexpected length");
    return;
  }
  
  
  // can't since D13 is SCK for Gameduino 2...but we'll use this later!
  /*
  // set the LED
  digitalWrite(LED_BUILTIN, (bool) buffer[0]);
  */
  
  
  // up = increase background redness, down = blueness
  if ((bool) buffer[0]) {
    red += 16;
  } else {
    blue += 16;    
  }
  
  // ACK that the write request was received
  ArduinoPebbleSerial::write(true, NULL, 0);
  //Serial.print//Serial.print("digitalWrite():");
  //Serial.println((bool) buffer[0]);
  //Serial.println("write(true, NULL, 0)");
}

void loop() {
  // background: Pebble GColorDarkCandyAppleRed
  GD.ClearColorRGB(red, green, blue);
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
 
   if (ArduinoPebbleSerial::is_connected()) {
    //Serial.println("IS_connected()");
    static uint32_t last_notify_time = 0;
    const uint32_t current_time = millis() / 1000;
    if (current_time > last_notify_time) {
      ArduinoPebbleSerial::notify(SERVICE_ID, UPTIME_ATTRIBUTE_ID);
      //Serial.println("notify()");
      last_notify_time = current_time;
    }
  } else {
    //Serial.println("!is_connected()");
    digitalWrite(LED_BUILTIN, 1);
  }

  uint16_t service_id;
  uint16_t attribute_id;
  size_t length;
  RequestType type;
  if (ArduinoPebbleSerial::feed(&service_id, &attribute_id, &length, &type)) {
    //Serial.println("feed()");
    // process the request
    if (service_id == SERVICE_ID) {
      //Serial.println("correct SERVICE_ID");
      switch (attribute_id) {
        case UPTIME_ATTRIBUTE_ID:
          //Serial.println("UPTIME_ATTRIBUTE_ID");
          prv_handle_uptime_request(type, length);
          break;
        case LED_ATTRIBUTE_ID:
          //Serial.println("LED_ATTRIBUTE_ID");
          prv_handle_led_request(type, length);
          break;
        default:
          //Serial.println("unknown attribute_id");
          break;
      }
    }
  }
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
