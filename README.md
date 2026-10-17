# IoT-Based Air Quality Monitoring Index (AQMI) & Continuous Logger

[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32--WROOM--32UE-blue.svg)](https://www.espressif.com/)
[![Sensors: MQ2 | BMP280](https://img.shields.io/badge/Sensors-MQ2%20%7C%20BMP280-green.svg)](docs/circuit_schematic.jpg)
[![Storage: MicroSD SPI](https://img.shields.io/badge/Storage-MicroSD%20SPI%20FAT32-orange.svg)](firmware/air_quality_logger.ino)
[![Framework: Arduino / PlatformIO](https://img.shields.io/badge/Framework-PlatformIO%20%7C%20Arduino-darkgreen.svg)](platformio.ini)
[![License: MIT](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

An industrial-grade embedded telemetry station built around the **ESP32-WROOM-32UE** microcontroller. The system combines multi-protocol sensor interfacing (Analog ADC, I²C, and SPI) with fault-tolerant, deterministic on-device MicroSD CSV data logging and real-time cloud streaming via Blynk IoT.

---

## 1. Architecture & Design Rationale

### The Industrial Reliability Problem
Standard commercial IoT environmental stations depend entirely on active Wi-Fi and cloud servers. If network dropouts, Wi-Fi jamming, or gateway resets occur, localized pollution anomalies (such as industrial gas leaks or smoke flare-ups) go unrecorded.

### Solution Architecture
This architecture decouples sensor acquisition and persistent storage from wireless telemetry:
- **Core 0 / Deterministic Loop**: Samples the multi-gas MQ-2 sensor via 12-bit ADC and barometric pressure/temperature via I²C at strict 5-second intervals, writing calibrated CSV records directly to SPI MicroSD storage.
- **Core 1 / Telemetry Loop**: Asynchronously transmits non-blocking telemetry frames to cloud dashboards when network connectivity is available, guaranteeing zero data loss during network outages.

```
+-------------------+      Analog (ADC1_CH6 / GPIO34)
|  MQ-2 Gas /       |------------------------------------+
|  Smoke Sensor     |                                    |
+-------------------+                                    v
+-------------------+      I2C (SDA: GPIO21, SCL: GPIO22)+-----------------------+   Wi-Fi 802.11b/g/n   +-----------------------+
|  GY-BMP280        |----------------------------------->|  ESP32-WROOM-32UE     |---------------------->|  Blynk Cloud /        |
|  Temp & Pressure  |                                    |  (Xtensa Dual-Core)   |                       |  Mobile Dashboard     |
+-------------------+                                    +-----------+-----------+                       +-----------------------+
                                                                     |
                                                                     | SPI (CS: GPIO5, SCK: GPIO18, MISO: GPIO19, MOSI: GPIO23)
                                                                     v
                                                         +-----------------------+
                                                         |  MicroSD FAT32 Module |
                                                         |  (Deterministic CSV)  |
                                                         +-----------------------+
```

---

## 2. Hardware Bill of Materials & Pin Configuration

| Subsystem | Component | Protocol / Interface | ESP32 Pinout | Operating Voltage | Engineering Specifications |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **Processor** | ESP32-WROOM-32UE | Central Controller | -- | 3.3V / 5.0V | 240 MHz dual Xtensa LX6, 520 KB SRAM, external IPEX antenna |
| **Gas Sensing** | MQ-2 Semiconductor Sensor | Analog Voltage | `GPIO 34` (ADC1_CH6) | 5.0V | Wide-spectrum detection: LPG, smoke, methane, butane, hydrogen |
| **Barometric** | Bosch Sensortec BMP280 | I²C Serial Data | `SDA = GPIO 21` | 3.3V | Range: 300 to 1100 hPa ($\pm 1.0\text{ hPa}$ absolute accuracy) |
| **Barometric** | Bosch Sensortec BMP280 | I²C Serial Clock | `SCL = GPIO 22` | 3.3V | Fast-mode I²C at 400 kHz |
| **Local Storage** | MicroSD Adapter | SPI Chip Select | `CS = GPIO 5` | 5.0V | Dedicated hardware SPI bus |
| **Local Storage** | MicroSD Adapter | SPI Clock | `SCK = GPIO 18` | 5.0V | SPI bus clock |
| **Local Storage** | MicroSD Adapter | SPI MISO | `MISO = GPIO 19` | 5.0V | Master In Slave Out data line |
| **Local Storage** | MicroSD Adapter | SPI MOSI | `MOSI = GPIO 23` | 5.0V | Master Out Slave In data line |

Refer to [`docs/circuit_schematic.jpg`](docs/circuit_schematic.jpg) for the verified breadboard prototype wiring schematic.

---

## 3. Mathematical Calibration Models

The MQ-2 sensor's conductivity increases with target gas concentration. The onboard 12-bit ADC converts the sensor load resistor voltage ($V_{\text{out}}$) into an integer value ($0 \le \text{ADC} \le 4095$). The firmware computes gas concentrations using empirical power-law modeling calibrated against baseline resistance:

$$\text{Concentration (ppm)} = K \cdot (\text{ADC}_{\text{raw}})^n$$

| Target Gas Species | Proportionality Constant ($K$) | Power-Law Exponent ($n$) | Dynamic Range |
| :--- | :---: | :---: | :---: |
| **Smoke** | $0.0035$ | $0.75$ | $200 - 5000\text{ ppm}$ |
| **Liquefied Petroleum Gas (LPG)** | $0.0030$ | $0.75$ | $200 - 5000\text{ ppm}$ |
| **Butane ($C_4H_{10}$)** | $0.0008$ | $0.70$ | $300 - 5000\text{ ppm}$ |
| **Methane ($CH_4$)** | $0.0006$ | $0.70$ | $500 - 10000\text{ ppm}$ |
| **Hydrogen ($H_2$)** | $0.0005$ | $0.70$ | $300 - 5000\text{ ppm}$ |

---

## 4. Hardware Validation & Empirical Results

In live experimental stress testing with controlled smoke exposure:
- **Baseline Smoke**: $258.71\text{ ppm}$ (ADC: 310)
- **Peak Exposure**: **$1866.65\text{ ppm}$** (ADC: 4095 saturation)
- **Dynamic Elevation**: **$7.21\times$** baseline concentration
- **Atmospheric Pressure**: $985.20\text{ hPa} \pm 0.3\text{ hPa}$
- **Ambient Temperature**: $28.5^\circ\text{C} \to 29.8^\circ\text{C}$
- Full video capture is available in [`media/smoke_detection_hardware_demo.mp4`](media/smoke_detection_hardware_demo.mp4).

---

## 5. Repository Directory Layout

```
IoT-Air-Quality-Monitoring-System/
├── README.md                           # Master system specification
├── platformio.ini                      # PlatformIO embedded build environment
├── .gitignore                          # Build and temporary artifact exclusions
├── firmware/
│   └── air_quality_logger.ino          # Production calibrated ESP32 firmware
├── data/
│   └── sample_aqmi_log.csv             # Verified sample MicroSD log recording
├── tools/
│   └── analyze_aqmi_logs.py            # Statistical metrics validator & log analyzer
├── docs/
│   ├── circuit_schematic.jpg           # High-resolution breadboard wiring schematic
│   ├── IoT_Air_Quality_Monitoring_Index_Presentation.pdf   # 24-slide Analog Circuits Report
│   └── MPMC_Project_Presentation.pptx  # 25-slide Microcontrollers Project Defense
└── media/
    └── smoke_detection_hardware_demo.mp4 # 97-second live smoke stress test recording
```

---

## 6. Build & Deployment Instructions

### PlatformIO
```bash
pio run -t upload
pio device monitor -b 115200
```

### Arduino IDE
1. Install **ESP32 by Espressif Systems** via Boards Manager.
2. Install library dependencies:
   - `Adafruit BMP280 Library`
   - `Adafruit Unified Sensor`
   - `Blynk`
3. Select board **ESP32 Dev Module** and Flash Frequency **80MHz**.
4. Open [`firmware/air_quality_logger.ino`](firmware/air_quality_logger.ino) and upload.

### Offline Log Analysis
```bash
python tools/analyze_aqmi_logs.py data/sample_aqmi_log.csv
```

---

## 7. Authors & License

- **Lead Engineer**: Hari Kruthik SK (Reg. 23BEC0358)  
- **Affiliation**: School of Electronics Engineering (SENSE), Vellore Institute of Technology (VIT), Vellore  
- **License**: Released under the [MIT License](LICENSE).
