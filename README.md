# LoRa-Based-IoT-Network-for-Smart-Applications
LoRa-based IoT system for long-range water level monitoring and gas leak detection using ESP32 + SX1278.

## Overview
This project designs a long-range low-power IoT network using LoRa technology for:

- Water level monitoring
- Gas leak detection
- Real-time alerts
- Remote dashboard

Built using:
ESP32 + SX1278 + Ultrasonic Sensor + MQ2 Gas Sensor + LoRa Gateway

---

## Features
- Long range communication (LoRa)
- Battery powered nodes
- Real-time monitoring
- Buzzer + LED alerts
- Cloud dashboard support
- Scalable multi-node network

---

## System Architecture
Sensor Node → LoRa → Gateway → Cloud → Dashboard

---

## Hardware
- ESP32
- SX1278 LoRa module
- JSN-SR04T Ultrasonic sensor
- MQ2 Gas sensor
- OLED display
- Buzzer + LED
- 7805 regulator

---

## Folder Structure
/docs → diagrams & report  
/hardware → PCB & schematics  
/firmware → ESP32 code  
/software → dashboard  

---

## Current Status
✔ Report completed  
✔ PCB designed  
✔ Simulation tested  
✔ Components purchased  
⏳ Hardware assembly in progress  
⏳ Gateway setup  
⏳ Dashboard development  
---
## Technical Specifications

Communication:
- Frequency: 433 MHz / 868 MHz / 915 MHz
- Modulation: LoRa (Chirp Spread Spectrum)
- Range: 2–5 km (LOS)

Microcontroller:
- ESP32 (Dual core, 240 MHz)

Sensors:
- Ultrasonic JSN-SR04T (2–400 cm range)
- MQ2 Gas Sensor

Power:
- 3.7V Li-ion battery
- Deep sleep enabled

Protocol:
- Point-to-Point LoRa communication


---

## Future Work
- Cloud integration
- Mobile alerts
- More sensors
- Solar powered nodes

---

## Author
Yash Bachhav  
ENTC Final Year  
