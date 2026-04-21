#include <stdint.h>
#include <string.h>
#include <Adafruit_NeoPixel.h>

// -- Create a NeoPixel object called onePixel that addresses 1 pixel in pin 8
Adafruit_NeoPixel onePixel = Adafruit_NeoPixel(1, 8, NEO_GRB + NEO_KHZ800);

typedef struct {
  uint16_t rgb_counter;
} GlobalState;

GlobalState s;

void setup()  {
  onePixel.begin();             // Start the NeoPixel object
  onePixel.clear();             // Set NeoPixel color to black (0,0,0)
  onePixel.setBrightness(20);   // Affects all subsequent settings
  onePixel.show();              // Update the pixel state
  memset(&s, 0, sizeof(s));
}

void loop()  {
  s.rgb_counter++;
  // Between 0 and 128 to make it a bit less blinding
  int r = (s.rgb_counter & 0xf) << 3;
  int g = (s.rgb_counter & 0xf0) >> 1;
  // b is full range though!
  int b = (s.rgb_counter & 0xf00) >> 4;
  onePixel.setPixelColor(0, r, g, b);   //  Set pixel 0 to (r,g,b) color value
  onePixel.show();
  if (s.rgb_counter >= 0xfff) {
    onePixel.clear();
    onePixel.show();
    s.rgb_counter = 0;
    delay(1000);
  } else {
    delay(1);
  }
}
