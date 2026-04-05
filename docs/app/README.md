# Sasyamithra v6.2 - Smart Farm IoT Platform

## App Identity

| Parameter | Value |
|-----------|-------|
| **App Name** | Sasyamithra |
| **Package ID** | `com.sasyamithra.pilot` |
| **Version** | v6.2 (NPM: 5.0.0) |
| **Description** | Smart Farm IoT Platform - Pilot Version |
| **Platform** | Android (APK) + Web (PWA) |

---

## System Architecture Overview

```
                            SASYAMITHRA IoT SYSTEM
 ============================================================================

  FIELD HARDWARE                    CLOUD                     MOBILE APP
 +-----------------+        +------------------+       +------------------+
 | 3-Phase Motors  |        | Firebase Auth    |       | React 18 + TS    |
 | (MOT-xxx-xx)    |--+     | (identitytoolkit)|       | Capacitor 8.1    |
 +-----------------+  |     +------------------+       | Tailwind CSS     |
                      |                                +--------+---------+
 +-----------------+  |     +------------------+                |
 | Weather         |  +---->| Firebase RTDB    |<-------REST--->+
 | Stations        |  |     | (sasymithrajulyv1)|     API (HTTP)
 | (WS-xxx-xx)     |--+     | User/Device Data |
 +-----------------+  |     +------------------+
                      |
 +-----------------+  |     +------------------+
 | Irrigation      |  +---->| Firebase RTDB    |<-------REST--->+
 | Valves          |        | (gsma7670c)      |      (5s poll)
 | (VLV-xxx-xx)    |        | Live IoT Stream  |
 +-----------------+        +------------------+

  Protocols:                 Region:
  MQTT / HTTP /              asia-southeast1
  LoRa / GSM
```

---

## Tech Stack

| Layer | Technology | Version |
|-------|-----------|---------|
| **Frontend Framework** | React | ^18.3.1 |
| **Language** | TypeScript | ^5.5.3 |
| **Build Tool** | Vite | ^5.4.2 |
| **Mobile Wrapper** | Capacitor | ^8.1.0 |
| **Styling** | Tailwind CSS | ^3.4.1 |
| **Backend / Database** | Firebase (Realtime DB + Auth) | ^12.11.0 |
| **Charts** | Recharts | ^2.12.0 |
| **Icons** | Lucide React | ^0.330.0 |
| **CSS Utilities** | clsx + tailwind-merge | ^2.1.0 / ^2.2.1 |
| **Routing** | React Router DOM | ^6.22.0 |
| **PWA** | vite-plugin-pwa | ^0.19.0 |
| **Android Build** | Gradle | 8.13.0 |
| **Google Services** | google-services plugin | 4.4.4 |

### Dev Dependencies

| Package | Version |
|---------|---------|
| @types/react | ^18.3.5 |
| @types/react-dom | ^18.3.0 |
| @vitejs/plugin-react | ^4.3.1 |
| autoprefixer | ^10.4.18 |
| eslint | ^9.9.1 |
| postcss | ^8.4.35 |

---

## Environment Variables

Create a `.env` file at project root:

```env
# Firebase Configuration - User Data & Auth (sasymithrajulyv1)
VITE_FIREBASE_API_KEY=<your-api-key>
VITE_FIREBASE_AUTH_DOMAIN=sasymithrajulyv1.firebaseapp.com
VITE_FIREBASE_DATABASE_URL=https://sasymithrajulyv1-default-rtdb.asia-southeast1.firebasedatabase.app
VITE_FIREBASE_PROJECT_ID=sasymithrajulyv1
VITE_FIREBASE_STORAGE_BUCKET=sasymithrajulyv1.appspot.com
VITE_FIREBASE_MESSAGING_SENDER_ID=
VITE_FIREBASE_APP_ID=

# Live IoT Data (gsma7670c - separate database)
VITE_LIVE_IOT_DATABASE_URL=https://gsma7670c-default-rtdb.asia-southeast1.firebasedatabase.app

# App Configuration
VITE_APP_NAME=Sasyamithra
VITE_SENSOR_POLL_INTERVAL=15000
```

---

## Build Scripts

```bash
npm run dev        # Start Vite dev server
npm run build      # TypeScript check + Vite production build
npm run preview    # Preview production build locally
npm run lint       # ESLint with TypeScript rules
```

---

## Project Directory Structure

