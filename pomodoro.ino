#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <Adafruit_NeoPixel.h>

#define PIN 8

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, PIN, NEO_GRB + NEO_KHZ800);

const uint32_t remaining = strip.Color(0, 64, 0);
const uint32_t elapsed = strip.Color(64, 0, 0);
const uint32_t flash = strip.Color(255, 255, 255);
const uint32_t pauseColor = strip.Color(0, 0, 64);

const uint64_t max_time_millis = 25 * 60 * 1000L;

uint64_t start_time_millis = 0L;

bool paused = false;
bool blink = false;
uint64_t blinkDelay = 1000L;


// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

const int buttonCount = 4;
int buttonPins[buttonCount] = {10, 9, 12, 11};
int buttonCurrent[buttonCount] = {0};
int buttonPrevious[buttonCount] = {0};
int buttonDelta[buttonCount] = {0};

void updateButtons() {
  for (int i = 0; i < buttonCount; i++) {
    buttonPrevious[i] = buttonCurrent[i];
    buttonCurrent[i] = digitalRead(buttonPins[i]);
    buttonDelta[i] = buttonCurrent[i] - buttonPrevious[i];
  }
}

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  // See http://www.engblaze.com/microcontroller-tutorial-avr-and-arduino-timer-interrupts/
  // Disable interrupts.
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 15624;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);
  // Enble interrupts.
  sei();

  for (int i = 0; i < buttonCount; i++) {
    pinMode(buttonPins[i], INPUT);
  }
  updateButtons();

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  start_time_millis = millis();
}

ISR(TIMER1_COMPA_vect) {
  // TODO.
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<1000; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

void loop1() {
  strip.clear();
  /*strip.setPixelColor(0, (digitalRead(button1) == HIGH) ? remaining : elapsed);*/
  /*strip.setPixelColor(1, (digitalRead(button2) == HIGH) ? remaining : elapsed);*/
  /*strip.setPixelColor(2, (digitalRead(button3) == HIGH) ? remaining : elapsed);*/
  /*strip.setPixelColor(3, (digitalRead(button4) == HIGH) ? remaining : elapsed);*/
  strip.show();
}

void allLeds(uint32_t color) {
  for(int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
}

void loop() {
  updateButtons();

  uint64_t time_millis = millis();

  if (buttonDelta[0] > 0) {
    start_time_millis = time_millis;
  }

  if (buttonDelta[1] > 0) {
    paused = !paused;
  }

  if (paused) {
    strip.clear();

    blink = !blink;
    if (blink) {
      allLeds(pauseColor);
    }

    strip.show();

    delay(blinkDelay);
    return;
  }

  strip.clear();
  for(int i = 0; i < strip.numPixels(); i++) {
    int threshold = strip.numPixels() * (time_millis - start_time_millis) / max_time_millis;
    uint32_t color = (i < threshold) ? elapsed : remaining;
    strip.setPixelColor(i, color);
  }
  strip.show();

  if (time_millis > max_time_millis) {
    theaterChase(flash, 50);
    start_time_millis = time_millis;
  }

  delay(100);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}


//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

