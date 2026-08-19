#include <Arduino.h>

// Set to 1 to run a one-shot test that turns off all relays instead of normal operation
#define TEST_ALL_RELAYS_OFF 0

// Pin Definitions
const int micPin = A0;            // SparkFun Sound Detector envelope pin
const int sensPin = A1;           // Potentiometer wiper for sensitivity adjustment
const int lightPins[] = {2, 3, 4, 5, 6, 7}; // Pins connected to relay channels
const int resetButtonPin = 8;     // Momentary button to GND, uses internal pull-up
const int numLights = sizeof(lightPins) / sizeof(lightPins[0]);

// Raw envelope value each light turns on at, once the running peak reaches it (tune after observing real applause)
const int soundThresholds[numLights] = {100, 200, 300, 400, 500, 540};

int peakRaw = 0; // Highest raw envelope value seen since the last reset

// Function to switch a light on or off
void switchLight(int lightPin, bool state) {
  digitalWrite(lightPin, state ? HIGH : LOW); // HIGH = ON for this relay module
}




// Reads the mic, updates the running peak, and returns how many lights that peak has earned
int getLightLevel() {
  int rawValue = analogRead(micPin);          // Read the envelope value
  Serial.print("Raw Envelope Value: ");
  Serial.println(rawValue);                  // Debugging output

  if (rawValue > peakRaw) {
    peakRaw = rawValue; // Only the loudest value since the last reset counts
  }

  // Pot scales all thresholds 50%-200%: turn one way for more sensitive, the other for less
  int sensPercent = map(analogRead(sensPin), 0, 1023, 50, 200);

  int level = 0;
  for (int i = 0; i < numLights; i++) {
    long threshold = (long)soundThresholds[i] * sensPercent / 100;
    if (peakRaw >= threshold) {
      level = i + 1;
    }
  }
  return level;
}

// Function to control lights based on level; lights up to `level` stay on
void controlLights(int level) {
  for (int i = 0; i < numLights; i++) {
    switchLight(lightPins[i], i < level);
  }
}

void setup() {
  Serial.begin(9600); // Initialize serial communication for debugging
  Serial.println("BOOT"); // Reappears if the board resets; helps spot power/wiring glitches

  // Initialize light pins as outputs
  for (int i = 0; i < numLights; i++) {
    pinMode(lightPins[i], OUTPUT);
    switchLight(lightPins[i], false); // Start with all lights OFF
  }

#if TEST_ALL_RELAYS_OFF
  Serial.println("TEST_ALL_RELAYS_OFF: all relays turned off, halting.");
  return; // Skip mic setup so loop() only does the test behavior below
#endif

  pinMode(micPin, INPUT); // Microphone input pin
  pinMode(resetButtonPin, INPUT_PULLUP); // Button reads LOW when pressed
}

void loop() {
#if TEST_ALL_RELAYS_OFF
  return; // Do nothing further; relays were already turned off in setup()
#endif
  if (digitalRead(resetButtonPin) == LOW) {
    peakRaw = 0; // Button pressed: clear the peak without rebooting the board, so relays don't flash
  }

  int lightLevel = getLightLevel(); // Get the current peak-hold level mapped to lights
  controlLights(lightLevel);       // Adjust lights accordingly
  delay(100);                      // Small delay to stabilize readings
}