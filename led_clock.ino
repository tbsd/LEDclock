#include "float.h"
#include <math.h>
#include <NeoPixelSegmentBus.h>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include <NeoPixelBus.h>
#include "GyverButton.h"
#include <Wire.h>
#include <DS3231.h>
#include "Queue.h"

#define LED_PIN 2
#define SOUND_PIN 11
#define NUM_LEDS 72 
#define SENSOR_BUTT_PIN 10

#define OFF      0
#define LIGHT    1
#define FLAME    2
#define CLOCK    3
#define FLAMECLOCK 4

int brightness = 255;
int mode = FLAMECLOCK;
int prevMode = FLAMECLOCK;
long lastSector = -1;
NeoPixelBus<NeoGrbwFeature, NeoSk6812Method> strip(NUM_LEDS, LED_PIN);
GButton sensorButt(SENSOR_BUTT_PIN);
DS3231 clock;
RTCDateTime dt;

void setup() {
  pinMode(SOUND_PIN, INPUT_PULLUP);
  clock.begin();
  strip.Begin();
  strip.ClearTo(RgbwColor(0, 0, 0, 0));   
  strip.Show();  
  persistantRainbow();                      
  changeMode();
}

void loop() {   
  sensorButt.tick();

  if (sensorButt.hasClicks()) {
    byte clicks = sensorButt.getClicks();
    menu(clicks);
  }
  if (digitalRead(SOUND_PIN) == 0) {
    byte clicks = 1;
    uint32_t clickTimer = millis();
    while (millis() < clickTimer + 1000) {
      if (millis() < clickTimer + 100) {
        playEffects();
        continue;
      }
      if (digitalRead(SOUND_PIN) == 0) {
        clickTimer = millis();
        ++clicks; 
      }
    }
    menu(clicks);
  }

  playEffects();
  dt = clock.getDateTime();
  if (dt.hour == 9 && dt.minute == 0 && dt.second < 5) {
    mode = FLAMECLOCK;
    prevMode = FLAMECLOCK;
    changeMode();
  } else {
    if (dt.hour == 0 && dt.minute == 0 && dt.second < 5) {
      mode = OFF;
      changeMode();
    } 
  }
  delay(10);
}

void playEffects() {
  switch (mode) {
    case OFF:
      break;
    case LIGHT:
      break;
    case FLAME:
      flameEffect5();
      break;
    case CLOCK:
      clockMode();
      break;
    case FLAMECLOCK:
      flameClock();   
      break;
  }
}

void menu(byte clicks) {
  switch (clicks) {
    default:
    case 1:
      if (mode == OFF) {
        persistantRainbow();
        mode = prevMode;
      }
      else {
        prevMode = mode; 
        mode = OFF;
      }
      break;
    case 2:
      if (mode == FLAMECLOCK) {
        mode = LIGHT;
      } else {
        mode = FLAMECLOCK;
      }
      prevMode = mode;
      break;
  }
  changeMode();
}

void flameClock() {
  clockMode();
  flameEffect5();
}

void clockMode() { 
  dt = clock.getDateTime();
  static int lastHour = dt.hour;

  long sectors = (((dt.hour * 60L + dt.minute) * 60 + dt.second) / 600) % 72; 
  int middle = NUM_LEDS / 2;

  if (lastSector != sectors) {
    if (dt.hour < 12) {
      if (sectors + middle >= NUM_LEDS) {
        strip.ClearTo(RgbwColor(0, 0, brightness, 0), 0, middle - 1);
        strip.ClearTo(RgbwColor(0, 0, 0, brightness), middle, NUM_LEDS - 1); 
        strip.ClearTo(RgbwColor(0, 0, 0, brightness), 0, middle + sectors - NUM_LEDS - 1);
      } else {        
        strip.ClearTo(RgbwColor(0, 0, 0, brightness), middle, middle + sectors - 1); 
        strip.ClearTo(RgbwColor(0, 0, brightness, 0), middle + sectors, NUM_LEDS - 1);
        strip.ClearTo(RgbwColor(0, 0, brightness, 0), 0, middle - 1);
      }

    } else {
      if (sectors + middle >= NUM_LEDS) {
        strip.ClearTo(RgbwColor(0, 0, 0, brightness), 0, middle - 1);
        strip.ClearTo(RgbwColor(0, 0, brightness, 0), middle, NUM_LEDS - 1); 
        strip.ClearTo(RgbwColor(0, 0, brightness, 0), 0, middle + sectors - NUM_LEDS - 1);

      } else {        
        strip.ClearTo(RgbwColor(0, 0, brightness, 0), middle, middle + sectors - 1); 
        strip.ClearTo(RgbwColor(0, 0, 0, brightness), middle + sectors, NUM_LEDS - 1);
        strip.ClearTo(RgbwColor(0, 0, 0, brightness), 0, middle - 1);
      }

    }


    if (mode == CLOCK)
      strip.Show();
    lastSector = sectors;
    if (lastHour != dt.hour) {
      rainbow();
      lastHour = dt.hour;
    }
  }
}

