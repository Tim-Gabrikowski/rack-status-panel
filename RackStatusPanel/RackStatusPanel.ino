#define PIN        D4 
#define NUMPIXELS 28 
#define DEVICE_COUNT 4
#define METRIC_COUNT 4
#define UPDATE_INTERVAL 500

#define START_INDEX_MET 0
#define START_INDEX_A 4
#define START_INDEX_DEV 14
#define START_INDEX_B 18

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint8_t status[DEVICE_COUNT][METRIC_COUNT * 2] = {};

int sel_device = 0;
int sel_metric = 0;

long lastUpdate = millis();

void init_status() {
  for(int d; d < DEVICE_COUNT; d++){
    for(int m; m < METRIC_COUNT * 2; m += 2){
      status[d][m] = 50;
      status[d][m+1] = 30;
    }
  }
}
void set_metrics() {
  if(millis() - lastUpdate < UPDATE_INTERVAL) return;
  Serial.println("uptade");

  for(int d = 0; d < DEVICE_COUNT; d++){
    for(int m = 0; m < METRIC_COUNT * 2; m += 2){
      status[d][m] = (uint8_t)random(0, 100);
      status[d][m+1] = (uint8_t)random(0, 100);
    }
  }
  lastUpdate = millis();
}

void display_selection(){
  pixels.setPixelColor(START_INDEX_DEV + sel_device, pixels.Color(25, 0, 0));
  pixels.setPixelColor(START_INDEX_MET + sel_metric, pixels.Color(25, 0, 0));
}
void display_value(int start, uint8_t value) {
  Serial.print(start);
  Serial.println(value);
  for(int i = 0; i < 10; i++) {
    if(value >= 10 + 10*i && i < 7) pixels.setPixelColor(start + i, pixels.Color(0, 25, 0));
    if(value >= 70 && i >= 7 && i <= 8) pixels.setPixelColor(start + i, pixels.Color(25, 25, 0));
    if(value >= 90 && i == 9) pixels.setPixelColor(start + i, pixels.Color(25, 0, 0));
  }
}
void display_metrics() {
  display_value(START_INDEX_A, status[sel_device][sel_metric]);
  display_value(START_INDEX_B, status[sel_device][sel_metric + 1]);
}

void checkButtonPressed() {
  
}

void setup() {
  pixels.begin();
  init_status();
  Serial.begin(9600);
}

void loop() {
  
  // check for changes in status
  set_metrics();
  
  // check for button presses
  sel_metric++;
  if(sel_metric >= METRIC_COUNT) sel_device++;
  sel_metric = sel_metric % METRIC_COUNT;
  sel_device = sel_device % DEVICE_COUNT;

  pixels.clear();

  display_selection();
  display_metrics();

  // display values
  pixels.show();
  delay(1000);
}