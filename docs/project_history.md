# Project Evolution & Design History

This document logs the step-by-step development history of the ESP32 LED Animation Controller, from its initial basic prototype to the final optimized multi-core system.

---

## 🛠️ Iteration 1: The Basic Prototype (Access Point)
The project started with a simple access point setup to display fixed-size animations.

### Characteristics:
* **Wi-Fi Mode**: SoftAP (Access Point) with its own SSID/Password.
* **Storage**: Default 4MB flash partition table, leaving only ~1.5MB for the SPIFFS filesystem.
* **File Reading**: Read pixel data from `/animation.bin` byte-by-byte (3 bytes at a time) directly inside the main Arduino `loop()`.

### Code Snippet (Iteration 1 Reader):
```cpp
// Inside loop()
if (animationFile) {
  if (animationFile.available() < 500 * 3) {
    animationFile.seek(0); // Loop back
  }
  
  uint8_t buffer[3];
  for (int i = 0; i < 500; i++) {
    if (animationFile.read(buffer, 3) == 3) {
      leds[i].r = buffer[0];
      leds[i].g = buffer[1];
      leds[i].b = buffer[2];
    }
  }
  FastLED.show();
  delay(33);
}
```

### Major Bottlenecks Discovered:
1. **Flash Upload Lockups**: Large binary files (5MB+) uploaded over a single POST request would crash the simulated ESP32 heap.
2. **Animation Stutter**: Making 1,500 individual `File.read()` system calls every 30ms caused massive disk-read overhead, lagging the frame rate.
3. **Rigid Layout**: Matrix size was statically compiled at 500 LEDs (25x20).

---

## 💾 Iteration 2: 16MB Flash Upgrade & Chunked Uploading
To support large animation binaries (like the client's 5.3MB file), we expanded the filesystem and rebuilt the transfer method.

### Upgrades:
* **Wokwi Board Configuration**: Upgraded the simulator board in `diagram.json` to `"flashSize": "16"`.
* **Custom Partitions**: Added a `partitions.csv` table allocating **12MB** to the LittleFS filesystem.
* **Chunked Uploading**: Replaced the standard HTML upload handler with a chunked JavaScript uploader. The client slices the binary file into **256KB segments** and sends them sequentially, waiting for the ESP32 to write each block before sending the next one.

---

## 📊 Iteration 3: Matrix Wiring Calculator & Dynamic Layouts
To accommodate different physical client setups, we built a layout wizard.

### Upgrades:
* **Dynamic Sliders**: The web portal was upgraded with sliders for **Number of Strips** and **LEDs per Strip**.
* **Smart Layout Engine**: Calculates total power, required current, framerate, and maps strips to the ESP32's 16 safe output GPIOs.
* **Layout Persistence**: Settings are posted to `/set_layout` and saved in `/layout.txt` inside LittleFS. During boot, the ESP32 reads this file and adjusts its runtime active count dynamically.

---

## ⚡ Iteration 4: Multicore Isolation & Block Memory Reads (Final)
To eliminate Wokwi simulation lag and guarantee perfectly smooth rendering on physical hardware, we applied advanced firmware optimizations.

### Final Optimizations:
1. **Single-Block Flash Reads**:
   We replaced the loop of 3-byte reads with a single block-level read directly into FastLED's memory array:
   ```cpp
   animationFile.read((uint8_t*)leds, active_leds * 3);
   ```
   This reduced frame load times from **50+ ms** to **< 1 ms**.
2. **FreeRTOS Dual-Core Execution**:
   Isolated the animation loop entirely onto **Core 0** as a background task. The web server runs on **Core 1**. Web interactions or upload writes no longer interrupt the LED timing clock, preventing stutters.

```text
       Core 0 (LED Thread)               Core 1 (Web Thread)
┌──────────────────────────────┐   ┌──────────────────────────────┐
│  - Blocks read from flash    │   │  - server.handleClient()     │
│  - Runs FastLED.show()       │   │  - Receives uploads          │
│  - vTaskDelay (Idle sleeping)│   │  - Sets active_leds config   │
└──────────────────────────────┘   └──────────────────────────────┘
```
