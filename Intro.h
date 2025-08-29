#ifndef INTRO_H
#define INTRO_H
#include <Adafruit_SSD1306.h>


// Example bitmaps (replace with your own images)
// Each 128x64 image takes 1KB in PROGMEM (128 * 64 / 8)
const unsigned char PROGMEM frame1[] = {
  0x00,0x00,0x00, ... // (generated bitmap data)
};
const unsigned char PROGMEM frame2[] = {
  0x00,0x00,0x00, ... 
};
const unsigned char PROGMEM frame3[] = {
  0x00,0x00,0x00, ... 
};

// Play intro animation
void playIntro(Adafruit_SSD1306 &display) {
  const unsigned char* frames[] = {frame1, frame2, frame3};
  int frameCount = sizeof(frames) / sizeof(frames[0]);

  for (int i = 0; i < frameCount; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0, frames[i], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    delay(500); // adjust for speed
  }
  delay(1000); // pause on last frame
}

#endif