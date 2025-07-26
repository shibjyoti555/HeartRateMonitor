#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SMOOTHING_WINDOW_SIZE 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int PULSE_SENSOR_PIN = A0;
const int LED_PIN = 13;

const int SAMPLE_INTERVAL = 10;  // Time between samples in milliseconds
const int BEAT_THRESHOLD = 550;  // Adjust based on your sensor's characteristics
const int NO_OBJECT_THRESHOLD = 400;

const unsigned long MIN_BEAT_INTERVAL = 600;  // Minimum time between beats (200 BPM)
const unsigned long MAX_BEAT_INTERVAL = 1500; // Maximum time between beats (40 BPM)

unsigned long lastBeatTime = 0;
int lastReading = 0;
bool rising = false;
float beatsPerMinute = 0;
bool objectPresent = false;

int readings[SMOOTHING_WINDOW_SIZE] = {0};  // Array for smoothing
int readIndex = 0;
int total = 0;

int getSmoothedReading(int reading) {
  total = total - readings[readIndex];        // Subtract the oldest reading
  readings[readIndex] = reading;              // Store new reading
  total = total + readings[readIndex];        // Add the new reading
  readIndex = (readIndex + 1) % SMOOTHING_WINDOW_SIZE;
  return total / SMOOTHING_WINDOW_SIZE;       // Return the average
}


// Heart icon (7x7 pixels)
const unsigned char PROGMEM heart_bmp[] = {
  B00100100,
  B01111110,
  B11111111,
  B11111111,
  B01111110,
  B00111100,
  B00011000
};

// Smaller heart for "beating" effect
const unsigned char PROGMEM heart_small_bmp[] = {
  B00000000,
  B00100100,
  B01111110,
  B01111110,
  B00111100,
  B00011000,
  B00000000
};

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Heart Rate Monitor");
  display.setTextSize(1);
  display.println("\n~~shibjyoti");
  display.display();
  delay(2000);
}

void loop() {
  display.clearDisplay();
  static unsigned long lastSampleTime = 0;
  unsigned long currentTime = millis();
  int reading = getSmoothedReading(analogRead(PULSE_SENSOR_PIN)); // Smoothed reading


  if (currentTime - lastSampleTime >= SAMPLE_INTERVAL) {
    lastSampleTime = currentTime;
    int reading = analogRead(PULSE_SENSOR_PIN);

    if (reading < NO_OBJECT_THRESHOLD) {
      if (objectPresent) {
        objectPresent = false;
        Serial.println("No object detected");
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.println("No object");
        display.println("Place finger on sensor");
        display.display();
      }
    } else {
      objectPresent = true;
      bool beatDetected = detectBeat(reading, currentTime);
      
      if (beatDetected) {
        unsigned long beatInterval = currentTime - lastBeatTime;
        if (beatInterval >= MIN_BEAT_INTERVAL && beatInterval <= MAX_BEAT_INTERVAL) {
          beatsPerMinute = 60000.0 / beatInterval;
          Serial.println(beatsPerMinute);
          displayHeartRate(beatsPerMinute, true);
          digitalWrite(LED_PIN, HIGH);
        }
        lastBeatTime = currentTime;
      } else {
        displayHeartRate(beatsPerMinute, false);
        digitalWrite(LED_PIN, LOW);
      }
    }
  }
  delay(100);
}

bool detectBeat(int reading, unsigned long currentTime) {
  static unsigned long lastDebounceTime = 0;
  bool beat = false;

  if (reading > BEAT_THRESHOLD && reading > lastReading && !rising) {
    rising = true;
  } else if (reading < lastReading && rising) {
    unsigned long debounceDelay = 200;
    if (currentTime - lastDebounceTime > debounceDelay) {
      rising = false;
      beat = true;
      lastDebounceTime = currentTime;
    }
  }
  lastReading = reading;
  return beat;
}


void displayHeartRate(float bpm, bool beating) {
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(bpm, 1);   // Print BPM value without clearing the whole display
  display.println(" BPM");
  display.setTextSize(1);
  display.println(getHealthAdvisory(bpm));

  // Display the heart icon (update only if beating status changes)
  if (beating) {
    display.drawBitmap(110, 0, heart_bmp, 8, 7, SSD1306_WHITE);
  } else {
    display.drawBitmap(110, 0, heart_small_bmp, 8, 7, SSD1306_WHITE);
  }

  display.display();
}


const char* getHealthAdvisory(float bpm) {
  if (bpm < 60) {
    return "Low. Consult doctor if symptomatic";
  } else if (bpm >= 60 && bpm <= 100) {
    return "Normal resting adult heart rate.";
  } else if (bpm > 100 && bpm <= 140) {
    return "Elevated. Normal during exercise or nervousness";
  } else {
    return "Very high. Seek medical attention if persistent";
  }
}
