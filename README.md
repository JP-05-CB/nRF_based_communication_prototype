# nRF-Based ADPCM Walkie-Talkie Prototype 📻

![ESP32](https://img.shields.io/badge/ESP32-Hardware-blue)
![nRF24L01](https://img.shields.io/badge/nRF24L01-Wireless-green)
![Status](https://img.shields.io/badge/Status-Completed-success)

A low-bandwidth, long-range wireless communication prototype built using ESP32 microcontrollers and nRF24L01 transceiver modules. This project acts as a functional walkie-talkie, achieving a communication range of up to 100 meters. 

To optimize performance, this project implements **ADPCM (Adaptive Differential Pulse Code Modulation)**, which was first simulated in MATLAB before hardware deployment. 

## 🌟 Key Features
* **Bandwidth Efficiency:** Utilizes ADPCM modulation to significantly reduce bandwidth usage compared to standard PCM.
* **Low Quantization Error:** Maintains clear audio quality while compressing the data stream.
* **Long Range:** Achieves up to 100 meters of wireless communication using the nRF modules.
* **MATLAB Verified:** Core modulation logic was simulated and verified in MATLAB prior to hardware implementation.
* **Half-Duplex Communication:** Push-to-talk functionality mirroring a real walkie-talkie.

## 🛠️ Hardware Specifications
To replicate this project, you will need two identical sets of the following components (one for each walkie-talkie node):
* 2x **ESP32 WROOM** Development Boards
* 2x **nRF24L01** Wireless Modules
* 2x **Microphone Modules** (MAX4466 or IN4444)
* 2x **MAX98357A** I2S Audio Amplifiers
* 2x Standard Push Buttons
* 2x Small Speakers (compatible with the MAX98357A)

## ⚡ Wiring Setup
Since both walkie-talkie nodes are identical, make sure both ESP32 boards have all components wired exactly like this:

| Component | Pin / Designation | ESP32 Pin / Connection |
| :--- | :--- | :--- |
| **Mic (MAX4466)** | OUT | `D34` |
| | VCC | `3.3V` |
| | GND | `GND` |
| **Push Button** | Leg 1 | `D13` |
| | Leg 2 | `GND` |
| **Amp (MAX98357A)**| LRC | `D25` |
| | BCLK | `D26` |
| | DIN | `D22` |
| | Vin | `5V` |
| | GND | `GND` |
| **NRF24L01** | CE | `D4` |
| | CSN | `D5` |
| | SCK | `D18` |
| | MISO | `D19` |
| | MOSI | `D23` |
| | VCC | `3.3V` *(Do not connect to 5V!)* |
| | GND | `GND` |

> **Note:** The nRF24L01 requires a stable 3.3V power supply. If you experience dropping connections, consider adding a 10µF to 100µF capacitor across the VCC and GND pins of the nRF module.

## 🧠 How It Works (ADPCM Modulation)
Standard raw audio (PCM) requires a high data rate, which easily overwhelms the limited payload sizes of NRF modules. 

This project solves that by using **ADPCM**. Instead of sending the absolute value of the audio sample, ADPCM only transmits the *difference* between the current sample and the previous one. Because the step size adapts dynamically to the audio signal, it drastically reduces the required bandwidth and minimizes quantization errors, making it perfect for microcontroller-based RF communication.

## 🚀 Getting Started
1. **Clone the repository:**
   ```bash
   git clone [https://github.com/JP-05-CB/nRF_based_communication_prototype.git](https://github.com/JP-05-CB/nRF_based_communication_prototype.git)
Review MATLAB Simulations: Check the /simulation folder (if applicable) to see the ADPCM algorithm tested on sample audio arrays.

Hardware Assembly: Wire both ESP32 units according to the table above.

Flash the ESP32s: * Open the .ino / .cpp source files in your preferred IDE (Arduino IDE, PlatformIO, etc.).

Ensure you have the necessary libraries installed (e.g., RF24 by TMRh20).

Upload the code to both ESP32 boards.

Test: Press and hold the push button on Unit A to speak, and listen to the output on Unit B's speaker!

🤝 Contributing
Contributions, issues, and feature requests are welcome! Feel free to check the issues page if you want to contribute.

📝 License
MIT License (or mention your specific license here)
