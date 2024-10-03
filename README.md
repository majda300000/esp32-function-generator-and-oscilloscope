
# 🚀 Function Generator and Oscilloscope on ESP32 Bytelab DevKit

Welcome to the **ESP32 Function Generator and Oscilloscope** project, developed as part of the **Bytelab Software Academy 2024** final task! This project combines the power of an oscilloscope with a function generator, packed with versatile features and built for the **ESP32** platform.

## 📋 Overview
This project includes:
- **Oscilloscope**: Capable of measuring and displaying two signals in real-time.
- **Function Generator**: Generates 4 waveform types with adjustable frequency, amplitude, and duty cycle.

Perfect for anyone looking to explore signal analysis and waveform generation with an easy-to-use interface that supports both touchscreen and physical controls!

## 🔌 Pinout
The pin connections for the oscilloscope and function generator are as follows:

| Component          | Pin       | ESP32 Pin |
|--------------------|-----------|-----------|
| **Oscilloscope CH1**| BTN_3     | GPIO33    |
| **Oscilloscope CH2**| BTN_2     | GPIO32    |
| **Function Output** | BTN_4     | GPIO25    |

## 🎮 Controls
- **Touchscreen**: Directly interact with on-screen elements.
- **Joystick**: Navigate through options by moving left/right.
- **Button (BTN_1)**: Select an item or confirm an action.

## 🛠️ Requirements
Before getting started, ensure you have the following:
- **ESP-IDF** version 5.0 or higher.
- **LVGL** library version 8.3.\*

## 🚀 Getting Started

### Project Structure
This project utilizes two key submodules:
- **[LVGL](https://github.com/lvgl/lvgl)**: A powerful graphics library.
- **[LVGL ESP32 Drivers](https://github.com/lvgl/lvgl_esp32_drivers)**: Hardware drivers for LVGL. *(Note: A custom patch for version 8.3 is provided)*.

### Build and Run

1. **Clone the repository**:
   ```bash
   git clone --recursive ssh://git@git.byte-lab.com:2222/byte-lab/academy/blesa/blesa-2024/majda-bakmaz/mashina.git
   ```
2. **Apply the LVGL 8.3 patch**:
   ```bash
   cd mashina/components/lvgl_esp32_drivers
   git apply --ignore-space-change --ignore-whitespace ../lvgl_esp32_drivers_8-3.patch
   ```
4. **Set up ESP-IDF environment**:
   ```bash
   . $HOME/esp/<path-to-your-idf-folder>/export.sh
   ```
5. **Build the project**:
   ```bash
   idf.py build
   ```
6. **Flash the project** to your ESP32:
   ```bash
   idf.py flash /dev/ttyUSB0
   ```
   ⚠️ Ensure the **TCH_IRQ switch** is set to `OFF` if using the ByteLab Development Kit.
7. **Monitor the output**:
   ```bash
   idf.py monitor -p /dev/ttyUSB0
   ```

## 📐 Features

### Function Generator
- **Waveforms Supported**: 
  - Sine
  - Square
  - Triangle
  - Sawtooth
- **Adjustable Parameters**:
  - Frequency
  - Amplitude (0V to Vmax)
  - Duty Cycle (0% to 100%)
- **On-screen visualization** of the generated waveform.
- **Presets**:
    - Up to 5 saveable presets

### Oscilloscope
- **Real-time waveform display** for two channels.
- **Adjustable Settings**:
  - Time Base: 10 ms/div to 1 ms/div
  - Voltage Scale: 500mV/div to 100V/div
  - Channel Selection: CH1 or CH2

### Other Features
- **Temperature & Humidity Monitoring**:
  - Automatic shutdown if temperature exceeds 32°C.
- **LED Signalization** for device states: 
    - device turned on **GREEN LED on** 
    - Function Generator on **RED LED fast blink**
    - Oscilloscope on **RED LED on**
    - Wi-Fi connected **BLUE LED on**

## ⚡ Features in Development
- Historical data logging for temperature and humidity.
- Enhanced Wi-Fi features for screenshot sharing and provisioning.


## 📖 Doxygen Documentation

The HTML documentation for this project is generated using [Doxygen](https://www.doxygen.nl/). To view the documentation:

1. Navigate to the `docs/html/` folder 
2. Open the `index.html` file in a web browser to access the full project documentation.