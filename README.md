# Ground Water Quality Monitoring System

An ESP32-based IoT system for real-time groundwater quality monitoring using water quality sensors, local LCD display, buzzer alerts, and remote data updates.

## Problem

Water contamination is often invisible and is usually detected late through manual testing. This project solves that problem by continuously monitoring important water quality parameters and alerting when values become unsafe.

## Solution

This system uses an ESP32 microcontroller to collect sensor data, process readings, display live values locally, and send updates to a remote dashboard or alert system.

## Key Features

- Real-time TDS monitoring
- Turbidity monitoring
- Water temperature monitoring using DS18B20
- Water level monitoring using ultrasonic sensor
- LCD display for live readings
- Buzzer alert for unsafe readings
- Wi-Fi based remote data update
- Modular documentation for pin mapping, calibration, and data flow

## Hardware Used

| Component | Purpose |
|---|---|
| ESP32 Dev Board | Main controller |
| TDS Sensor | Measures dissolved solids in water |
| Turbidity Sensor | Detects water cloudiness |
| DS18B20 Sensor | Measures water temperature |
| Ultrasonic Sensor | Measures water level |
| 20x4 I2C LCD | Displays live readings |
| Buzzer | Gives alert when water is unsafe |

## System Architecture

```txt
Water Quality Sensors
(TDS, Turbidity, Temperature, Water Level)
        ↓
      ESP32
        ↓
 ┌──────┼────────────┐
 LCD   Buzzer   Wi-Fi / Cloud
        ↓
 Dashboard / Telegram Alert
