# PlantCare# 🌱 PlantCare — Smart IoT Plant Monitoring & Care System

An intelligent, end-to-end IoT system for monitoring plant health and automating irrigation — built at **Auckland University of Technology (AUT)**. Combines an ESP32 microcontroller, MQTT messaging, InfluxDB time-series storage, and an Express.js REST API to deliver real-time sensor data and remote pump control.

> Developed as a team project by **Gagandeep Singh Ahluwalia**, **Yeoungjun Kim**, and **Ali Sultani**.

---

## 📋 Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Tech Stack](#tech-stack)
- [Features](#features)
- [Hardware](#hardware)
- [Project Structure](#project-structure)
- [Environment Setup](#environment-setup)
- [Getting Started](#getting-started)
- [API Reference](#api-reference)
- [MQTT Topics](#mqtt-topics)
- [Sensor Thresholds & Alerts](#sensor-thresholds--alerts)
- [Team](#team)

---

## Overview

PlantCare is a full IoT pipeline that monitors four environmental parameters — **soil moisture**, **temperature**, **humidity**, and **light** — and allows users to remotely control a water pump via a web dashboard. Data flows from an ESP32 device → MQTT broker (HiveMQ Cloud TLS) → Express.js server → InfluxDB, enabling real-time dashboards and historical trend analysis.

The system also features **AI-powered plant detection** with personalised care recommendations and **customisable user profiles** for managing multiple plants.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        ESP32 Device                          │
│  Soil Moisture │ DHT11 (Temp/Humidity) │ Light │ Water Pump  │
└──────────────────────────┬──────────────────────────────────┘
                           │ MQTT over TLS (port 8883)
                           ▼
             ┌─────────────────────────┐
             │   HiveMQ Cloud Broker   │
             └────────────┬────────────┘
                          │
                          ▼
             ┌─────────────────────────┐
             │  Express.js API Server  │
             │  (Node.js + MQTT client)│
             └────────┬────────┬───────┘
                      │        │
             InfluxDB │        │ REST API
          (telemetry) │        │ (dashboard / controls)
             ┌────────▼─┐  ┌───▼────────────┐
             │ InfluxDB │  │  Web Dashboard  │
             │  Cloud   │  │  (real-time)    │
             └──────────┘  └────────────────┘
```

---

## Tech Stack

### Hardware
![ESP32](https://img.shields.io/badge/ESP32-WROOM--32D-red?style=flat)
![DHT11](https://img.shields.io/badge/Sensor-DHT11-green?style=flat)

### Firmware
![Arduino](https://img.shields.io/badge/Arduino-C++-00979D?style=flat&logo=arduino&logoColor=white)

### Backend
![Node.js](https://img.shields.io/badge/Node.js-339933?style=flat&logo=node.js&logoColor=white)
![Express.js](https://img.shields.io/badge/Express.js-000000?style=flat&logo=express&logoColor=white)
![MQTT](https://img.shields.io/badge/MQTT-5.x-660066?style=flat)

### Database
![InfluxDB](https://img.shields.io/badge/InfluxDB_3-22ADF6?style=flat&logo=influxdb&logoColor=white)

### Cloud
![HiveMQ](https://img.shields.io/badge/HiveMQ-Cloud-f5a623?style=flat)

### Key Libraries
| Library | Purpose |
|---|---|
| `mqtt` v5 | MQTT client for Node.js |
| `@influxdata/influxdb3-client` | InfluxDB 3 write/query |
| `express` | REST API server |
| `helmet` | HTTP security headers |
| `cors` | Cross-origin resource sharing |
| `morgan` | HTTP request logging |
| `dotenv` | Environment variable management |
| `node-cron` | Scheduled tasks |

---

## Features

### Real-Time Monitoring
- 4-sensor telemetry: soil moisture (%), temperature (°C), humidity (%), and light level (%)
- Live dashboard with per-device data
- Automatic alerting when soil moisture drops below 10%
- Online/offline status tracking per device

### Remote Irrigation Control
- Start/stop water pump via REST API from anywhere
- Configurable pump duration (milliseconds)
- Pump status reflected in live telemetry feed

### Data Persistence
- All telemetry written to InfluxDB time-series database
- Device status events stored for audit and diagnostics
- Last 10 readings cached in-memory per device for fast access

### AI-Powered Plant Intelligence
- Plant detection from images with personalised care recommendations
- Optimal watering schedules based on plant species and environmental data

### Multi-Device & Multi-User
- Supports multiple ESP32 devices simultaneously
- Customisable user profiles for managing multiple plants

### Developer-Friendly API
- Health check endpoint with live service status
- RESTful irrigation control endpoints
- Modular service architecture (MQTT, InfluxDB, routes fully separated)

---

## Hardware

| Component | Model | Pin |
|---|---|---|
| Microcontroller | ESP32-WROOM-32D | — |
| Soil moisture sensor | Capacitive analog | GPIO 34 |
| Temperature & humidity | DHT11 | GPIO 32 |
| Light sensor | Analog LDR | GPIO 35 |
| Button | Momentary push | GPIO 33 |
| Water pump / motor | 5V DC via relay | GPIO 2 |

### Wiring Notes
- The DHT11 data line requires a 10kΩ pull-up resistor to 3.3V.
- Drive the pump through a relay or MOSFET — do **not** connect it directly to the GPIO.
- All analog sensors read 0–4095 via the ESP32's 12-bit ADC; raw millivolt values are computed in firmware before sending.

---

## Project Structure

```
PlantCare/
├── EPS32_Code.ino                    # ESP32 firmware (Arduino)
├── README.md
└── express_server/
    ├── server.js                     # App entry point
    ├── package.json
    ├── package-lock.json
    ├── routes/
    │   ├── irrigation.js             # Irrigation & device control endpoints
    │   ├── mqtt.js                   # MQTT management endpoints
    │   └── example.js               # Example route scaffold
    └── services/
        ├── mqtt.js                   # MQTTService class (connect, subscribe, publish)
        └── influxdbService.js        # InfluxDBService class (write telemetry/status)
```

---

## Environment Setup

Create a `.env` file inside `express_server/`:

```env
# Server
PORT=3002
NODE_ENV=development

# MQTT (HiveMQ Cloud)
MQTT_HOST=your-cluster.hivemq.cloud
MQTT_PORT=8883
MQTT_PROTOCOL=mqtts
MQTT_USERNAME=your_username
MQTT_PASSWORD=your_password
MQTT_DEFAULT_TOPICS=irrig/#,claim/#

# InfluxDB
INFLUXDB_HOST=https://us-east-1-1.aws.cloud2.influxdata.com
INFLUXDB_TOKEN=your_influxdb_token
INFLUXDB_BUCKET=plantcare-telemetry
```

> **Never commit `.env` to version control.** Add it to `.gitignore`.

---

## Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/YOUR_USERNAME/PlantCare.git
cd PlantCare/express_server
```

### 2. Install dependencies

```bash
npm install
```

### 3. Configure environment

Copy the template above into a `.env` file and fill in your credentials.

### 4. Start the server

```bash
# Development (auto-restart on file changes)
npm run dev

# Production
npm start
```

### 5. Flash the ESP32

Open `EPS32_Code.ino` in the Arduino IDE and:
- Install required libraries: `PubSubClient`, `DHT sensor library`, `WiFiClientSecure`
- Update WiFi credentials in the sketch
- Flash to your ESP32-WROOM-32D

### 6. Verify the setup

```bash
curl http://localhost:3002/health
```

Expected response:
```json
{
  "status": "OK",
  "services": {
    "influxdb": { "connected": true },
    "mqtt": { "connected": true }
  }
}
```

---

## API Reference

Base URL: `http://localhost:3002`

### Health & Info

| Endpoint | Method | Description |
|---|---|---|
| `/health` | GET | Server and service health status |
| `/` | GET | API overview and available endpoints |

### Irrigation

| Endpoint | Method | Description |
|---|---|---|
| `/api/irrigation/devices` | GET | List all registered devices |
| `/api/irrigation/devices/:deviceId` | GET | Get a specific device's details |
| `/api/irrigation/telemetry` | GET | Latest telemetry from all devices |
| `/api/irrigation/telemetry/:deviceId` | GET | Latest telemetry for one device |
| `/api/irrigation/history/:deviceId` | GET | Last 10 telemetry readings for one device |
| `/api/irrigation/pump/:deviceId` | POST | Control the water pump |
| `/api/irrigation/dashboard` | GET | Summary overview with alerts |

#### Pump Control Example

```bash
# Start pump for 5 seconds
curl -X POST http://localhost:3002/api/irrigation/pump/ABCD-1234 \
  -H "Content-Type: application/json" \
  -d '{ "action": "start", "duration": 5000 }'

# Stop pump immediately
curl -X POST http://localhost:3002/api/irrigation/pump/ABCD-1234 \
  -H "Content-Type: application/json" \
  -d '{ "action": "stop" }'
```

### MQTT Management

| Endpoint | Method | Description |
|---|---|---|
| `/api/mqtt` | GET/POST | MQTT status and manual publish |

---

## MQTT Topics

All topics follow the pattern `irrig/<deviceId>/...`

| Topic | Direction | Payload | Description |
|---|---|---|---|
| `irrig/<id>/telemetry` | ESP32 → Server | JSON | Sensor readings |
| `irrig/<id>/status` | ESP32 → Server | `"online"` / `"offline"` | Device availability |
| `irrig/<id>/cmd` | Server → ESP32 | `{ "water_ms": 5000 }` | Pump command |
| `claim/hello/<id>` | ESP32 → Server | `{ "claimCode": "...", "fw": "2.1.0" }` | Device pairing |

### Telemetry Payload Example

```json
{
  "soil_mv": 1823,
  "soil_pct": 62,
  "light_mv": 2100,
  "light_pct": 51,
  "temp_c": 22.4,
  "humidity_pct": 58,
  "pump_running": false
}
```

---

## Sensor Thresholds & Alerts

The dashboard endpoint (`/api/irrigation/dashboard`) automatically generates alerts:

| Condition | Alert Type | Message |
|---|---|---|
| `soil_pct < 10` | `warning` | Low soil moisture: X% |
| `pump_running == true` | `info` | Pump is currently running |

---

## Team

This project was built collaboratively at **Auckland University of Technology (AUT)**:

| Name | Role |
|---|---|
| **Gagandeep Singh Ahluwalia** | IoT integration, backend, MQTT |
| **Yeoungjun Kim** | Hardware, firmware, sensor calibration |
| **Ali Sultani** | Dashboard, data visualisation |

> *"Late-night debugging sessions, breakthrough moments, and constant support — we didn't just build a project together; we built lasting friendships."*

---

## Tags

`#IoT` `#SmartAgriculture` `#ESP32` `#MachineLearning` `#MQTT` `#InfluxDB` `#NodeJS` `#AUT`
