# IoT-Based Air Quality Monitoring Index (AQMI) & Continuous Logger

[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/)
[![Sensors: MQ2 | BMP280](https://img.shields.io/badge/Sensors-MQ2%20%7C%20BMP280-green.svg)]()
[![Storage: MicroSD SPI](https://img.shields.io/badge/Storage-MicroSD%20SPI-orange.svg)]()
[![IoT: Blynk](https://img.shields.io/badge/IoT-Blynk%20Cloud-00bcd4.svg)](https://blynk.io/)

A complete, dual-tier environmental monitoring and automated data-logging embedded system built around the **ESP32-WROOM-32UE**. The system unifies multi-protocol sensor interfacing (Analog ADC, I²C, and SPI) with local high-resolution CSV logging and live mobile dashboard telemetry.

---

## 1. Problem Statement & Architecture

### The Gap Addressed
Many existing IoT weather and air pollution monitoring stations stream readings only to the cloud. When network outages, Wi-Fi drops, or server downtimes occur, critical historical pollution trends are permanently lost. 

### Solution Overview
This system couples real-time cloud streaming with deterministic **on-device 5-second MicroSD data logging**, ensuring continuous, uninterruptible historical records alongside instant smartphone alerting.

```
┌──────────────┐     Analog (ADC)
│  MQ-2 Gas /  ├──────────────────┐
│ Smoke Sensor │                  │
└──────────────┘                  ▼
┌──────────────┐     I2C (GPIO21/22)┌────────────────┐  Wi-Fi   ┌──────────────┐
│ GY-BMP280    ├───────────────────►│  ESP32-WROOM-  ├────────►│ Blynk Mobile │
│ Temp/Pressure│                    │      32UE      │         │  Dashboard   │
└──────────────┘                    └───────┬────────┘         └──────────────┘
                                            │
                                            │ SPI (GPIO5/18/19/23)
                                            ▼
                                    ┌────────────────┐
                                    │ MicroSD Card   │
                                    │ (5s CSV Log)   │
                                    └────────────────┘
```

---

## 2. Hardware Bill of Materials & Wiring

| Component | Interface / Protocol | ESP32 Pin Assignment | Operating Voltage | Function |
| :--- | :---: | :---: | :---: | :--- |
| **ESP32-WROOM-32UE** | Central MCU | — | $5\text{V} / 3.3\text{V}$ | 240 MHz dual-core processor, ADC, SPI & I²C master |
| **MQ-2 Gas Sensor** | Analog ADC | `GPIO 34` (ADC1_CH6) | $5\text{V}$ | Detects LPG, smoke, methane, butane, and hydrogen |
| **GY-BMP280** | I²C Serial Data | `SDA = GPIO 21` | $3.3\text{V}$ | Digital temperature (°C) & atmospheric pressure (hPa) |
| **GY-BMP280** | I²C Serial Clock | `SCL = GPIO 22` | $3.3\text{V}$ | Clock synchronization for BMP280 |
| **MicroSD Module** | SPI Chip Select | `CS = GPIO 5` | $5\text{V}$ | Slave select for SD SPI bus |
| **MicroSD Module** | SPI Clock | `SCK = GPIO 18` | $5\text{V}$ | Master clock |
| **MicroSD Module** | SPI MISO | `MISO = GPIO 19` | $5\text{V}$ | Master In Slave Out data line |
| **MicroSD Module** | SPI MOSI | `MOSI = GPIO 23` | $5\text{V}$ | Master Out Slave In data line |

### Complete Breadboard Connection Diagram
Refer to [`docs/circuit_schematic.jpg`](docs/circuit_schematic.jpg) for the pin-verified breadboard wiring schematic.

---

## 3. Gas Concentration Calibration Modeling

The MQ-2 analog sensor voltage changes proportionally to target gas concentration. The firmware translates raw 12-bit ADC values ($0 \to 4095$) into calibrated parts-per-million ($\text{ppm}$) using empirical power-law formulas:

$$\text{Concentration (ppm)} = K \cdot (\text{ADC}_{\text{raw}})^n$$

| Gas Component | Proportionality Constant ($K$) | Power Exponent ($n$) |
| :--- | :---: | :---: |
| **Smoke** | $0.0035$ | $0.75$ |
| **LPG** | $0.0030$ | $0.75$ |
| **Butane** | $0.0008$ | $0.70$ |
| **Methane** | $0.0006$ | $0.70$ |
| **Hydrogen** | $0.0005$ | $0.70$ |

---

## 4. Live Demonstration & Verification

### Smoke Exposure Stress Test
In live testing, controlled smoke exposure adjacent to the MQ-2 sensor demonstrated instantaneous response:
- **Baseline Smoke**: $\sim 300\text{ ppm}$
- **Peak Exposure**: **$2559\text{ ppm}$**
- **Atmospheric Pressure**: $985\text{ hPa}$
- **Ambient Temperature**: $29^\circ\text{C}$
- Full video available in [`media/smoke_detection_hardware_demo.mp4`](media/smoke_detection_hardware_demo.mp4).

---

## 5. Repository File Structure

```text
IoT-Air-Quality-Monitoring-System/
├── README.md
├── .gitignore
│
├── firmware/
│   └── air_quality_logger.ino             # Production calibrated Arduino sketch
│
├── docs/
│   ├── circuit_schematic.jpg              # High-resolution breadboard wiring diagram
│   ├── IoT_Air_Quality_Monitoring_Index_Presentation.pdf   # 24-slide Analog Circuits PPT
│   └── MPMC_Project_Presentation.pptx     # 25-slide Microcontrollers project presentation
│
└── media/
    └── smoke_detection_hardware_demo.mp4  # 97-second live hardware smoke test video
```

---

## 6. How to Run & Flash

1. **Hardware Assembly**: Wire components according to the table in Section 2.
2. **Arduino IDE Configuration**:
   - Board: `ESP32 Dev Module`
   - Flash Frequency: `80MHz`
   - Upload Speed: `921600`
3. **Required Libraries**:
   - `Adafruit BMP280 Library`
   - `Adafruit Unified Sensor`
   - `SD` and `SPI` (bundled with ESP32 core)
4. Open [`firmware/air_quality_logger.ino`](firmware/air_quality_logger.ino) and upload. Open Serial Monitor at **115200 baud** to view real-time calibrated telemetry.
