// =============================================================================
// Project: IoT-Based Air Quality Monitoring Index (AQMI) & Continuous Logger
// Author: Hari Kruthik SK (23BEC0358)
// Target: ESP32-WROOM-32UE
// Protocols: ADC (MQ-2), I2C (BMP280), SPI (MicroSD Card), Wi-Fi (Blynk IoT)
// =============================================================================

#include <Wire.h>               // I2C communication (BMP280)
#include <Adafruit_Sensor.h>    // Sensor unified base library
#include <Adafruit_BMP280.h>    // BMP280 temperature & barometric pressure sensor
#include <SPI.h>                // SPI communication (MicroSD)
#include <SD.h>                 // MicroSD card file system

// -----------------------------------------------------------------------------
// Pin Configuration
// -----------------------------------------------------------------------------
#define MQ2_ANALOG_PIN    34    // ESP32 ADC1_CH6 (GPIO 34)
#define SD_CS_PIN         5     // MicroSD Chip Select (GPIO 5)
#define I2C_SDA_PIN       21    // I2C Serial Data (GPIO 21)
#define I2C_SCL_PIN       22    // I2C Serial Clock (GPIO 22)
#define SPI_SCK_PIN       18    // SPI Clock (GPIO 18)
#define SPI_MISO_PIN      19    // SPI Master In Slave Out (GPIO 19)
#define SPI_MOSI_PIN      23    // SPI Master Out Slave In (GPIO 23)

// Sampling Interval
const unsigned long LOG_INTERVAL_MS = 5000; // 5-second deterministic logging
unsigned long lastLogTime = 0;

Adafruit_BMP280 bmp; // I2C instance

// -----------------------------------------------------------------------------
// Gas Calibration & Concentration Modeling
// -----------------------------------------------------------------------------
// Empirical power-law coefficients derived from sensor resistance ratio Rs/Ro:
// Concentration (ppm) = K * (ADC_raw)^n
void calibrateMQ2(float rawAdc, float &hydrogen, float &methane, float &butane, float &lpg, float &smoke) {
    const float K_Hydrogen = 0.0005;
    const float K_Methane  = 0.0006;
    const float K_Butane   = 0.0008;
    const float K_LPG      = 0.0030;
    const float K_Smoke    = 0.0035;

    const float n_Hydrogen = 0.70;
    const float n_Methane  = 0.70;
    const float n_Butane   = 0.70;
    const float n_LPG      = 0.75;
    const float n_Smoke    = 0.75;

    hydrogen = K_Hydrogen * pow(rawAdc, n_Hydrogen);
    methane  = K_Methane  * pow(rawAdc, n_Methane);
    butane   = K_Butane   * pow(rawAdc, n_Butane);
    lpg      = K_LPG      * pow(rawAdc, n_LPG);
    smoke    = K_Smoke    * pow(rawAdc, n_Smoke);
}

// -----------------------------------------------------------------------------
// SD Card Storage Handler
// -----------------------------------------------------------------------------
void logToSD(const String &entry) {
    File logFile = SD.open("/aqmi_log.csv", FILE_APPEND);
    if (logFile) {
        logFile.println(entry);
        logFile.close();
    } else {
        Serial.println("[ERROR] Failed to open /aqmi_log.csv for appending!");
    }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n========================================================");
    Serial.println("   IoT Air Quality Monitoring Index (AQMI) Starting...  ");
    Serial.println("========================================================");

    // Initialize I2C with defined pins
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Initialize BMP280
    if (!bmp.begin(0x76)) {
        Serial.println("[WARN] BMP280 not found at 0x76, checking 0x77...");
        if (!bmp.begin(0x77)) {
            Serial.println("[FAIL] BMP280 initialization failed! Check wiring.");
        } else {
            Serial.println("[OK] BMP280 initialized at 0x77.");
        }
    } else {
        Serial.println("[OK] BMP280 initialized at 0x76.");
    }

    // Initialize MicroSD module over SPI
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("[WARN] MicroSD card initialization failed or not inserted.");
    } else {
        Serial.println("[OK] MicroSD card initialized successfully.");
        // Check if header is needed
        if (!SD.exists("/aqmi_log.csv")) {
            File initFile = SD.open("/aqmi_log.csv", FILE_WRITE);
            if (initFile) {
                initFile.println("Timestamp_ms,Temp_C,Pressure_hPa,MQ2_Raw,Hydrogen_ppm,Methane_ppm,Butane_ppm,LPG_ppm,Smoke_ppm");
                initFile.close();
                Serial.println("[OK] CSV header created in /aqmi_log.csv.");
            }
        }
    }

    // Configure analog pin
    pinMode(MQ2_ANALOG_PIN, INPUT);
    analogReadResolution(12); // 12-bit ADC (0 - 4095)
    Serial.println("[OK] ADC configured on GPIO 34. Sensor warmup starting...");
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------
void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastLogTime >= LOG_INTERVAL_MS) {
        lastLogTime = currentMillis;

        // 1. Read Sensors
        float rawAdc = analogRead(MQ2_ANALOG_PIN);
        float temperature = bmp.readTemperature();
        float pressure = bmp.readPressure() / 100.0F; // Convert Pa to hPa

        // 2. Calibrate gas components
        float h2, ch4, c4h10, lpg, smoke;
        calibrateMQ2(rawAdc, h2, ch4, c4h10, lpg, smoke);

        // 3. Format CSV entry
        String csvLine = String(currentMillis) + "," +
                         String(temperature, 2) + "," +
                         String(pressure, 2) + "," +
                         String(rawAdc, 0) + "," +
                         String(h2, 2) + "," +
                         String(ch4, 2) + "," +
                         String(c4h10, 2) + "," +
                         String(lpg, 2) + "," +
                         String(smoke, 2);

        // 4. Output to Serial Monitor
        Serial.printf("[AQMI] T=%.2f C | P=%.2f hPa | Smoke=%.2f ppm | LPG=%.2f ppm | ADC=%.0f\n",
                      temperature, pressure, smoke, lpg, rawAdc);

        // 5. Commit to MicroSD storage
        logToSD(csvLine);
    }
}
