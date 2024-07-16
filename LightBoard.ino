#include <FastLED.h>
#include <RotaryEncoder.h>

#define LED_PIN     6    // Digital pin connected to the LED data line
#define POSITION_ROTARY_PIN_1     8
#define POSITION_ROTARY_PIN_2     9    
#define SHIFT_ROTARY_PIN_1     10
#define SHIFT_ROTARY_PIN_2     11
#define COLOR_ROTARY_PIN_1     2
#define COLOR_ROTARY_PIN_2     3

#define NUM_CIRCLES_HEIGHT 2
#define NUM_CIRCLES_WIDTH 3
#define NUM_LEDS_CIRCLE 20
#define NUM_LEDS NUM_CIRCLES_HEIGHT * NUM_CIRCLES_WIDTH * NUM_LEDS_CIRCLE  // Total number of LEDs in the strip
#define NUM_LEDS_ON 5 / 2 // divide by 2 because the # is actually distance from center LED to leave on
#define COLOR_SATURATION 1
#define COLOR_VALUE 1

CRGB leds[NUM_LEDS];     // Define the LED array
RotaryEncoder positionEncoder(POSITION_ROTARY_PIN_1, POSITION_ROTARY_PIN_2, RotaryEncoder::LatchMode::TWO03);  
RotaryEncoder shiftEncoder(SHIFT_ROTARY_PIN_1, SHIFT_ROTARY_PIN_2, RotaryEncoder::LatchMode::TWO03);  
RotaryEncoder colorEncoder(COLOR_ROTARY_PIN_1, COLOR_ROTARY_PIN_2, RotaryEncoder::LatchMode::TWO03);  

int lastPositionEncoder = 0;
int currentPos = 0;
int positionShift = 0;
int colorShift = 0;
int currentHue = 0;

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}

void fillLEDArray() {  
  for (int y = 0; y < NUM_CIRCLES_HEIGHT; y++) {
    for (int x = 0; x < NUM_CIRCLES_WIDTH; x++) {
      for (int i = 0; i < NUM_LEDS_CIRCLE; i++) {
        int pos = NUM_LEDS_CIRCLE * (NUM_CIRCLES_WIDTH * y + x) + i;
        int litPos = positionShift * (x + y) + currentPos;
        litPos = litPos % 20;
        if (abs(litPos - i) <= NUM_LEDS_ON || abs(litPos + NUM_LEDS_CIRCLE - i) <= NUM_LEDS_ON || abs(litPos - NUM_LEDS_CIRCLE - i) <= NUM_LEDS_ON) {
          //convert HSV (which is good for making rainbow) to RGB
          hsv initialColor;
          initialColor.h = currentHue + colorShift * (x + y);
          initialColor.s = COLOR_SATURATION;
          initialColor.v = COLOR_VALUE;
          rgb color = hsv2rgb(initialColor);
          leds[pos] = CRGB(color.r * 255, color.g * 255, color.b * 255);
        }
        else {
          leds[pos] = CRGB::Black;
        }
      }
    }
  }
  
  FastLED.show();  // Display the initial LED states
}

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);  // Initialize FastLED library
  Serial.begin(9600); // open the serial port at 9600 bps:

  fillLEDArray();
}

void loop() {
  positionEncoder.tick();
  shiftEncoder.tick();
  colorEncoder.tick();

  // Speed
  if (lastPositionEncoder != (-1 * positionEncoder.getPosition()) % 20)
  {
    Serial.println("Speed Encoder Moved");
    int dir = (int)positionEncoder.getDirection();
    lastPositionEncoder = (-1 * positionEncoder.getPosition()) % 20;
    currentPos += -1 * dir;
    if (colorShift > 5)
      colorShift -= dir * 1;
    else if (colorShift < -5)
      colorShift += dir * 1;
    else
      colorShift = 0;
  }

  // Delay
  if (positionShift != (-1 * shiftEncoder.getPosition()) % NUM_LEDS_CIRCLE)
  {
    Serial.println("Delay Encoder Moved");
    int dir = (int)shiftEncoder.getDirection();
    currentPos += -1 * dir;
    positionShift = (-1 * shiftEncoder.getPosition()) % NUM_LEDS_CIRCLE;
    colorShift += dir * 1;
  }

  // Color
  currentHue = (colorEncoder.getPosition() * 10) % 360;

  currentPos = currentPos % NUM_LEDS_CIRCLE;
  colorShift = colorShift % 360;
  
  fillLEDArray();
}
