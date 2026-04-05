# Application Architecture & UI

This document covers the React application architecture, UI/theme system, component library, and build/deploy process. This is the sub-part documentation - see the primary hardware documents for motor, valve, and weather station details.

---

## Tech Stack

| Layer | Technology | Version |
|-------|-----------|---------|
| Frontend | React | 18.3.1 |
| Language | TypeScript | 5.5.3 |
| Build | Vite | 5.4.2 |
| Mobile | Capacitor | 8.1.0 |
| Styling | Tailwind CSS | 3.4.1 |
| Charts | Recharts | 2.12.0 |
| Icons | Lucide React | 0.330.0 |
| Routing | React Router DOM | 6.22.0 |
| CSS Merge | clsx + tailwind-merge | 2.1.0 / 2.2.1 |
| PWA | vite-plugin-pwa | 0.19.0 |

---

## State Management - Context Provider Chain

```
  PROVIDER HIERARCHY
  ==================

  <AuthProvider>                  # Authentication state
    <ToastProvider>               # Toast notifications
      <LiveDataProvider>          # Real-time IoT data (gsma7670c)
        <DeviceProvider>          # Motors, Valves, Stations, Alerts
          <Router>
            <Layout>              # Sidebar + main content
              <Pages />           # Active page component
            </Layout>
          </Router>
        </DeviceProvider>
      </LiveDataProvider>
    </ToastProvider>
  </AuthProvider>
```

### AuthContext (web-src/contexts/AuthContext.tsx)

| Export | Type | Description |
|--------|------|-------------|
| `user` | `User \| null` | Current authenticated user with token |
| `loading` | `boolean` | Auth state loading |
| `login(email, password)` | `Promise` | Firebase email/password sign-in |
| `register(data)` | `Promise` | Create user + store profile |
| `loginDemo()` | `Promise` | Demo account with seeded data |
| `logout()` | `void` | Clear session |
| `updateProfile(data)` | `Promise` | Update user profile in Firebase |

Demo mode: auto-creates a demo account and seeds motors, valves, stations, and alerts if not already present.

### DeviceContext (web-src/contexts/DeviceContext.tsx)

| Export | Type | Description |
|--------|------|-------------|
| `motors` | `Motor[]` | All user motors |
| `valves` | `Valve[]` | All user valves |
| `stations` | `WeatherStation[]` | All user stations |
| `alerts` | `Alert[]` | All user alerts |
| `weather` | `WeatherData` | Current weather snapshot |
| `sensorLog` | `Record<string, SensorReading[]>` | Per-station sensor history |
| `powerHistory` | `{t, kw}[]` | Power consumption history |
| `loading` | `boolean` | Data loading state |
| `addMotor(data)` | `Promise<{success, error?, id?}>` | Create motor |
| `updateMotor(id, data)` | `Promise` | Update motor fields |
| `deleteMotor(id)` | `Promise` | Delete motor |
| `toggleMotor(id)` | `Promise` | Start/stop motor + log command |
| `addValve(data)` | `Promise` | Create valve |
| `updateValve(id, data)` | `Promise` | Update valve fields |
| `deleteValve(id)` | `Promise` | Delete valve |
| `toggleValve(id)` | `Promise` | Open/close valve + log command |
| `addStation(data)` | `Promise` | Create station |
| `deleteStation(id)` | `Promise` | Delete station |
| `acknowledgeAlert(id)` | `Promise` | Mark alert as acknowledged |
| `refreshAll()` | `Promise` | Reload all data from Firebase |

Polling: 3s for motor/valve state, 15s for sensor data push.

### LiveDataContext (web-src/contexts/LiveDataContext.tsx)

| Export | Type | Description |
|--------|------|-------------|
| `latestPower` | `PowerReading \| null` | Latest 3-phase power reading |
| `latestWeather` | `WeatherReading \| null` | Latest weather reading |
| `isMotorRunning` | `boolean` | Live motor running status |
| `latestFault` | `FaultMessage \| null` | Latest fault message |
| `powerHistory` | `PowerReading[]` | Last 50 power readings |
| `weatherHistory` | `WeatherReading[]` | Last 50 weather readings |
| `faultHistory` | `FaultMessage[]` | Last 20 fault messages |
| `loading` | `boolean` | Data loading state |
| `lastUpdate` | `Date \| null` | Last successful fetch time |
| `error` | `string \| null` | Error message if any |
| `connected` | `boolean` | Connection status |
| `databaseName` | `string` | "gsma7670c" |
| `totalPower` | `number` | Sum of 3-phase power (kW) |
| `avgVoltage` | `number` | Average of 3-phase voltages |
| `avgCurrent` | `number` | Average of 3-phase currents |

