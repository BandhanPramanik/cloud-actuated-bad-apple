/*
 * ESP32 PROJECT: IOT VIDEO PLAYER + UI SOUNDS
 * - Video: 15 FPS Turbo
 * - Audio: System Beeps/Chimes (Justifies the hardware!)
 * - Control: MQTT
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// --- CONFIG ---
const char* ssid         = "WIFI_SSID"; // CHANGE THIS 
const char* password     = "WIFI_PASSWORD"; // CHANGE THIS
const char* mqtt_server  = "broker.hivemq.com";
const char* mqtt_topic   = "TOPIC"; // MQTT TOPIC

// --- PINS ---
#define CUSTOM_SDA 33
#define CUSTOM_SCL 14
#define SD_CS_PIN  5
#define SPEAKER_PIN 25 // DAC Pin

// --- TUNING ---
#define TARGET_FPS 15
const int frame_interval = 1000 / TARGET_FPS;

// --- GLOBALS ---
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);
WiFiClient espClient;
PubSubClient client(espClient);
File videoFile;
uint8_t buffer[1024];
unsigned long lastFrameStart = 0;

// State
enum AppState { STATE_IDLE, STATE_PLAYING, STATE_PAUSED };
AppState currentState = STATE_IDLE;

// --- SOUND ENGINE / DIRECT-DRIVE AUDIO DRIVER (The "Money Saver") ---
// Simple function to generate a beep without libraries
void beep(int freq, int duration) {
  int delayTime = 1000000 / freq / 2;
  long loops = (long)freq * duration / 1000;
  for (long i = 0; i < loops; i++) {
    dacWrite(SPEAKER_PIN, 200); // High
    delayMicroseconds(delayTime);
    dacWrite(SPEAKER_PIN, 0);   // Low
    delayMicroseconds(delayTime);
  }
  dacWrite(SPEAKER_PIN, 0); // Silence
}

// Play a "Success" Chime (Mario Coin style)
void playStartSound() {
  beep(1000, 100);
  delay(50);
  beep(2000, 200);
}

// Play a "Stop" Sound (Low pitch)
void playStopSound() {
  beep(500, 150);
  beep(300, 300);
}

// Play a "Pause" blip
void playPauseSound() {
  beep(1500, 50);
  delay(50);
  beep(1500, 50);
}

// --- CALLBACK ---
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for(int i=0; i<length; i++) msg += (char)payload[i];
  Serial.print("CMD: "); Serial.println(msg);

  if (msg == "PLAY") {
    if(videoFile) videoFile.close();
    videoFile = SD.open("/badapple.bin");
    
    if (videoFile) {
      videoFile.seek(0);
      currentState = STATE_PLAYING;
      playStartSound(); // <--- MAKES NOISE!
      Serial.println("Playing");
    }
  } 
  else if (msg == "PAUSE") {
    if (currentState == STATE_PLAYING) {
      currentState = STATE_PAUSED;
      playPauseSound(); // <--- MAKES NOISE!
      
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.setCursor(40,25); display.print("PAUSE");
      display.display();
    }
  }
  else if (msg == "RESUME") {
    if (currentState == STATE_PAUSED) {
      playStartSound(); // <--- MAKES NOISE!
      currentState = STATE_PLAYING;
    }
  }
  else if (msg == "STOP") {
    currentState = STATE_IDLE;
    if(videoFile) videoFile.close();
    playStopSound(); // <--- MAKES NOISE!
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextColor(SH110X_WHITE);
    display.println("STOPPED.");
    display.display();
  }
}

void reconnect() {
  while (!client.connected()) {
    String id = "ESP32-" + String(random(0xffff), HEX);
    if (client.connect(id.c_str())) {
      client.subscribe(mqtt_topic);
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("ONLINE.");
      display.display();
      beep(2000, 50); // Connection Beep!
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Config DAC
  pinMode(SPEAKER_PIN, OUTPUT); 

  Wire.begin(CUSTOM_SDA, CUSTOM_SCL);
  Wire.setClock(800000);
  
  if(!display.begin(0x3C, true)) while(1);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.println("Booting...");
  display.display();

  if (!SD.begin(SD_CS_PIN)) { 
    display.println("SD Fail!"); display.display(); while(1); 
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(100);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (currentState == STATE_PLAYING && videoFile) {
    unsigned long now = millis();
    if (now - lastFrameStart >= frame_interval) {
      lastFrameStart = now;
      if (videoFile.available()) {
        videoFile.read(buffer, 1024);
        display.drawBitmap(0, 0, buffer, 128, 64, 1, 0);
        display.display();
      } else {
        // Loop
        videoFile.seek(0);
      }
    }
  }
}
