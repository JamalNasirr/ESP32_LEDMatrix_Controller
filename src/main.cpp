#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <FastLED.h>

#define LED_PIN 2      
#define MAX_LEDS 6000

CRGB leds[MAX_LEDS];
int active_leds = 500;
CLEDController *ledController = NULL;
File animationFile;
bool isUploading = false;

// Frame timing — target ~30 FPS
const unsigned long FRAME_INTERVAL_MS = 33;

WebServer server(80);

const char* ssid = "Wokwi-GUEST";
const char* password = ""; 

// HTML Web Page with dark mode styling and uploader
const char index_html[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 LED Controller</title>
    <style>
        :root {
            --bg-color: #0f0c1b;
            --card-bg: #1b162e;
            --accent-color: #7f5af0;
            --accent-hover: #9b7bf6;
            --text-color: #fffffe;
            --text-muted: #94a1b2;
            --success-color: #2cb67d;
        }
        body {
            background-color: var(--bg-color);
            color: var(--text-color);
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            box-sizing: border-box;
        }
        .container {
            width: 100%;
            max-width: 600px;
            background-color: var(--card-bg);
            border-radius: 16px;
            padding: 30px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.5);
            border: 1px solid rgba(255,255,255,0.05);
        }
        h1 {
            font-size: 24px;
            text-align: center;
            margin-bottom: 30px;
            color: var(--text-color);
            background: linear-gradient(45deg, #7f5af0, #2cb67d);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .section {
            margin-bottom: 25px;
        }
        .section-title {
            font-size: 16px;
            font-weight: 600;
            margin-bottom: 12px;
            color: var(--accent-color);
            text-transform: uppercase;
            letter-spacing: 1px;
            border-bottom: 1px solid rgba(255,255,255,0.1);
            padding-bottom: 6px;
        }
        .guide-card {
            background: rgba(255,255,255,0.02);
            border-radius: 8px;
            padding: 15px;
            margin-bottom: 10px;
            border-left: 4px solid var(--accent-color);
        }
        .guide-line {
            display: flex;
            justify-content: space-between;
            margin-bottom: 8px;
            font-size: 14px;
        }
        .guide-line:last-child {
            margin-bottom: 0;
        }
        .pin-label {
            font-weight: 600;
            color: var(--text-color);
            background: rgba(255,255,255,0.1);
            padding: 2px 6px;
            border-radius: 4px;
            font-family: monospace;
        }
        .upload-area {
            border: 2px dashed rgba(255,255,255,0.15);
            border-radius: 12px;
            padding: 30px;
            text-align: center;
            cursor: pointer;
            transition: all 0.3s ease;
            background: rgba(255,255,255,0.01);
        }
        .upload-area:hover, .upload-area.dragover {
            border-color: var(--accent-color);
            background: rgba(127,90,240,0.05);
        }
        .upload-icon {
            font-size: 40px;
            margin-bottom: 10px;
            color: var(--text-muted);
        }
        .upload-btn {
            background-color: var(--accent-color);
            color: var(--text-color);
            border: none;
            padding: 10px 20px;
            border-radius: 8px;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s;
            margin-top: 10px;
        }
        .upload-btn:hover {
            background-color: var(--accent-hover);
        }
        #file-input {
            display: none;
        }
        .progress-container {
            margin-top: 15px;
            display: none;
        }
        .progress-bar {
            width: 100%;
            height: 8px;
            background-color: rgba(255,255,255,0.1);
            border-radius: 4px;
            overflow: hidden;
        }
        .progress-fill {
            height: 100%;
            width: 0%;
            background-color: var(--success-color);
            transition: width 0.1s ease;
        }
        .progress-text {
            font-size: 12px;
            color: var(--text-muted);
            margin-top: 5px;
            display: flex;
            justify-content: space-between;
        }
        .status-msg {
            font-size: 14px;
            margin-top: 15px;
            text-align: center;
        }
        .status-success { color: var(--success-color); }
        .status-error { color: #ff5c5c; }
        .footer {
            text-align: center;
            font-size: 12px;
            color: var(--text-muted);
            margin-top: 25px;
        }
        .slider-group {
            margin-bottom: 15px;
        }
        .slider-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 14px;
            font-weight: 500;
            margin-bottom: 5px;
        }
        .slider-val {
            color: var(--accent-hover);
            font-weight: 600;
            font-family: monospace;
            background: rgba(127, 90, 240, 0.1);
            padding: 2px 8px;
            border-radius: 4px;
        }
        .num-input {
            width: 70px;
            background: rgba(255,255,255,0.08);
            border: 1px solid rgba(127, 90, 240, 0.4);
            border-radius: 6px;
            color: var(--accent-hover);
            font-weight: 600;
            font-family: monospace;
            font-size: 14px;
            padding: 4px 8px;
            text-align: center;
            outline: none;
            transition: border-color 0.2s, box-shadow 0.2s;
            -moz-appearance: textfield;
        }
        .num-input::-webkit-outer-spin-button,
        .num-input::-webkit-inner-spin-button {
            -webkit-appearance: none;
            margin: 0;
        }
        .num-input:focus {
            border-color: var(--accent-color);
            box-shadow: 0 0 8px rgba(127, 90, 240, 0.3);
        }
        .slider-input {
            width: 100%;
            -webkit-appearance: none;
            height: 6px;
            border-radius: 3px;
            background: rgba(255,255,255,0.1);
            outline: none;
            margin: 10px 0;
        }
        .slider-input::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 18px;
            height: 18px;
            border-radius: 50%;
            background: var(--accent-color);
            cursor: pointer;
            box-shadow: 0 0 10px var(--accent-color);
            transition: transform 0.1s ease;
        }
        .slider-input::-webkit-slider-thumb:hover {
            transform: scale(1.2);
        }
        .calc-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
            margin-top: 15px;
            margin-bottom: 20px;
        }
        .calc-card {
            background: rgba(255,255,255,0.02);
            border: 1px solid rgba(255,255,255,0.04);
            border-radius: 8px;
            padding: 12px;
            text-align: center;
        }
        .calc-val {
            font-size: 18px;
            font-weight: 700;
            color: var(--text-color);
            margin-bottom: 4px;
        }
        .calc-lbl {
            font-size: 11px;
            color: var(--text-muted);
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .alert-box {
            border-radius: 8px;
            padding: 12px 15px;
            margin-top: 10px;
            font-size: 13px;
            line-height: 1.4;
            display: none;
        }
        .alert-warning {
            background: rgba(245, 158, 11, 0.08);
            border-left: 4px solid #f59e0b;
            color: #fbbf24;
        }
        .alert-danger {
            background: rgba(239, 68, 68, 0.08);
            border-left: 4px solid #ef4444;
            color: #f87171;
        }
        .alert-icon {
            font-weight: bold;
            margin-right: 5px;
        }
        .board-wrap { text-align: center; margin-bottom: 12px; }
        .board {
            display: inline-flex;
            background: #111827;
            border-radius: 14px;
            padding: 10px 4px;
            border: 2px solid rgba(255,255,255,0.08);
            position: relative;
        }
        .pin-col { display: flex; flex-direction: column; gap: 1px; }
        .board-mid {
            width: 64px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }
        .chip {
            width: 42px; height: 42px;
            background: #1f2937;
            border-radius: 6px;
            display: flex; align-items: center; justify-content: center;
            font-size: 8px; color: #6b7280; font-weight: 700;
            letter-spacing: 1px;
            border: 1px solid #374151;
        }
        .usb-lbl {
            margin-top: 6px;
            background: #1f2937;
            padding: 2px 10px;
            border-radius: 4px;
            font-size: 7px; color: #6b7280; font-weight: 600;
        }
        .pr { display: flex; align-items: center; gap: 4px; height: 16px; padding: 0 3px; }
        .pr.l { flex-direction: row-reverse; }
        .pd {
            width: 8px; height: 8px;
            border-radius: 50%;
            background: #374151;
            flex-shrink: 0;
            transition: all 0.3s ease;
            border: 1px solid #4b5563;
        }
        .pn {
            font-family: monospace;
            font-size: 9px;
            color: #6b7280;
            white-space: nowrap;
            min-width: 28px;
            transition: color 0.3s ease;
        }
        .pr.l .pn { text-align: right; }
        .pr.r .pn { text-align: left; }
        .pr.act .pd {
            background: #2cb67d;
            border-color: #2cb67d;
            animation: pp 2s ease-in-out infinite;
        }
        .pr.act .pn { color: #2cb67d; font-weight: 700; }
        .pr.gnd .pd {
            background: #f59e0b;
            border-color: #f59e0b;
            box-shadow: 0 0 5px rgba(245,158,11,0.4);
        }
        .pr.gnd .pn { color: #f59e0b; font-weight: 700; }
        @keyframes pp {
            0%,100% { box-shadow: 0 0 4px rgba(44,182,125,0.4); }
            50% { box-shadow: 0 0 10px rgba(44,182,125,0.9); }
        }
        .board-legend {
            display: flex; justify-content: center; gap: 14px;
            margin-top: 10px; font-size: 11px; color: var(--text-muted);
        }
        .ldot {
            display: inline-block; width: 8px; height: 8px;
            border-radius: 50%; margin-right: 4px; vertical-align: middle;
        }
        .ldot.data { background: #2cb67d; box-shadow: 0 0 4px rgba(44,182,125,0.5); }
        .ldot.gnd { background: #f59e0b; box-shadow: 0 0 4px rgba(245,158,11,0.4); }
        .ldot.off { background: #374151; border: 1px solid #4b5563; }
        .pwr-note {
            text-align: center; font-size: 11px; color: var(--text-muted);
            margin-top: 8px;
        }
        .pwr-note b { color: #f59e0b; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 LED CONTROLLER</h1>
        
        <div class="section">
            <div class="section-title">Matrix Configuration</div>
            
            <div class="slider-group">
                <div class="slider-header">
                    <span>Number of LED Strips:</span>
                    <input type="number" id="num-strips" class="num-input" min="1" max="200" value="20">
                </div>
                <input type="range" id="input-strips" class="slider-input" min="1" max="200" value="20">
            </div>

            <div class="slider-group">
                <div class="slider-header">
                    <span>LEDs per Strip:</span>
                    <input type="number" id="num-leds" class="num-input" min="1" max="500" value="25">
                </div>
                <input type="range" id="input-leds" class="slider-input" min="1" max="500" value="25">
            </div>

            <div class="calc-grid">
                <div class="calc-card">
                    <div class="calc-val" id="val-total-leds">500</div>
                    <div class="calc-lbl">Total LEDs</div>
                </div>
                <div class="calc-card">
                    <div class="calc-val" id="val-fps">66 FPS</div>
                    <div class="calc-lbl">Refresh Rate</div>
                </div>
                <div class="calc-card">
                    <div class="calc-val" id="val-power">30.0 A</div>
                    <div class="calc-lbl">Max Power</div>
                </div>
            </div>
        </div>

        <div class="section">
            <div class="section-title">Smart Wiring Guide</div>
            <div class="board-wrap">
                <div class="board">
                    <div class="pin-col">
                        <div class="pr l"><span class="pn">3V3</span><span class="pd"></span></div>
                        <div class="pr l"><span class="pn">EN</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="36"><span class="pn">D36</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="39"><span class="pn">D39</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="34"><span class="pn">D34</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="35"><span class="pn">D35</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="32"><span class="pn">D32</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="33"><span class="pn">D33</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="25"><span class="pn">D25</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="26"><span class="pn">D26</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="27"><span class="pn">D27</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="14"><span class="pn">D14</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="12"><span class="pn">D12</span><span class="pd"></span></div>
                        <div class="pr l" data-gpio="13"><span class="pn">D13</span><span class="pd"></span></div>
                        <div class="pr l" data-gnd><span class="pn">GND</span><span class="pd"></span></div>
                    </div>
                    <div class="board-mid">
                        <div class="chip">ESP32</div>
                        <div class="usb-lbl">USB</div>
                    </div>
                    <div class="pin-col">
                        <div class="pr r"><span class="pd"></span><span class="pn">VIN</span></div>
                        <div class="pr r" data-gnd><span class="pd"></span><span class="pn">GND</span></div>
                        <div class="pr r" data-gpio="23"><span class="pd"></span><span class="pn">D23</span></div>
                        <div class="pr r" data-gpio="22"><span class="pd"></span><span class="pn">D22</span></div>
                        <div class="pr r"><span class="pd"></span><span class="pn">TX</span></div>
                        <div class="pr r"><span class="pd"></span><span class="pn">RX</span></div>
                        <div class="pr r" data-gpio="21"><span class="pd"></span><span class="pn">D21</span></div>
                        <div class="pr r" data-gpio="19"><span class="pd"></span><span class="pn">D19</span></div>
                        <div class="pr r" data-gpio="18"><span class="pd"></span><span class="pn">D18</span></div>
                        <div class="pr r" data-gpio="5"><span class="pd"></span><span class="pn">D5</span></div>
                        <div class="pr r" data-gpio="17"><span class="pd"></span><span class="pn">D17</span></div>
                        <div class="pr r" data-gpio="16"><span class="pd"></span><span class="pn">D16</span></div>
                        <div class="pr r" data-gpio="4"><span class="pd"></span><span class="pn">D4</span></div>
                        <div class="pr r" data-gpio="2"><span class="pd"></span><span class="pn">D2</span></div>
                        <div class="pr r" data-gpio="15"><span class="pd"></span><span class="pn">D15</span></div>
                    </div>
                </div>
                <div class="board-legend">
                    <span><span class="ldot data"></span>Data</span>
                    <span><span class="ldot gnd"></span>Ground</span>
                    <span><span class="ldot off"></span>Unused</span>
                </div>
                <div class="pwr-note">⚡ Power: <b>External 5V</b> supply — join grounds with ESP32</div>
            </div>

            <!-- Dynamic Alerts -->
            <div id="alert-container">
                <!-- Alerts inject here -->
            </div>
        </div>

        <div class="section">
            <div class="section-title">Upload Animation Bin</div>
            <div class="upload-area" id="drop-zone">
                <div class="upload-icon">📁</div>
                <div style="font-size:15px; font-weight:500; margin-bottom:5px;">Drag & Drop your .bin file here</div>
                <div style="font-size:13px; color:var(--text-muted); margin-bottom:10px;">or click to browse</div>
                <button class="upload-btn" type="button" onclick="document.getElementById('file-input').click()">Select File</button>
                <input type="file" id="file-input" accept=".bin">
            </div>
            
            <div class="progress-container" id="progress-container">
                <div class="progress-bar">
                    <div class="progress-fill" id="progress-fill"></div>
                </div>
                <div class="progress-text">
                    <span id="progress-status">Uploading...</span>
                    <span id="progress-percent">0%</span>
                </div>
            </div>
            
            <div class="status-msg" id="status-msg"></div>
        </div>
        
        <div class="footer">
            IP Address: 192.168.4.1 | Access Point: ESP32-LED-Controller
        </div>
    </div>

    <script>
        const dropZone = document.getElementById('drop-zone');
        const fileInput = document.getElementById('file-input');
        const progressContainer = document.getElementById('progress-container');
        const progressFill = document.getElementById('progress-fill');
        const progressPercent = document.getElementById('progress-percent');
        const progressStatus = document.getElementById('progress-status');
        const statusMsg = document.getElementById('status-msg');

        // Prevent defaults for drag events
        ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
            dropZone.addEventListener(eventName, preventDefaults, false);
        });

        function preventDefaults(e) {
            e.preventDefault();
            e.stopPropagation();
        }

        // Highlight drop zone
        ['dragenter', 'dragover'].forEach(eventName => {
            dropZone.addEventListener(eventName, () => dropZone.classList.add('dragover'), false);
        });

        ['dragleave', 'drop'].forEach(eventName => {
            dropZone.addEventListener(eventName, () => dropZone.classList.remove('dragover'), false);
        });

        // Handle dropped files
        dropZone.addEventListener('drop', handleDrop, false);
        fileInput.addEventListener('change', handleFileSelect, false);

        function handleDrop(e) {
            const dt = e.dataTransfer;
            const files = dt.files;
            if (files.length) uploadFile(files[0]);
        }

        function handleFileSelect(e) {
            const files = e.target.files;
            if (files.length) uploadFile(files[0]);
        }

        const CHUNK_SIZE = 256 * 1024; // 256KB chunks

        async function uploadFile(file) {
            if (!file.name.endsWith('.bin')) {
                showStatus('Please select a valid .bin animation file.', 'status-error');
                return;
            }

            progressContainer.style.display = 'block';
            statusMsg.textContent = '';
            progressFill.style.width = '0%';
            progressPercent.textContent = '0%';
            progressStatus.textContent = 'Preparing upload...';

            const totalSize = file.size;
            let offset = 0;
            let chunkIndex = 0;
            const totalChunks = Math.ceil(totalSize / CHUNK_SIZE);

            while (offset < totalSize) {
                const chunk = file.slice(offset, offset + CHUNK_SIZE);
                const isLast = (offset + CHUNK_SIZE >= totalSize) ? 1 : 0;

                try {
                    await uploadChunk(file.name, chunk, offset, isLast);
                    offset += CHUNK_SIZE;
                    chunkIndex++;

                    const percent = Math.min(100, Math.round((offset / totalSize) * 100));
                    progressFill.style.width = percent + '%';
                    progressPercent.textContent = percent + '%';
                    progressStatus.textContent = `Uploading: chunk ${chunkIndex} of ${totalChunks}...`;
                } catch (err) {
                    showStatus('Upload failed: ' + err.message, 'status-error');
                    progressStatus.textContent = 'Failed';
                    fileInput.value = '';
                    return;
                }
            }

            showStatus('Success! Animation uploaded and loaded.', 'status-success');
            progressStatus.textContent = 'Completed';
            fileInput.value = '';
        }

        function uploadChunk(filename, chunk, offset, isLast) {
            return new Promise((resolve, reject) => {
                const formData = new FormData();
                formData.append('file', chunk, 'animation.bin');

                const xhr = new XMLHttpRequest();
                xhr.open('POST', `/upload?offset=${offset}&isLast=${isLast}`, true);

                xhr.onload = function() {
                    if (xhr.status === 200) {
                        resolve();
                    } else {
                        reject(new Error(`Server returned status ${xhr.status}`));
                    }
                };

                xhr.onerror = function() {
                    reject(new Error('Connection lost'));
                };

                xhr.send(formData);
            });
        }

        const inputStrips = document.getElementById('input-strips');
        const inputLeds = document.getElementById('input-leds');
        const numStrips = document.getElementById('num-strips');
        const numLeds = document.getElementById('num-leds');
        
        const valTotalLeds = document.getElementById('val-total-leds');
        const valFps = document.getElementById('val-fps');
        const valPower = document.getElementById('val-power');
        
        const alertContainer = document.getElementById('alert-container');
        const allPinRows = document.querySelectorAll('.pr[data-gpio]');
        const gndPins = document.querySelectorAll('.pr[data-gnd]');
        
        const safePins = [2, 4, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27];
        
        function updateLayout() {
            const strips = parseInt(inputStrips.value);
            const leds = parseInt(inputLeds.value);
            
            numStrips.value = strips;
            numLeds.value = leds;
            
            const totalLeds = strips * leds;
            valTotalLeds.textContent = totalLeds.toLocaleString();
            
            const frameTimeMs = (totalLeds * 0.03) + 0.3;
            const hwFps = Math.round(1000 / frameTimeMs);
            const actualFps = Math.min(30, hwFps); // Firmware caps at 30 FPS
            
            if (actualFps < 1) {
                valFps.textContent = "<1 FPS";
            } else if (hwFps >= 30) {
                valFps.textContent = "30 FPS";
            } else {
                valFps.textContent = actualFps + " FPS (HW limited)";
            }
            
            const maxAmps = (totalLeds * 0.06).toFixed(1);
            const watts = (totalLeds * 0.06 * 5).toFixed(0);
            valPower.textContent = `${maxAmps} A`;
            
            // Clear all pin highlights
            allPinRows.forEach(p => p.classList.remove('act'));
            gndPins.forEach(p => p.classList.remove('gnd'));
            alertContainer.innerHTML = '';
            
            let alertsHTML = '';
            let activePins = [];
            
            if (strips <= 1) {
                // Serial mode — single data pin
                activePins = [2];
            } else if (strips <= 16) {
                // Parallel mode — one pin per strip
                activePins = safePins.slice(0, strips);
                const parallelFrameTimeMs = (leds * 0.03) + 0.3;
                const hwFps = Math.round(1000 / parallelFrameTimeMs);
                const fps = Math.min(30, hwFps);
                valFps.textContent = hwFps >= 30 ? '30 FPS' : fps + ' FPS (HW limited)';
            } else {
                // Chained parallel — multiple strips per pin
                activePins = safePins.slice(0, 16);
                const stripsPerPin = Math.ceil(strips / 16);
                const longestPinLeds = stripsPerPin * leds;
                const parallelFrameTimeMs = (longestPinLeds * 0.03) + 0.3;
                const hwFps = Math.round(1000 / parallelFrameTimeMs);
                const fps = Math.min(30, hwFps);
                valFps.textContent = hwFps >= 30 ? '30 FPS' : fps + ' FPS (HW limited)';
            }
            
            // Highlight active data pins on the board
            activePins.forEach(gpio => {
                const el = document.querySelector(`.pr[data-gpio="${gpio}"]`);
                if (el) el.classList.add('act');
            });
            
            // Highlight GND pins
            gndPins.forEach(p => p.classList.add('gnd'));
            
            if (totalLeds > 15000) {
                alertsHTML += `<div class="alert-box alert-danger"><span class="alert-icon">❌ OOM CRASH RISK:</span> At ${totalLeds.toLocaleString()} pixels, FastLED consumes ~${Math.round(totalLeds * 3 / 1024)}KB of RAM. Standard ESP32 WILL crash. You MUST use an <strong>ESP32-WROVER</strong> with PSRAM enabled.</div>`;
            } else if (totalLeds > 8000) {
                alertsHTML += `<div class="alert-box alert-warning"><span class="alert-icon">⚠️ MEMORY WARNING:</span> ${totalLeds.toLocaleString()} pixels requires substantial RAM. Keep other firmware tasks lightweight to prevent crashes.</div>`;
            }
            
            if (totalLeds > 600 && totalLeds <= 30000 && valFps.textContent.includes("FPS")) {
                const fpsVal = parseInt(valFps.textContent);
                if (fpsVal < 20) {
                    alertsHTML += `<div class="alert-box alert-warning"><span class="alert-icon">⚠️ LOW FRAMERATE:</span> The refresh rate is very low (${valFps.textContent}) due to long pixel chains. Consider splitting the design across multiple physical ESP32 controllers.</div>`;
                }
            }
            
            if (totalLeds > 300) {
                if (totalLeds > 3000) {
                    alertsHTML += `<div class="alert-box alert-danger"><span class="alert-icon">⚡ CRITICAL POWER INJECTION:</span> Requires up to ${maxAmps}A (${watts}W). Inject power lines every 150-200 LEDs from a high-quality 5V supply to prevent wire fires or dropouts.</div>`;
                } else {
                    alertsHTML += `<div class="alert-box alert-warning"><span class="alert-icon">⚡ POWER INJECTION REQUIRED:</span> Inject 5V power at both ends of the strips (every 150 LEDs) to avoid color shifting and dimming.</div>`;
                }
            }
            
            alertContainer.innerHTML = alertsHTML;
            const boxes = alertContainer.getElementsByClassName('alert-box');
            for (let b of boxes) {
                b.style.display = 'block';
            }
        }
        
        function saveLayout() {
            const strips = parseInt(inputStrips.value);
            const leds = parseInt(inputLeds.value);
            fetch(`/set_layout?strips=${strips}&leds=${leds}`, { method: 'POST' });
        }
        
        inputStrips.addEventListener('input', updateLayout);
        inputLeds.addEventListener('input', updateLayout);
        inputStrips.addEventListener('change', saveLayout);
        inputLeds.addEventListener('change', saveLayout);

        // Sync number input -> slider
        numStrips.addEventListener('input', function() {
            let v = parseInt(this.value) || 1;
            v = Math.max(1, Math.min(200, v));
            inputStrips.value = v;
            updateLayout();
        });
        numStrips.addEventListener('change', function() {
            let v = parseInt(this.value) || 1;
            v = Math.max(1, Math.min(200, v));
            this.value = v;
            inputStrips.value = v;
            updateLayout();
            saveLayout();
        });
        numLeds.addEventListener('input', function() {
            let v = parseInt(this.value) || 1;
            v = Math.max(1, Math.min(500, v));
            inputLeds.value = v;
            updateLayout();
        });
        numLeds.addEventListener('change', function() {
            let v = parseInt(this.value) || 1;
            v = Math.max(1, Math.min(500, v));
            this.value = v;
            inputLeds.value = v;
            updateLayout();
            saveLayout();
        });

        updateLayout();

        function showStatus(text, className) {
            statusMsg.className = 'status-msg ' + className;
            statusMsg.textContent = text;
        }
    </script>
</body>
</html>
)rawhtml";

// Layout Persistence Helpers
void saveLayoutConfig(int strips, int leds_per_strip) {
  File configFile = LittleFS.open("/layout.txt", FILE_WRITE);
  if (configFile) {
    configFile.println(strips);
    configFile.println(leds_per_strip);
    configFile.close();
    Serial.printf("Saved layout to flash: %d strips, %d leds/strip\n", strips, leds_per_strip);
  } else {
    Serial.println("ERROR: Failed to save layout to flash!");
  }
}

void loadLayoutConfig() {
  if (LittleFS.exists("/layout.txt")) {
    File configFile = LittleFS.open("/layout.txt", "r");
    if (configFile) {
      String stripsStr = configFile.readStringUntil('\n');
      String ledsStr = configFile.readStringUntil('\n');
      configFile.close();
      
      int strips = stripsStr.toInt();
      int leds_per_strip = ledsStr.toInt();
      
      if (strips > 0 && leds_per_strip > 0) {
        active_leds = strips * leds_per_strip;
        if (active_leds > MAX_LEDS) active_leds = MAX_LEDS;
        Serial.printf("Loaded layout from flash: %d strips, %d leds/strip (Total: %d LEDs)\n", strips, leds_per_strip, active_leds);
        return;
      }
    }
  }
  // Default fallback
  active_leds = 500;
  Serial.println("No layout config found, using default 500 LEDs.");
}

void handleSetLayout() {
  String stripsStr = server.arg("strips");
  String ledsStr = server.arg("leds");
  
  if (stripsStr.length() > 0 && ledsStr.length() > 0) {
    int strips = stripsStr.toInt();
    int leds_per_strip = ledsStr.toInt();
    
    if (strips > 0 && leds_per_strip > 0) {
      active_leds = strips * leds_per_strip;
      if (active_leds > MAX_LEDS) active_leds = MAX_LEDS;
      
      // Update FastLED to only push active_leds (huge performance gain)
      if (ledController) ledController->setLeds(leds, active_leds);
      
      saveLayoutConfig(strips, leds_per_strip);
      server.send(200, "text/plain", "Layout updated");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid layout parameters");
}

// Web server callbacks
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  
  String offsetStr = server.arg("offset");
  String isLastStr = server.arg("isLast");
  
  size_t offset = offsetStr.length() > 0 ? (size_t)offsetStr.toInt() : 0;
  bool isLast = isLastStr.length() > 0 ? (isLastStr.toInt() == 1) : true;
  
  if (upload.status == UPLOAD_FILE_START) {
    isUploading = true;
    
    // Turn off LEDs once at the start of the upload
    if (offset == 0) {
      fill_solid(leds, MAX_LEDS, CRGB::Black);
      FastLED.show();
      Serial.println("\n>>> Starting new file upload...");
    }
    
    // Close animation file before writing/appending
    if (animationFile) {
      animationFile.close();
    }
    
    // Open in write mode for the first chunk, append mode for subsequent chunks
    if (offset == 0) {
      animationFile = LittleFS.open("/animation.bin", FILE_WRITE);
    } else {
      animationFile = LittleFS.open("/animation.bin", FILE_APPEND);
    }
    
    if (!animationFile) {
      Serial.printf("ERROR: Failed to open /animation.bin at offset %u!\n", offset);
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (animationFile) {
      animationFile.write(upload.buf, upload.currentSize);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (animationFile) {
      animationFile.close();
    }
    
    if (isLast) {
      Serial.println(">>> File upload completed successfully!");
      isUploading = false;
      
      // Open newly uploaded file for animation playback
      animationFile = LittleFS.open("/animation.bin", "r");
      if (!animationFile) {
        Serial.println("ERROR: Could not open /animation.bin for reading!");
      } else {
        animationFile.seek(0);
        Serial.println(">>> Animation file loaded, ready to play.");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting ESP32 LED Web Controller AP...");

  // Mount LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: LittleFS Mount Failed");
    return;
  }

  // Load active layout settings
  loadLayoutConfig();

  // Setup FastLED with only active_leds (avoids pushing 6000 LEDs per frame)
  ledController = &FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, active_leds);
  FastLED.setBrightness(255);
  fill_solid(leds, active_leds, CRGB::Black);
  FastLED.show();

  // Load existing animation if it exists
  animationFile = LittleFS.open("/animation.bin", "r");
  if (animationFile) {
    animationFile.seek(0);
    Serial.println(">>> Found existing animation.bin, ready to play.");
  } else {
    Serial.println(">>> No animation.bin found. Waiting for upload.");
  }

  // Setup Wi-Fi Connection
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/set_layout", HTTP_POST, handleSetLayout);
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "Upload Complete");
  }, handleFileUpload);

  server.begin();
  Serial.println("Web Server started!");
}

void loop() {
  server.handleClient();

  if (isUploading) {
    delay(10);
    return;
  }

  if (!animationFile) {
    // If no file loaded, display an idle green indicator at pixel 0
    fill_solid(leds, active_leds, CRGB::Black);
    leds[0] = CRGB::Green;
    FastLED.show();
    delay(500);
    return;
  }

  // Read frame data directly into FastLED memory array in one single block read
  int frameSize = active_leds * 3;
  if (animationFile.available() < frameSize) {
    animationFile.seek(0); // Loop back to first frame
  }

  int bytesRead = animationFile.read((uint8_t*)leds, frameSize);
  
  if (bytesRead != frameSize) {
    // File read error, reset seek and try next iteration
    animationFile.seek(0);
    delay(10);
    return;
  }

  FastLED.show();
  delay(FRAME_INTERVAL_MS); // ~30 FPS — delay yields CPU properly
}