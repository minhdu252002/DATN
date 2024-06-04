#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>
#include <Servo.h>

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define UPDATE_PERIOD_MS 3000
#define DISPLAY_UPDATE_PERIOD_MS 2000

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
PulseOximeter pox;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

uint32_t lastSensorUpdate = 0; 
uint32_t lastDisplayUpdate = 0;

// Servo control parameters
Servo servo1;  
const int upButton = 2;        
const int downButton = 3;     
const int buttonPin = 4; 
const int relayPin1 = 6;          
const int buzzerPin = 7; 

const int swPin = 8; 

int buttonState;
int motionDetected = LOW;
int angle1 = 0;   
unsigned long lastDebounceTimeUp = 0;   
unsigned long lastDebounceTimeDown = 0; 
const unsigned long debounceDelay = 150; 
bool relayActive = false;
unsigned long relayStartTime = 0;
const unsigned long relayDuration = 1000; 
int Level = 0;
bool levelFromESP32 = false; // Flag to indicate if Level was set by ESP32

void onBeatDetected() {
  // Callback for beat detection (not used in this example)
}

void setup() {
  Serial.begin(9600);
  Serial.print("Initializing pulse oximeter...");
  Wire.begin();

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialize temperature sensor
  mlx.begin(0x5A);

  // Initialize Pulse Oximeter
  if (!pox.begin()) {
    Serial.println("Pulse oximeter initialization FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);

  // Initialize servo motor
  servo1.attach(5);

  // Initialize button and relay pins
  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(relayPin1, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(swPin, INPUT);
}

void loop() {
  pox.update();
  
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorUpdate >= UPDATE_PERIOD_MS) {
    updateSensorData();
    lastSensorUpdate = currentMillis;
  }

  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_PERIOD_MS) {
    updateDisplay();
    handleBuzzer();
    handleMotionDetection();
    lastDisplayUpdate = currentMillis;
  }

  handleButtonInput();
  handleRelay();
  handleESP32Commands();
}

void updateSensorData() {
  float temperature = mlx.readObjectTempC();
  float spo2 = pox.getSpO2();
  float heartRate = pox.getHeartRate();
  Serial.print("T:"); Serial.print(temperature);
  Serial.print(", S:"); Serial.print(spo2);
  Serial.print(", H:"); Serial.print(heartRate);
  Serial.print(", D:"); Serial.print(motionDetected);
  Serial.print(", L:"); Serial.println(Level);
}
void updateDisplay() {
  float temperature = mlx.readObjectTempC();
  float spo2 = pox.getSpO2();
  float heartRate = pox.getHeartRate();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("T:"); display.print(temperature); display.println("C");
  display.setCursor(70, 0);
  display.print("D:"); display.print(motionDetected);
  display.setCursor(70, 10);
  display.print("Bu:"); display.println(digitalRead(buzzerPin) == HIGH ? "ON" : "OFF");
  display.setCursor(0, 10);
  display.print("Heart:"); display.println(heartRate);
  display.setCursor(0, 20);
  display.print("Oxygen:"); display.println(spo2);
  display.setCursor(70, 20);
  display.print("Level:"); display.println(Level);
  display.display();
}

void handleBuzzer() {
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW || mlx.readObjectTempC() > 40 || pox.getHeartRate() > 110) {
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
  }
}

void handleMotionDetection() {
  int sensorValue = digitalRead(swPin);
  if (sensorValue == LOW) {
    motionDetected = LOW;
  } else if (sensorValue == HIGH) {
    motionDetected = HIGH;
  }
}

void handleButtonInput() {
  int upReading = digitalRead(upButton);
  int downReading = digitalRead(downButton);

  if (upReading == LOW && (millis() - lastDebounceTimeUp) > debounceDelay) {
    lastDebounceTimeUp = millis();
    if (Level < 3) {
      Level++;
      angle1 = Level * 30;  // Convert level to angle
      servo1.write(angle1);
      activateRelay();
      sendLevelToESP32();
      levelFromESP32 = false; // Indicate that Level was updated from button
    }
  }

  if (downReading == LOW && (millis() - lastDebounceTimeDown) > debounceDelay) {
    lastDebounceTimeDown = millis();
    if (Level > 0) {
      Level--;
      angle1 = Level * 30;  // Convert level to angle
      servo1.write(angle1);
      activateRelay();
      sendLevelToESP32();
      levelFromESP32 = false; // Indicate that Level was updated from button
    }
  }
}

void handleRelay() {
  if (relayActive && millis() - relayStartTime > relayDuration) {
    digitalWrite(relayPin1, LOW);
    relayActive = false;
  }
}

void activateRelay() {
  relayActive = true;
  relayStartTime = millis();
  digitalWrite(relayPin1, HIGH);
}

void handleESP32Commands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if (command.startsWith("L:")) {
      String levelString = command.substring(2);
      int newLevel = levelString.toInt();
      newLevel = constrain(newLevel, 0, 3);  // Constrain Level between 0 and 3
      if (newLevel != Level) {
        Level = newLevel;
        angle1 = Level * 30;  // Convert level to angle
        servo1.write(angle1);
        activateRelay();
        sendLevelToESP32();  // Acknowledge the new level to ESP32
        levelFromESP32 = true; // Indicate that Level was updated from ESP32
      }
    }
  }
}

void sendLevelToESP32() {
  Serial.print("L:");
  Serial.println(Level);
}