Source: gsma7670c Firebase, 5s polling interval.

### ToastContext (web-src/contexts/ToastContext.tsx)

| Export | Type | Description |
|--------|------|-------------|
| `toast(message, type?)` | `void` | Show notification (success/error/warning) |

---

## Navigation & Routing

### Pages (7 total)

| Route | Page Component | Description |
|-------|---------------|-------------|
| `/auth` | AuthPage | Login, register, demo login |
| `/` | DashboardPage | Real-time overview with stats |
| `/motors` | MotorsPage | Motor control & monitoring |
| `/valves` | ValvesPage | Valve control & irrigation |
| `/weather` | WeatherPage | Weather data & sensors |
| `/alerts` | AlertsPage | Alerts & notifications |
| `/settings` | SettingsPage | User settings (6 tabs) |

### Sidebar Navigation

6 navigation items:
1. Dashboard (home icon)
2. Motors (zap icon)
3. Valves (droplet icon)
4. Weather (cloud icon)
5. Alerts (bell icon)
6. Settings (settings icon)

Plus: user profile card (name, role, farm/village) and logout button.

Responsive: full sidebar on desktop, collapsible on mobile.

---

## Component Library

### UI Components (web-src/components/ui/)

| Component | Variants | Props |
|-----------|----------|-------|
| **Card** | Default | children, className |
| **Button** | primary, outline, danger, ghost | size (sm/md/lg), disabled, onClick |
| **Toggle** | Default | checked, onChange, label |
| **Input** | text, email, password, number, date, time | label, value, onChange, placeholder |
| **Modal** | Default | title, subtitle, open, onClose, tabs |
| **Badge** | success, warning, danger, info, default | children, variant |
| **Slider** | Default | min, max, value, onChange, label, unit |
| **Tabs** | Default | tabs array, activeTab, onChange |

### Dialog Components (web-src/components/dialogs/)

| Dialog | Trigger | Tabs |
|--------|---------|------|
| **ConfigureValveDialog** | "Schedule" button on valve card | Configure, View Schedules |
| **ValveSettingsDialog** | "Settings" button on valve card | General, Schedule, Crop Info, Notifications, Appearance |
| **MotorSettingsDialog** | "Settings" button on motor card | General, Protection, Notifications |
| **WeatherStationDialog** | "Weather" button on valve card | Station linking |
| **CropInfoDialog** | "Add/Edit" in ValveSettingsDialog | Crop survey form |
| **SensorTable** | Expand button on weather station | Sensor log table |

### Layout Components (web-src/components/layout/)

| Component | Purpose |
|-----------|---------|
| **Layout** | Main page wrapper with sidebar |
| **Sidebar** | Navigation menu + user profile |

---

## Page Features

### DashboardPage
- Live power monitor card (total kW from 3-phase)
- Phase voltage cards (R/Y/B volts)
- Device statistics: open valves, running motors, active stations, unack'd alerts
- Live sensor data: air temp, humidity, soil moisture (3-stage), leaf wetness
- Motor fault alert banner
- Quick action buttons: Add Valve, Add Motor, Add Station, View Alerts
- Farm greeting with user's farm/village/crop info

### AuthPage
- Tab switcher: Login / Register
- Login: email + password fields
- Register: name, email, password, phone, role selector, farm details (district, mandal, village, farm, crop, acreage)
- Demo login button (auto-creates demo account with seed data)
- Language selector: English, Telugu, Hindi, Tamil, Kannada
- User roles: farmer, technician, admin, seed

### SettingsPage (6 tabs)
1. **Profile**: name, email (read-only), phone, role (read-only)
2. **Farm Survey**: district, mandal, village, farm name, acreage, crop type, previous crop, sowing/harvest dates, soil type, irrigation method
3. **Notifications**: email/SMS/push toggles, critical-only, daily reports, per-type (pest/disease/moisture) toggles
4. **System**: auto irrigation, smart scheduling, weather integration, energy saving, units (metric/imperial), language
5. **Security**: change password, 2FA, login history, privacy, download data, delete account
6. **Appearance**: theme (light/dark/system), dashboard layout (default/compact/detailed), font size (small/medium/large)

