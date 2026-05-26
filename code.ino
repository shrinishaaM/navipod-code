/* * Project: Stark Voice-Assisted Smart ID Card
 * Board: ESP32 38-Pin (Core 2.0.17)
 * IDE: 1.8.19
 * Components: 3x VL53L0X, ADXL345, DFPlayer Mini, PAM8403
 */

#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <DFRobotDFPlayerMini.h>

// XSHUT Pin Definitions
#define SHT_LOX1 13
#define SHT_LOX2 14   // Center sensor moved to GPIO 4 to fix boot error
#define SHT_LOX3 4
#define VIB_MOTOR_PIN 26

// Sensor & Audio Objects
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox3 = Adafruit_VL53L0X();
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
DFRobotDFPlayerMini myDFPlayer;

// Timing Variables
unsigned long lastVoiceTime = 0;
const int voiceInterval = 4000; // 4 seconds between alerts to prevent overlapping

void setID() {
  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);
  pinMode(SHT_LOX3, OUTPUT);
  pinMode(VIB_MOTOR_PIN, OUTPUT);
  
  // Reset all sensors
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  digitalWrite(VIB_MOTOR_PIN, LOW);
  delay(100);

  // Initialize and assign unique addresses
  digitalWrite(SHT_LOX1, HIGH); delay(10);
  if(!lox1.begin(0x30)) { Serial.println(F("Lox1 Fail")); }

  digitalWrite(SHT_LOX2, HIGH); delay(10);
  if(!lox2.begin(0x31)) { Serial.println(F("Lox2 Fail")); }

  digitalWrite(SHT_LOX3, HIGH); delay(10);
  if(!lox3.begin(0x32)) { Serial.println(F("Lox3 Fail")); }
}

void setup() {
  Serial.begin(115200);
  // Using Hardware Serial 2 for DFPlayer (Pins 16 and 17)
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // Power stabilization delay
  delay(3000); 
  Serial.println(F("--- JARVIS System Initializing ---"));

  setID();

  if (!accel.begin()) {
    Serial.println(F("ADXL345 not found!"));
  }

  if (!myDFPlayer.begin(Serial2)) {
    Serial.println(F("Audio System Error! Check Power/SD Card."));
  } else {
    myDFPlayer.volume(25); // Volume 0-30
    Serial.println(F("Audio Online."));
  }

  Serial.println(F("Stark, all systems are go."));
}

void loop() {
  // Only check sensors if the voice interval has passed
  if (millis() - lastVoiceTime > voiceInterval) {
    checkTilt();
    checkObstacles();
  }
}

void checkTilt() {
  sensors_event_t event;
  accel.getEvent(&event);
  
  // Monitor Y-axis for tilt (roughly 45-60 degrees)
  if (abs(event.acceleration.y) > 5.5) {
    Serial.println(F("Alert: ID Card Tilted"));
    digitalWrite(VIB_MOTOR_PIN, HIGH);
    myDFPlayer.playFolder(1, 4);// Play file 004.mp3 
    delay(500);
    digitalWrite(VIB_MOTOR_PIN, LOW);
    lastVoiceTime = millis();
  }
}

void checkObstacles() {
  VL53L0X_RangingMeasurementData_t m1, m2, m3;
  
  lox1.rangingTest(&m1, false);
  lox2.rangingTest(&m2, false);
  lox3.rangingTest(&m3, false);

  // Convert mm to cm
  int dL = (m1.RangeStatus != 4) ? m1.RangeMilliMeter / 10 : 999;
  int dC = (m2.RangeStatus != 4) ? m2.RangeMilliMeter / 10 : 999;
  int dR = (m3.RangeStatus != 4) ? m3.RangeMilliMeter / 10 : 999;

  // Obstacle detection threshold (30cm)
  if (dC < 20) {
    digitalWrite(VIB_MOTOR_PIN, HIGH);
    myDFPlayer.playFolder(1, 1); // Front: 001.mp3
    delay(500);
    digitalWrite(VIB_MOTOR_PIN, LOW);
    lastVoiceTime = millis();
  } 
  else if (dL < 20) {
    digitalWrite(VIB_MOTOR_PIN, HIGH);
    myDFPlayer.playFolder(1, 2); // Left: 002.mp3
    delay(500);
    digitalWrite(VIB_MOTOR_PIN, LOW);
    lastVoiceTime = millis();
  }
  else if (dR < 20) {
    digitalWrite(VIB_MOTOR_PIN, HIGH);
    myDFPlayer.playFolder(1, 3); // Right: 003.mp3
    delay(500);
    digitalWrite(VIB_MOTOR_PIN, LOW);
    lastVoiceTime = millis();
  }
}