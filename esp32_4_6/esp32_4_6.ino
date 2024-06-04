#include <HardwareSerial.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// Initialize UART2 for ESP32
HardwareSerial SerialPort(2);

// Define Wi-Fi and Firebase credentials
#define WIFI_SSID "TEST"
#define WIFI_PASSWORD "88888888"
#define DATABASE_URL "kltn-9e25e-default-rtdb.firebaseio.com"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long updateDonangPrevMillis = 0;

// Global variables for sensor data
float temperature = 0;
float spo2 = 0;
float heartRate = 0;
float motionDetected = 0;
float Level = 0;
bool levelFromWeb = false;
bool levelUpdatedFromArduino = false;
bool levelChangePending = false;

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(9600);

  // Initialize UART2 with baud rate 9600, RX pin 16, and TX pin 17
  SerialPort.begin(9600, SERIAL_8N1, 16, 17);

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Initialize Firebase
  config.database_url = DATABASE_URL;
  config.signer.test_mode = true;
  Firebase.begin(&config, &auth);
  fbdo.setBSSLBufferSize(4096, 1024);
}

void loop() {
  // Check if data is available from Arduino
  if (SerialPort.available()) {
    // Read data from Arduino until newline character
    String data = SerialPort.readStringUntil('\n');

    // Print received data to Serial Monitor
    Serial.println("Received data: " + data);

    // Parse and validate data
    float tempTemperature, tempSpo2, tempHeartRate, tempMotionDetected, tempLevel;
    bool dataValid = parseSensorData(data, tempTemperature, tempSpo2, tempHeartRate, tempMotionDetected, tempLevel);

    if (dataValid) {
      // Print parsed values to Serial Monitor
      Serial.print("Temperature: ");
      Serial.print(tempTemperature, 1);
      Serial.print(" C, SpO2: ");
      Serial.print(tempSpo2, 1);
      Serial.print(" %, Heart Rate: ");
      Serial.print(tempHeartRate, 1);
      Serial.print(" bpm, Rung: ");
      Serial.print(tempMotionDetected, 1);
      Serial.print(" , level: ");
      Serial.println(tempLevel, 1);

      // Update global variables
      temperature = tempTemperature;
      spo2 = tempSpo2;
      heartRate = tempHeartRate;
      motionDetected = tempMotionDetected;
      
      if (Level != tempLevel) {
        Level = tempLevel;
        levelUpdatedFromArduino = true;
        levelFromWeb = false; // Reset the web flag when Arduino sends new data
        levelChangePending = true; // Indicate there is a pending level change to be sent to Firebase
      }
    } else {
      Serial.println("Data format error.");
    }
  }

  // Send data to Firebase every second
  if (millis() - sendDataPrevMillis > 1000) {
    sendDataPrevMillis = millis();

    Firebase.setFloat(fbdo, "/Giuong_benh/Nhietdo", temperature);
    Firebase.setFloat(fbdo, "/Giuong_benh/Oxi", spo2);
    Firebase.setFloat(fbdo, "/Giuong_benh/Nhiptim", heartRate);
    Firebase.setFloat(fbdo, "/thietbi2/move", motionDetected);

    // Only send Level if it wasn't recently updated from the web and if there is a pending level change
    if (!levelFromWeb && levelChangePending) {
      Firebase.setFloat(fbdo, "/thietbi1/donang", Level);
      levelChangePending = false; // Reset the flag after sending data
    }
  }

  // Update donang value from Firebase and send to Arduino every second
  if (millis() - updateDonangPrevMillis > 1000) {
    updateDonangPrevMillis = millis();

    if (Firebase.getFloat(fbdo, "/thietbi1/donang")) {
      float newLevel = fbdo.floatData();
      Serial.print("Updating donang: ");
      Serial.println(newLevel, 1);

      // Update Level if it's different and wasn't recently updated from Arduino
      if (newLevel != Level && !levelUpdatedFromArduino) {
        Level = newLevel;
        levelFromWeb = true; // Set the flag indicating Level is updated from the web
        SerialPort.print("L:");
        SerialPort.println(Level, 1);
      }
    } else {
      Serial.println("Failed to get donang value from Firebase");
    }

    // Reset the flag to allow Level updates from Arduino
    levelUpdatedFromArduino = false;
  }
}

bool parseSensorData(const String& data, float& temperature, float& spo2, float& heartRate, float& motionDetected, float& Level) {
  // Check if the data string contains the expected labels
  if (data.startsWith("T:") && data.indexOf("S:") != -1 && data.indexOf("H:") != -1 && data.indexOf("D:") != -1 && data.indexOf("L:") != -1) {
    // Find the positions of the labels
    int startIndexT = data.indexOf("T:") + 2;
    int endIndexT = data.indexOf(",", startIndexT);
    int startIndexS = data.indexOf("S:") + 2;
    int endIndexS = data.indexOf(",", startIndexS);
    int startIndexH = data.indexOf("H:") + 2;
    int endIndexH = data.indexOf(",", startIndexH);
    int startIndexD = data.indexOf("D:") + 2;
    int endIndexD = data.indexOf(",", startIndexD);
    int startIndexL = data.indexOf("L:") + 2;
    int endIndexL = data.length();
    // Extract the values as strings
    String tempString = data.substring(startIndexT, endIndexT);
    String spo2String = data.substring(startIndexS, endIndexS);
    String heartRateString = data.substring(startIndexH, endIndexH);
    String swString = data.substring(startIndexD, endIndexD);
    String LevString = data.substring(startIndexL, endIndexL);
    // Convert the strings to float
    temperature = tempString.toFloat();
    spo2 = spo2String.toFloat();
    heartRate = heartRateString.toFloat();
    motionDetected = swString.toFloat();
    Level = LevString.toFloat();
    return true; // Return true if parsing was successful
  }
  return false; // Return false if the format is incorrect
}