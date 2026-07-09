# ESP32 LED Matrix Controller with Dynamic Wiring Calculator

A performance-optimized ESP32 web server that hosts an interactive matrix layout calculator, receives raw binary animations via a custom chunked uploader, and plays them smoothly using FastLED across various grid dimensions.

Designed for physical installation and simulation testing in [Wokwi](https://wokwi.com).

---

## 📂 Repository File Tree

```text
├── docs/
│   └── project_history.md        # Chronological development steps (Iterations 1-4)
├── simulation_templates/
│   ├── diagram_standard_500.json # 25x20 matrix config (500 LEDs)
│   ├── diagram_parallel_2000.json# 45x45 matrix config (2,025 LEDs)
│   └── diagram_chained_5000.json # 72x72 matrix config (5,184 LEDs)
├── src/
│   └── main.cpp                  # Multi-core, block-reading ESP32 firmware
├── diagram.json                  # Active Wokwi Simulator Profile
├── partitions.csv                # Custom 16MB Partition Table (12MB LittleFS)
├── platformio.ini                # PlatformIO build configuration
├── merge_bin.py                  # Post-build SPIFFS merging hook script
├── wokwi.toml                    # Local Wokwi network mapping configuration
└── README.md                     # This manual
```

---

## ⚡ Key Software Optimizations

To handle large layouts (up to 6,000 LEDs) and large binary files (5.3MB+) smoothly, the firmware implements:

1. **FreeRTOS Dual-Core Threading**:
   * **Core 1** handles the Web Server (`server.handleClient()`) and incoming chunked file uploads.
   * **Core 0** runs a dedicated background task (`ledTask`) that decodes and plays the LED animation loop.
   * *Benefit*: Heavy Web traffic or file uploads never cause the LED animation to freeze or stutter.
2. **Fast Block Memory Reads**:
   * Rather than reading file bytes individually in a loop (which was extremely slow), the controller reads a whole frame directly into the FastLED memory array in a single block transaction:
     `animationFile.read((uint8_t*)leds, active_leds * 3);`
   * *Benefit*: Cuts frame decode times from **50ms** down to **< 1ms**, boosting refresh rates.
3. **Chunked Upload Handler**:
   * Slices binary animation files into **256KB segments** on the client browser before uploading. The ESP32 writes each block to flash and confirms receipt before requesting the next chunk, preventing heap crashes.

---

## 🔌 Physical Hardware Safety & Protection Guide

When assembling this project physically, follow these wiring safeguards to protect your ESP32 from electrical damage or thermal breakdown:

### 1. External 5V Power Supply (Mandatory)
* **Never** power the LED matrix from the ESP32's 5V/VIN pin. A large grid draws massive current (e.g. 2,000 LEDs draw up to 120 Amps at full white).
* Connect the **External 5V supply** directly to the LED strips.
* Connect a **Common Ground (GND)** between the External 5V supply and the ESP32 board.

### 2. Series Data Resistor
* Place a **`330Ω` to `470Ω` resistor** in series between the ESP32 data pin (GPIO 2) and the first LED strip's `DIN` input.
* *Why*: If the LEDs are powered on but the ESP32 is powered off, current can feed backward through the data pin, destroying the GPIO channel. The resistor limits this current to safe levels.

### 3. Voltage Level Shifter (Recommended)
* The ESP32 sends data signals at 3.3V, but WS2812B LEDs expect 5V logic.
* Use a high-speed level shifter like the **`74AHCT125`** to convert the data signal to 5V before feeding it into the strips. This prevents flashing, flickering, or signal drop over long runs.

### 4. Capacitor
* Place a large electrolytic capacitor (**`1000 µF, 6.3V` or higher**) across the `5V` and `GND` rails of the power supply to smooth out power surges.

---

## 🎮 Simulator Quickstart (Wokwi)

To run and test the simulation profiles locally in VS Code:

1. **Copy the desired profile over `diagram.json`**:
   * *Standard Layout (25x20 matrix - 500 LEDs)*:
     ```powershell
     Copy-Item simulation_templates/diagram_standard_500.json diagram.json -Force
     ```
   * *Parallel Layout (45x45 matrix - 2,025 LEDs)*:
     ```powershell
     Copy-Item simulation_templates/diagram_parallel_2000.json diagram.json -Force
     ```
   * *Chained Parallel Layout (72x72 matrix - 5,184 LEDs)*:
     ```powershell
     Copy-Item simulation_templates/diagram_chained_5000.json diagram.json -Force
     ```

2. **Start the Simulator**:
   Press **`F1`** (or `Ctrl+Shift+P`), then type **`Wokwi: Start Simulator`**.

3. **Access the Portal**:
   Open **[http://localhost:8180](http://localhost:8180)** in your web browser.

4. **Align the Sliders & Upload**:
   * Set the sliders to match the selected layout dimensions (e.g. 45 strips by 45 LEDs).
   * Drag-and-drop the matching test binary (`animation_500.bin`, `animation_2000.bin`, or `animation_5120.bin`) to upload and watch it play.
