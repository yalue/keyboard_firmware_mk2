#include <stdint.h>
#include <string.h>
#include <Adafruit_NeoPixel.h>
#include "nrf.h"

// Pin D5
#define WAKEUP_PIN (5)

#define DEBOUNCE_MS (3)

#define NUM_KEYS (9)

// -- Create a NeoPixel object called onePixel that addresses 1 pixel in pin 8
Adafruit_NeoPixel onePixel = Adafruit_NeoPixel(1, 8, NEO_GRB + NEO_KHZ800);

/*
 * NOTES:
 * 
 *  GPIO 0 = Starts at 0x5...504 Goes to 0x5....77c
 *  GPIO 1 = Starts at 0x5...804 Goes to 0x5...107c
 *  
 *  See how pin I/O works at portable\packages\adafruit\hardware\nrf52\1.7.0\cores\nRF5\wiring_digital.c
 *  See the GPIO struct definition at portable\packages\adafruit\hardware\nrf52\1.7.0\cores\nRF5\nordic\nrfx\mdk\nrf52840.h
 *  
 *  Actual datasheet: https://cdn.sparkfun.com/assets/d/2/6/2/6/nRF52840_PS_v1.1.pdf
 *  
 *  Things to consider:
 *   - I should already have access to all of these random defines, right?? What to include to access it myself?
 *   - Look at other things that can be passed to pinMode in wiring_digital.c. Maybe I don't need to figure that part out
 *   - Can easily grab the full GPIO0 input or output bit set from the struct (GPIO0->IN)
 */

/* PINOUT (real, labeled, arduino number)
 *  P0.02: A4, 18
 *  P0.03: A5, 19
 *  P0.04: A0, 14
 *  P0.05: A1, 15
 *  P0.06: 11 (D11), 11
 *  P0.07: 6 (D6), 6
 *  P0.08: 12 (D12), 12
 *  P0.09: NFC1, 33
 *  P0.10: D2, 2 (also used for NFC)
 *  P0.11: SCL, 23
 *  P0.12: SDA, 22
 *  P0.13: MO (MOSI), 25
 *  P0.14: SCK, 26
 *  P0.15: MI (MISO), 24
 *  P0.16: Neopixel, 8
 *  P0.24: RX, 1
 *  P0.25: TX, 0
 *  P0.26: 9 (D9), 9
 *  P0.27: 10 (D10), 10
 *  P0.28: A3, 17
 *  P0.29: A6 (VDIV, LIPO voltage monitor), 20
 *  P0.30: A2, 16
 *  P0.31: AREF, 21
 *  P1.02: UserSw (SWITCH), 7
 *  P1.08: 5 (D5), 5
 *  P1.09: 13 (D13), 13
 *  P1.10: Conn (blue LED), 4
 *  P1.15: D3 (red LED), 3
 */

typedef struct {
  // 0 if key i is released, nonzero if pressed.
  int key_state[NUM_KEYS];
  // The time at which we expect key i to stop bouncing.
  unsigned long debounce_done[NUM_KEYS];
  // Updated with millis() at the start of each loop.
  unsigned long current_ms;
  int blue_led_blink_counter;
} GlobalState;

// Current flows from columns to rows.

// Pins A0, A1, A2
static constexpr uint8_t row_pins[3] = {14, 15, 16};

// Pins A3, A4, A5
static constexpr uint8_t col_pins[3] = {17, 18, 19};

GlobalState s;

void setup()  {
  int i;
  onePixel.begin();             // Start the NeoPixel object
  onePixel.clear();             // Set NeoPixel color to black (0,0,0)
  onePixel.setBrightness(20);   // Affects all subsequent settings
  onePixel.show();              // Update the pixel state
  memset(&s, 0, sizeof(s));

  //pinMode(4, OUTPUT);
  // Direction = output (1), disconnect input buffer = disconnected (1), all others 0
  NRF_P1->PIN_CNF[10] = 3;

  // Columns are outputs
  for (i = 0; i < 3; i++) {
    pinMode(col_pins[i], OUTPUT);
    digitalWrite(col_pins[i], LOW);
  }

  // Rows are inputs
  for (i = 0; i < 3; i++) {
    pinMode(row_pins[i], INPUT_PULLDOWN);
  }
}

static void HandleKeyPressed(int key_index) {
  s.key_state[key_index] = 1;
  s.debounce_done[key_index] = s.current_ms + DEBOUNCE_MS;
}

static void HandleKeyReleased(int key_index) {
  s.key_state[key_index] = 0;
  s.debounce_done[key_index] = s.current_ms + DEBOUNCE_MS;
}

static void UpdateKeyState(int row, int col, int pressed) {
  int index = row * 3 + col;
  if (index >= NUM_KEYS) return;
  // Don't do anything yet if we're not done debouncing this key.
  if (s.current_ms < s.debounce_done[index]) return;
  // Quit now if the key state hasn't changed.
  if (s.key_state[index] == pressed) return;
  // The key state has changed, react accordingly.
  if (pressed) {
    HandleKeyPressed(index);
  } else {
    HandleKeyReleased(index);
  }
}

static void UpdateAllKeys(void) {
  int pressed, row, col;
  for (col = 0; col < 3; col++) {
    digitalWrite(col_pins[col], HIGH);
    for (row = 0; row < 3; row++) {
      pressed = digitalRead(row_pins[row]) == HIGH;
      UpdateKeyState(row, col, pressed);
    }
    digitalWrite(col_pins[col], LOW);
  }
}

// Updates s.current_ms, and handles wraparound if it occurred.
static void UpdateTimer(void) {
  int i;
  unsigned long new_ms = millis();
  if (new_ms < s.current_ms) {
    // Timer wraparound. Clear debounce counters to avoid being stuck forever.
    // Setting to 0 may lead to spurious presses, but only when the timer
    // wraps around (which practically should be never).
    for (i = 0; i < NUM_KEYS; i++) {
      s.debounce_done[i] = 0;
    }
  }
  s.current_ms = new_ms;
}

static int IsKeyPressed(int row, int col) {
  return s.key_state[row * 3 + col];
}

void loop()  {
  int i, j, rgb_color[3];
  UpdateTimer();
  UpdateAllKeys();

  // Test: LED RGB channel intensity determined by highest key pressed on each column.
  for (i = 0; i < 3; i++) {
    rgb_color[i] = 0;
    for (j = 0; j < 3; j++) {
      if (IsKeyPressed(j, i)) {
        rgb_color[i] = (j + 1) * 85;
      }
    }
  }
  onePixel.setPixelColor(0, rgb_color[0], rgb_color[1], rgb_color[2]);
  onePixel.show();

  if (s.blue_led_blink_counter == 0) {
    //digitalWrite(4, HIGH);
    NRF_P1->OUTSET = (1 << 10);
  } else if (s.blue_led_blink_counter == 1000) {
    //digitalWrite(4, LOW);
    NRF_P1->OUTCLR = (1 << 10);
  }
  s.blue_led_blink_counter++;
  if (s.blue_led_blink_counter >= 2000) {
    s.blue_led_blink_counter = 0;
  }
  delay(1);
}