### AlertsPage
- Summary cards: high/medium/low/acknowledged counts
- Filter buttons: All, Pests, Diseases, Weather
- Alert cards: name, severity badge, confidence %, area, timestamp (relative), recommendation, acknowledge button
- Mark all as read

---

## Theme & Styling

### Tailwind Configuration

```javascript
// tailwind.config.js
module.exports = {
  darkMode: 'class',
  content: ['./index.html', './src/**/*.{js,ts,jsx,tsx}'],
  theme: {
    extend: {
      colors: {
        brand: {
          50:  '#F0FDF4',
          100: '#DCFCE7',
          200: '#BBF7D0',
          300: '#86EFAC',
          400: '#4ADE80',
          500: '#22C55E',   // Primary green
          600: '#16A34A',
          700: '#166534',   // Dark green
          800: '#14532D',
          900: '#052E16',   // Darkest green
        }
      },
      fontFamily: {
        sans: ['Inter', 'system-ui', 'sans-serif'],
      }
    }
  }
};
```

### Color Palette

| Usage | Color | Hex |
|-------|-------|-----|
| Primary | Green | #22C55E |
| Dark | Dark Green | #166534 |
| Darkest | Forest Green | #052E16 |
| Splash Screen | Green background | #166534 |

### Font
- Primary: **Inter** (Google Fonts)
- Fallback: system-ui, sans-serif

---

## Build & Deploy Guide

### Step-by-Step App Recreation

#### 1. Project Scaffolding

```bash
npm create vite@latest sasyamithra -- --template react-ts
cd sasyamithra
```

#### 2. Install Dependencies

```bash
# Production dependencies
npm install react@^18.3.1 react-dom@^18.3.1 react-router-dom@^6.22.0
npm install firebase@^12.11.0 recharts@^2.12.0 lucide-react@^0.330.0
npm install clsx@^2.1.0 tailwind-merge@^2.2.1
npm install @capacitor/core@^8.1.0 @capacitor/cli@^8.1.0 @capacitor/android@^8.1.0

# Dev dependencies
npm install -D tailwindcss@^3.4.1 autoprefixer@^10.4.18 postcss@^8.4.35
npm install -D vite-plugin-pwa@^0.19.0
npm install -D @vitejs/plugin-react@^4.3.1
npm install -D typescript@^5.5.3
```

#### 3. Firebase Project Setup

1. Create Firebase project: `sasymithrajulyv1`
2. Enable Authentication (Email/Password)
3. Create Realtime Database (asia-southeast1 region)
4. Deploy database rules from `database.rules.json`
5. Create second Firebase project: `gsma7670c` (live IoT data)
6. Set database rules for gsma7670c to allow public read

#### 4. Environment Configuration

Create `.env` from `env-config.txt` with your Firebase credentials.

#### 5. Vite Configuration

Key settings in `vite.config.ts`:
- React plugin
- PWA plugin with auto-update and service worker
- Path alias: `@` -> `./src`
- Runtime caching for `identitytoolkit.googleapis.com` (NetworkFirst)
- Source maps disabled in production

#### 6. Capacitor Android Setup

```bash
npx cap init "Sasyamithra" "com.sasyamithra.pilot" --web-dir dist
npx cap add android
```

`capacitor.config.ts`:
```typescript
const config: CapacitorConfig = {
  appId: 'com.sasyamithra.pilot',
  appName: 'Sasyamithra',
  webDir: 'dist',
  server: { androidScheme: 'https' },
  plugins: {
    SplashScreen: {
      launchShowDuration: 2000,
      backgroundColor: '#166534',
    }
  }
};
```

#### 7. Build Process

```bash
# TypeScript check + Vite build
npm run build

# Sync with Android
npx cap sync android
```

#### 8. Android APK Generation

```bash
cd android

# Debug build
./gradlew assembleDebug

# Release build (requires keystore)
./gradlew assembleRelease
```

Android config (app-build.gradle):
```gradle
namespace = "com.sasyamithra.pilot"
applicationId = "com.sasyamithra.pilot"
versionCode = 1
versionName = "1.0"
```