```
sasyamithra-v6.2/
+-- package.json                       # Dependencies & scripts
+-- tsconfig.json                      # TypeScript config (ES2020, strict, react-jsx)
+-- vite.config.ts                     # Vite + PWA + alias (@/ -> ./src)
+-- tailwind.config.js                 # Brand colors, Inter font, dark mode
+-- capacitor.config.ts                # Android wrapper config
+-- firebase.json                      # Firebase hosting & rules
+-- database.rules.json                # Security rules (user-scoped)
+-- env-config.txt                     # Environment variable reference
+-- sasyamithra-release.jks            # Android signing keystore
+-- Sasyamithra-v6.2-demo.apk         # Demo build (with sample data)
+-- Sasyamithra-v6.2-nodemo.apk       # Production build
+-- app-release-unsigned.apk           # Unsigned release
|
+-- web-src/                           # Source code (6,402 lines)
|   +-- main.tsx                       # React DOM root entry
|   +-- App.tsx                        # Root component + context providers + routing
|   +-- index.css                      # Global Tailwind imports
|   |
|   +-- pages/                         # 7 page components
|   |   +-- AuthPage.tsx               # Login / Register / Demo login
|   |   +-- DashboardPage.tsx          # Real-time overview dashboard
|   |   +-- MotorsPage.tsx             # Motor control & monitoring
|   |   +-- ValvesPage.tsx             # Valve control & irrigation
|   |   +-- WeatherPage.tsx            # Weather data & sensor display
|   |   +-- AlertsPage.tsx             # Alerts & notifications
|   |   +-- SettingsPage.tsx           # User settings (6 tabs)
|   |   +-- LiveDashboardPage.tsx      # Real-time data view
|   |   +-- index.ts                   # Barrel export
|   |
|   +-- contexts/                      # React Context state management
|   |   +-- AuthContext.tsx             # Authentication (login, register, demo)
|   |   +-- DeviceContext.tsx           # Motors, Valves, Stations, Alerts CRUD
|   |   +-- LiveDataContext.tsx         # Real-time IoT data (gsma7670c)
|   |   +-- ToastContext.tsx            # Toast notifications
|   |   +-- index.ts
|   |
|   +-- components/
|   |   +-- dialogs/                   # Modal dialog components
|   |   |   +-- ConfigureValveDialog.tsx    # 4-mode valve configuration
|   |   |   +-- ValveSettingsDialog.tsx     # Valve settings (5 tabs)
|   |   |   +-- MotorSettingsDialog.tsx     # Motor settings (3 tabs)
|   |   |   +-- WeatherStationDialog.tsx    # Station-valve linking
|   |   |   +-- CropInfoDialog.tsx          # Crop survey form
|   |   |   +-- SensorTable.tsx             # Sensor readings table
|   |   |   +-- index.ts
|   |   |
|   |   +-- layout/                    # Layout components
|   |   |   +-- Layout.tsx             # Main app wrapper
|   |   |   +-- Sidebar.tsx            # Navigation sidebar
|   |   |   +-- index.ts
|   |   |
|   |   +-- ui/                        # Reusable UI components
|   |       +-- Badge.tsx, Button.tsx, Card.tsx, Input.tsx
|   |       +-- Modal.tsx, Slider.tsx, Tabs.tsx, Toggle.tsx
|   |       +-- index.ts
|   |
|   +-- lib/                           # Utility libraries
|   |   +-- firebase.ts                # Firebase REST API wrapper
|   |   +-- demoData.ts                # Demo data for testing
|   |   +-- utils.ts                   # Helper functions
|   |
|   +-- types/                         # TypeScript type definitions
|       +-- index.ts                   # Runtime app types
|       +-- database.ts                # Production database schemas
|
+-- web-dist/                          # Production build output
+-- android-build-config/              # Android Gradle config
    +-- AndroidManifest.xml
    +-- app-build.gradle
    +-- build.gradle
```

---

## Documentation Index

| Document | Description |
|----------|-------------|
| [THREE_PHASE_MOTOR.md](THREE_PHASE_MOTOR.md) | 3-Phase motor hardware, protection, R/Y/B phase monitoring, uplink/downlink |
| [WEATHER_STATION.md](WEATHER_STATION.md) | Weather station sensors, 3-stage soil moisture, pest/disease algorithms |
| [VALVE_CONTROL.md](VALVE_CONTROL.md) | Valve hardware, 4 control modes, crop survey integration |
| [DATA_UPLINK_DOWNLINK.md](DATA_UPLINK_DOWNLINK.md) | Communication protocols, Firebase databases, REST API, polling architecture |
| [DATA_MODELS.md](DATA_MODELS.md) | Complete TypeScript interfaces, schemas, entity relationships |
| [APP_ARCHITECTURE.md](APP_ARCHITECTURE.md) | React architecture, UI/theme, component library, build & deploy guide |

---

## Device ID Naming Conventions

| Device Type | Format | Example |
|-------------|--------|---------|
| Motor | `MOT-{NNN}-{ZONE}` | MOT-001-A1 |
| Valve | `VLV-{NNN}-{ZONE}` | VLV-001-A1 |
| Weather Station | `WS-{NNN}-{LOCATION}` | WS-001-MAIN |

---

## Quick Start (Development)

```bash
# 1. Clone and install
git clone <repo-url>
cd sasyamithra-v6.2
npm install

# 2. Configure environment
cp env-config.txt .env

# 3. Start dev server
npm run dev

# 4. Build for production
npm run build

# 5. Build Android APK (requires Capacitor setup)
npx cap sync android
cd android && ./gradlew assembleRelease
```
