# 📘 Magnetometer-Driven Digital Compass with an Interactive GUI on the STM32f429-Discovery

This repository contains the implementation of a digital compass built using an STM32F429-Discovery board and a QMC5883L/GY-271 magnetometer, programmed with the [libopencm3 library](https://libopencm3.org/).
The project features a fully custom graphical user interface (GUI) rendered on the board’s embedded display, providing real-time heading visualization through both dynamic and static elements.

---

## 📦 Requirements

### **Hardware**

* STM32F429-Discovery board
* QMC5883L 3-axis magnetometer
* Mini-USB cable

### **Software**

* Linux
* `gcc-arm-none-eabi`
* `make`
* `OpenOCD`
* `libopencm3`

---

## ⚙️ Installation

### 1. **Clone the repository**

```bash
git clone https://github.com/josveji/STM32-COMPASS.git
cd STM32-COMPASS
```

### 2. **Install dependencies**

Run the commands below on your Linux terminal to install the dependencies: 

```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi openocd make stlink-tools stlink-gui gdb-multiarch minicom emacs git
```

```bash
git submodule init
git submodule update
cd libopencm3
make -j`nproc`
cd ..
cd libopencm3-plus
make -j`nproc`
```

---

## 🛠️ Compilation

Navigate to the following directory by running the command below:

```bash
cd /src/stm32/f4/stm32f429idiscovery/compass
```
### **Build the firmware**

```bash
make
```

### **Clean build files**

```bash
make clean
```

---

## 🔌 Flashing the Device

### **Flash using OpenOCD**

```bash
make flash
```
---

## ▶️ Usage

1. Make the connections: Wire the components according to the provided schematic.

2. Start the system: Power the STM32F429-Discovery board; the firmware initializes the display and magnetometer automatically.

3. On-screen behavior: A compass rose, rotating arrow, and zero-padded heading (e.g., 045°) appear on the display.

4. Interaction: No buttons are required. Rotate the board and the GUI will update based on magnetometer data.

5. Data shown: Real-time heading angle and cardinal directions.

6. Expected behavior: Smooth arrow rotation with a small delay due to GUI rendering and sensor processing.

---

## 📚 Repository Structure

```
/src/stm32/f4/stm32f429idiscovery/compass → Source code
/libopencm3 → External dependencies
/libopencm3-plus → External dependencies
/hook → Automatically generated scripts or functions
README.md → This file
```

---


## 📄 License

MIT License

Copyright (c) 2025 Josué M. Jiménez Ramírez & Gabriel J. Vega Chavez

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.