Gradle plugin: `com.android.tools.build:gradle:8.13.0`
Google Services: `com.google.gms:google-services:4.4.4`

#### 9. APK Signing

Keystore: `sasyamithra-release.jks`

Three APK variants:
- `Sasyamithra-v6.2-demo.apk` - Includes demo data for testing
- `Sasyamithra-v6.2-nodemo.apk` - Production (no demo data)
- `app-release-unsigned.apk` - Unsigned release

#### 10. Firebase Hosting (Web)

```bash
npm install -g firebase-tools
firebase login
firebase deploy --only hosting
```

Config in `firebase.json`:
```json
{
  "hosting": { "public": "dist" },
  "database": { "rules": "database.rules.json" }
}
```

---

## File-by-File Source Map

| File | Lines | Purpose |
|------|-------|---------|
| **Pages** | | |
| pages/AuthPage.tsx | ~180 | Login/register/demo authentication |
| pages/DashboardPage.tsx | ~200 | Real-time dashboard with stats |
| pages/MotorsPage.tsx | ~350 | Motor control and monitoring |
| pages/ValvesPage.tsx | ~400 | Valve control and irrigation |
| pages/WeatherPage.tsx | ~300 | Weather data and sensor display |
| pages/AlertsPage.tsx | ~180 | Alert management |
| pages/SettingsPage.tsx | ~350 | User settings (6 tabs) |
| **Contexts** | | |
| contexts/AuthContext.tsx | ~180 | Auth state, login/register/demo |
| contexts/DeviceContext.tsx | ~500 | Device CRUD, polling, command logging |
| contexts/LiveDataContext.tsx | ~230 | Live IoT data polling (5s) |
| contexts/ToastContext.tsx | ~60 | Toast notification system |
| **Dialogs** | | |
| dialogs/ConfigureValveDialog.tsx | ~400 | 4-mode valve configuration |
| dialogs/ValveSettingsDialog.tsx | ~350 | Valve settings (5 tabs) |
| dialogs/MotorSettingsDialog.tsx | ~250 | Motor settings (3 tabs) |
| dialogs/WeatherStationDialog.tsx | ~150 | Station-valve linking |
| dialogs/CropInfoDialog.tsx | ~300 | Crop survey form |
| dialogs/SensorTable.tsx | ~100 | Sensor readings table |
| **UI** | | |
| ui/Badge.tsx | ~30 | Status badge component |
| ui/Button.tsx | ~40 | Button with variants |
| ui/Card.tsx | ~20 | Container card |
| ui/Input.tsx | ~50 | Form input component |
| ui/Modal.tsx | ~80 | Modal dialog with tabs |
| ui/Slider.tsx | ~40 | Range slider |
| ui/Tabs.tsx | ~30 | Tab navigation |
| ui/Toggle.tsx | ~30 | Toggle switch |
| **Libraries** | | |
| lib/firebase.ts | ~165 | Firebase REST API wrapper |
| lib/demoData.ts | ~200 | Demo data for testing |
| lib/utils.ts | ~100 | Helper functions |
| **Types** | | |
| types/index.ts | ~265 | Runtime type definitions |
| types/database.ts | ~455 | Production database schemas |
| **Root** | | |
| App.tsx | ~80 | Root component with providers |
| main.tsx | ~15 | React DOM entry point |
| index.css | ~20 | Tailwind imports |

**Total: ~6,400 lines of TypeScript/TSX**

---

## Android Configuration

### AndroidManifest.xml

```xml
<manifest package="com.sasyamithra.pilot">
  <uses-permission android:name="android.permission.INTERNET" />
  <application
    android:label="Sasyamithra"
    android:theme="@style/AppTheme"
    android:icon="@mipmap/ic_launcher"
    android:roundIcon="@mipmap/ic_launcher_round">
    <activity
      android:name=".MainActivity"
      android:launchMode="singleTask"
      android:exported="true">
      <intent-filter>
        <action android:name="android.intent.action.MAIN" />
        <category android:name="android.intent.category.LAUNCHER" />
      </intent-filter>
    </activity>
    <provider android:name="androidx.core.content.FileProvider" ... />
  </application>
</manifest>
```

Android dependencies:
- `androidx.appcompat:appcompat`
- `androidx.coordinatorlayout:coordinatorlayout`
- `androidx.core:core-splashscreen`
- Capacitor Android library
