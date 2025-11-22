# 🍎 ESP32 Cloud-Actuated IoT Media Player
A multi-threaded embedded system that streams raw video and audio from an SD card, controlled remotely via Discord Slash Commands with millisecond latency.

## 📖 Overview
This project demonstrates an end-to-end IoT Control System. It allows a user to control physical hardware (an ESP32) via a cloud interface (Discord). The system features a custom-written video rendering engine and a direct-drive audio driver to overcome the hardware limitations of the ESP32 Devkit V1 chip.
Key Features
 * ☁️ Cloud Telemetry: Controlled via a custom Discord Bot using MQTT as the real-time message broker.
 * 📺 High-Speed Rendering: Plays video at a locked 15 FPS on an I2C OLED by overclocking the bus to 800kHz.
 * 🔊 Direct-Drive Audio: Uses a custom DAC driver to generate UI feedback sounds (beeps/chimes) without heavy audio libraries.
 * ⏱️ Fixed-Timestep Engine: Implements a "Metronome" loop to ensure perfect frame pacing regardless of SD card read latency.
 * 🧠 State Machine Architecture: Robust handling of IDLE, PLAYING, and PAUSED states with non-destructive overlays.
## 🏗️ System Architecture
```
graph LR
    A[User] -- "/play" --> B(Discord Cloud)
    B -- Event --> C[Python Bot]
    C -- MQTT Publish --> D[HiveMQ Broker]
    D -- MQTT Subscribe --> E[ESP32 Controller]
    E -- SPI --> F[SD Card]
    E -- I2C (800kHz) --> G[OLED Display]
    E -- DAC (GPIO25) --> H[Amplifier & Speaker]
```
## 🛠️ Hardware BOM
| Component | Function | Pinout |
|---|---|---|
| ESP32 DevKit V1 | Main Microcontroller | N/A |
| 1.3" SH1106 OLED | Video Output | SDA: 33, SCL: 14 |
| MicroSD Module | Asset Storage | CS: 5, MOSI: 23, MISO: 19, SCK: 18 |
| LM386 Amplifier | Audio Driver | IN: 25 (Internal DAC), VCC: 5V |
| 3W Speaker | Haptic/Audio Feedback | Connected to Amp Output |
## ⚙️ Engineering Challenges & Solutions
### 1. The I2C Bottleneck
Problem: Standard I2C speed (400kHz) is too slow to push 1024 bytes of video data + overhead at 15 FPS, resulting in screen tearing and lag.
Solution: The I2C bus was overclocked to 800kHz (Turbo Mode) within the firmware Wire.setClock(800000), doubling the throughput and allowing for fluid playback.
### 2. Audio Resource Contention
Problem: Decoding MP3/WAV files requires significant RAM and CPU cycles, causing the video renderer to stutter or the ESP32 to crash due to memory fragmentation.
Solution: I wrote a custom Direct-Drive Audio Engine. Instead of decoding files, the ESP32 manually toggles the internal DAC (Digital-to-Analog Converter) to generate square-wave "Chiptune" melodies for UI feedback. This uses near-zero RAM.
### 3. Frame Pacing (The Metronome)
Problem: SD Card read times are inconsistent. A naive loop results in "jittery" video (variable framerate).
Solution: Implemented a Fixed-Timestep Loop:
```
long wait = frame_interval - execution_time;
if (wait > 0) delay(wait);
```

This ensures the video runs at a strict 15 Hz, regardless of how fast the processor finishes the drawing task.
## 🚀 Installation & Setup
### 1. Firmware (ESP32)
 * Open BadAppleFinal.ino in Arduino IDE.
 * Install dependencies:
   * PubSubClient (Nick O'Leary)
   * Adafruit_GFX & Adafruit_SH110X
 * Update ssid, password, and pins in config section.
 * Upload to ESP32.
### 2. Controller (Python)
 * Install Python requirements:
   `pip install discord.py paho-mqtt` (for Fedora)

 * Update DISCORD_TOKEN and GUILD_ID in bot.py.
 * Run the bot:
   `python3 bot.py` (for Fedora)

### 3. Asset Preparation
The video file must be processed into a raw binary format.
 * Extract frames from video (1-bit Dithered).
 * Run the provided FFmpeg command (`ffmpeg.sh`) or Python script to generate `badapple.bin`.
 * Copy `badapple.bin` to the root of the SD card.
## 🎮 Usage
Go to your Discord Server and use the Slash Commands:
 * /play: Starts the video and plays the startup chime.
 * /pause: Freezes video and displays "PAUSED" overlay.
 * /resume: Continues playback from the exact frame.
 * /stop: Clears screen, stops audio, and resets the state machine.
## 📜 Credits
 * Video Source: "Bad Apple!!" (Touhou Project)
 * Libraries: Adafruit, PubSubClient
