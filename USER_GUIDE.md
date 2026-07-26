# LED Matrix Animation Player — User & Operator Guide

A complete step-by-step manual on using **`animation_viewer.html`** to play, align, inspect, and troubleshoot raw `.bin` animation files for LED matrices.

---

## 1. Quick Start Guide

1. **Open Web App**: Double-click **`animation_viewer.html`** in any web browser (Chrome, Edge, Firefox, Safari).
2. **Load `.bin` File**: Drag and drop your `.bin` animation file into the dropzone or click to choose a file.
3. **Set Matrix Dimensions**: Click **⚡ Run Auto-Detect** or enter Width x Height manually.
4. **Play & Preview**: Click **Play**, scrub the timeline, or adjust playback speed.

---

## 2. Matrix Dimensions: Auto-Detect vs. Manual Input

### A. Auto-Detect Dimensions (Recommended First Step)
- Click **⚡ Run Auto-Detect** in the blue sidebar panel.
- **How Auto-Detect works**:
  1. Inspects filename patterns (e.g. `25X20-ISLAMI_0_orig.bin` $\rightarrow$ extracts `25 x 20`).
  2. Parses binary header metadata bytes for width and height values.
  3. Calculates total LED factor ratios.
- **When it works**: Ideal for files with descriptive filenames or embedded metadata headers.
- **When to use manual input**: If a file is headerless or has a generic name (like `data.bin`), Auto-Detect may suggest a default ratio. In that case, enter dimensions manually.
- Click **✓ Apply Detected Dimensions** to set.

### B. Manual Matrix Input (Always Available)
- Type the **Width (Columns)** and **Height (Rows)** directly in the inputs.
- Or select from common presets (20x25, 25x20, 32x32, 64x64, 72x72, 120x150).
- **Note**: Manual inputs are **never removed** and will always override auto-detection when modified.

---

## 3. Binary Decoding & Alignment Options

| Setting | Options | When to Use |
|---|---|---|
| **Header Offset** | `0 Bytes` (Default), `512 Bytes`, `Auto` | Set to `0 Bytes` for raw headerless files. Set to `512 Bytes` for files exported from tools like Jinx!/Glediator that include 512B headers. |
| **Color Order** | `RGB`, `GRB`, `BGR` | Use `RGB` for standard display. Switch to `GRB` if red and green colors appear swapped. |
| **Wiring Layout** | `Progressive`, `Serpentine` | Use `Progressive` if rows run left-to-right. Use `Serpentine` if physical wiring zigzags back and forth every row. |
| **Start Corner** | Top-Left, Bottom-Left, Top-Right, Bottom-Right | Adjust if the animation appears upside-down or horizontally mirrored. |
| **Rotation** | `0°`, `90°`, `180°`, `270°`, `↻ Rotate 90°` Button | Rotates the canvas view in real time without reloading the file. |

---

## 4. Playback & Speed Controls

- **Play / Pause**: Toggle animation playback.
- **Timeline Scrubber**: Drag to scrub to any frame in the file.
- **Step Controls**: Use `Step -1` and `Step +1` for frame-by-frame inspection.
- **Speed Multipliers**: Use the speed dropdown on the player bar (`0.25x Slow Mo`, `0.5x`, `1.0x Normal`, `2.0x`, `4.0x Ultra Fast`) or the **FPS Slider** (1-60 FPS) in the sidebar.
- **Pixel Size & Glow**: Adjust pixel size slider and toggle NeoPixel glow effect for visual realism.

---

## 5. Troubleshooting Visual Distortion Issues

| Symptom | Root Cause | How to Fix |
|---|---|---|
| **Scrambled, diagonal, or skewed lines** | Header offset or matrix dimensions mismatch | Toggle Header Offset between **0 Bytes** and **512 Bytes**, or verify matrix Width x Height. |
| **Colors look incorrect (e.g. Red is Green)** | Color channel order mismatch | Change Color Order from **RGB** to **GRB** or **BGR**. |
| **Alternating rows look backward or wavy** | Serpentine vs Progressive layout mismatch | Switch Matrix Wiring Layout from **Progressive** to **Serpentine**. |
| **Image is upside down or mirrored** | Start corner or rotation angle mismatch | Use **Start Corner** or click **↻ Rotate 90°**. |
