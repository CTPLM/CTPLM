# Camera Timing Parameter Estimation using Off-the-shelf Hardware 

## Abstract
Good estimation of unknown camera timing parameters (exposure time, frame rate, and, in the case of rolling shutter sensors, their readout speed) is essential for many computer vision tasks, including capturing fast-moving objects and synchronizing multiple sensors. However, these parameters are often undocumented, unknown, or unreliable in consumer-grade cameras. In this paper, we present a simple and low-cost method for reading timing parameters using an off-the-shelf $8\times8$ LED matrix and a microcontroller that displays time-varying patterns recorded by the camera under test. These patterns consist of running LED counters, Gray code counters, and running LED columns. We exploit the interaction between rolling shutter and fast-blinking LEDs. We demonstrate the effectiveness of the proposed method and show that the timing parameters can be accurately estimated.

## Hardware Preparation

- Arduino Nano ESP32 (https://docs.arduino.cc/hardware/nano-esp32/)
- LED matrix $8\times8$, such as 1088BS (https://www.aliexpress.com/item/1005007646761968.html)
- Soldering board / Non-soldering contact field
- Suitable current-limiting resistors (16) - $40\,\Omega$ 
- DuPont F-F cables or similar (16)

<p align="center">
  <img src="images/hw_layout.pdf" div align=center>
</p> 

## Software Preparation

1. Download [Arduino IDE](https://docs.arduino.cc/software/ide/)
2. Clone this repository
3. From Arduino IDE, open folder "readoutSpeed", "expTime", or "fps".
4. **Switch pin numbering (Tools->Pin Numbering->By GPIO number (legacy)) ** 
5. Connect device to computer and Upload