void initClock() {
  strip.ClearTo(RgbwColor(0, 0, 0, 0));
  lastSector = -1;
  clockMode();
}

void changeMode() {
  switch (mode) {
    case OFF:
      strip.ClearTo(RgbwColor(0, 0, 0, 0));
      strip.Show();
      break;
    case LIGHT:
      light();
      break;
    case FLAME:
      strip.ClearTo(RgbwColor(0, 0, 0, 0));
      strip.ClearTo(RgbwColor(0, 0, 0, brightness));
      flameEffect5();
      break;
    case CLOCK:
      initClock();
      break;
    case FLAMECLOCK:
      initClock();
      break;
  }
}
void light() {
  for (int i = 0; i < NUM_LEDS; ++i) {
    strip.SetPixelColor(i, RgbwColor(0, 0, 0, brightness));
    strip.Show();
  }
}

void flameEffect5() {
  static boolean isForward = true;
  static double step = random(0, 1024);
  double lightness = brightness / 255.0;

  int thisPos = 0, lastPos = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    auto oldColor = strip.GetPixelColor(i);
    double newW =  (renderNoise(i, step));
    RgbwColor newColor;
    if (newW < 8) {
      newW = 4;
      if (oldColor.B != 0)
        newColor = RgbwColor(0, 0, newW, 0); 
      else
        newColor = RgbwColor(0, 0, 0, newW);
    } else {        
      if (oldColor.B != 0)
        newColor = RgbwColor(0, 0, newW * lightness, 0); 
      else
        newColor = RgbwColor(0, 0, 0, newW * lightness);
    }
    strip.SetPixelColor(i, newColor);
  }
  if (isForward) {
    step += 0.025;
    if (step >= FLT_MAX_EXP - 2)
      isForward = false;
  } else {
    step -= 0.025;
    if (step <= 2)
      isForward = true;
  }
  strip.Show();
}

void persistantRainbow() {
  int del = 2;
  for (int i = 0; i < NUM_LEDS; ++i) {
    strip.SetPixelColor(i, RgbwColor(0, 0, 0, 0));
    strip.SetPixelColor(i, HsbColor(1.0 * i / NUM_LEDS, 1.0, brightness/255.0));
    strip.Show();
    delay(del);
  }
}

void rainbow() {
  int buffSz = 20;
  int del = 4;
  Queue<RgbwColor> prevColors(buffSz);
  for (int i = 0; i < NUM_LEDS; ++i) {
    if (i >= buffSz)
      strip.SetPixelColor(i - buffSz, prevColors.pop()); 
    prevColors.push(strip.GetPixelColor(i));
    strip.SetPixelColor(i, RgbwColor(0, 0, 0, 0));
    strip.SetPixelColor(i, HsbColor(1.0 * i / NUM_LEDS, 1.0, brightness/255.0));
    strip.Show();
    delay(del);
  }
  for (int i = NUM_LEDS - buffSz; i < NUM_LEDS; ++i) {
    strip.SetPixelColor(i, prevColors.pop()); 
    strip.Show();
    delay(del);
  }
}

byte renderNoise(float x, float y) { 
  float noise;
  noise = perlin(x,y); 
  noise = noise * 128 + 127; 
  return (byte) noise;  
}

float perlin(float x, float y) {
  int fx = floor(x);
  int fy = floor(y);
  float s,t,u,v;
  s = perlin2d(fx,fy);
  t = perlin2d(fx+1,fy);
  u = perlin2d(fx,fy+1);
  v = perlin2d(fx+1,fy+1);
  float inter1 = lerp(s,t,x-fx);
  float inter2 = lerp(u,v,x-fx);  
  return lerp(inter1,inter2,y-fy);
}

float perlin2d(int x, int y) {
  long n=(long)x+(long)y*57;
  n = (n<<13)^ n;
  return 1.0 - (((n *(n * n * 15731L + 789221L) + 1376312589L)  & 0x7fffffff) / 1073741824.0);    
}

float lerp(float a, float b, float x) {
  return a + x * (b - a);
}
