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

#define SPEED_COLOR_SHIFT_DRAWBACK 0.125 // how fast speed dial reduces color shift
#define COLOR_SHIFT_INCREASE_SPEED 0.25 // how fast delay dial changes color shift
#define COLOR_SHIFT_ZERO_OFFSET 1
#define INITIAL_COLOR_INCREASE_SPEED 0.1

CRGB leds[NUM_LEDS];     // Define the LED array
RotaryEncoder speedEncoder(POSITION_ROTARY_PIN_1, POSITION_ROTARY_PIN_2, RotaryEncoder::LatchMode::TWO03);  
RotaryEncoder delayEncoder(SHIFT_ROTARY_PIN_1, SHIFT_ROTARY_PIN_2, RotaryEncoder::LatchMode::TWO03);  
RotaryEncoder colorEncoder(COLOR_ROTARY_PIN_1, COLOR_ROTARY_PIN_2, RotaryEncoder::LatchMode::TWO03);  

int lastSpeedEncoder = 0;
double currentPos = 0;
int lastDelayEncoder = 0;
double currentDelay = 0;
int lastColorEncoder = 0;
double colorShift = 0;
double currentHue = 0;

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

double doubleModulo(double val, int mod)
{
  while (val >= mod)
    val -= mod;
  while (val < 0)
    val += mod;
  return val;
}

int intModulo(int val, int mod)
{
  val = val % mod;
  return val >= 0 ? val : val + mod;
}

void fillLEDArray() {  
  for (int y = 0; y < NUM_CIRCLES_HEIGHT; y++) {
    for (int x = 0; x < NUM_CIRCLES_WIDTH; x++) {
      int litPos = (int) (currentDelay * (x + y) + currentPos);
      litPos = intModulo(litPos, 20);
      int distanceFromTop = litPos < 10 ? litPos : 20 - litPos;
      distanceFromTop += COLOR_SHIFT_ZERO_OFFSET;
      for (int i = 0; i < NUM_LEDS_CIRCLE; i++) {
        int ledPos = NUM_LEDS_CIRCLE * (NUM_CIRCLES_WIDTH * y + x) + i;
        if (abs(litPos - i) <= NUM_LEDS_ON || abs(litPos + NUM_LEDS_CIRCLE - i) <= NUM_LEDS_ON || abs(litPos - NUM_LEDS_CIRCLE - i) <= NUM_LEDS_ON) {
          //convert HSV (which is good for making rainbow) to RGB
          hsv initialColor;
          initialColor.h = doubleModulo(currentHue + colorShift * distanceFromTop, 360);
          initialColor.s = COLOR_SATURATION;
          initialColor.v = COLOR_VALUE;
          rgb color = hsv2rgb(initialColor);
          leds[ledPos] = CRGB(color.r * 255, color.g * 255, color.b * 255);
        }
        else {
          leds[ledPos] = CRGB::Black;
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
  speedEncoder.tick();
  delayEncoder.tick();
  colorEncoder.tick();

  // Speed
  if (lastSpeedEncoder != speedEncoder.getPosition())
  {
    Serial.println("Speed Encoder Moved");
    lastSpeedEncoder = speedEncoder.getPosition();
    int dir = (int)speedEncoder.getDirection();
    currentPos += -dir * 1;
    if (colorShift > SPEED_COLOR_SHIFT_DRAWBACK)
      colorShift -= SPEED_COLOR_SHIFT_DRAWBACK;
    else if (colorShift < -SPEED_COLOR_SHIFT_DRAWBACK)
      colorShift += SPEED_COLOR_SHIFT_DRAWBACK;
    else
      colorShift = 0;
    Serial.print("Color Shift: ");
    Serial.println(colorShift);
  }

  // Delay
  if (lastDelayEncoder != delayEncoder.getPosition())
  {
    Serial.println("Delay Encoder Moved");
    lastDelayEncoder = delayEncoder.getPosition();
    int dir = (int)delayEncoder.getDirection();
    currentPos += -dir * 1;
    currentDelay += -dir * 0.5;
    colorShift += dir * COLOR_SHIFT_INCREASE_SPEED;
    currentHue += dir * INITIAL_COLOR_INCREASE_SPEED;
    Serial.print("Color Shift: ");
    Serial.println(colorShift);
    Serial.print("Hue: ");
    Serial.println(currentHue);
  }

  // Color
  if (lastColorEncoder != colorEncoder.getPosition())
  {
    Serial.println("Color Encoder Moved");
    lastColorEncoder = colorEncoder.getPosition();
    int dir = (int)colorEncoder.getDirection();
    currentHue += dir * 10;
    Serial.print("Hue: ");
    Serial.println(currentHue);
  }

  currentPos = doubleModulo(currentPos, NUM_LEDS_CIRCLE);
  currentDelay = doubleModulo(currentDelay, NUM_LEDS_CIRCLE);
  currentHue = doubleModulo(currentHue, 360);

  fillLEDArray();
}
