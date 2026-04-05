# Data Communication Architecture - Uplink & Downlink

## System Data Flow

```
  DATA FLOW ARCHITECTURE
  ======================

  UPLINK (Device -> Cloud -> App)
  ================================

  Field Devices              Firebase Cloud                  Mobile App
  +----------------+        +-------------------+           +------------+
  | 3-Phase Motor  |--+     |                   |           |            |
  | Sensors        |  |     | gsma7670c RTDB    |  5s poll  | LiveData   |
  | (power, faults)|  +---->| /Motor-Values     |---------->| Context    |
  |                |  |     | /weatherData      |           |            |
  +----------------+  |     | /message          |           |            |
                      |     | /isRunning        |           |            |
  +----------------+  |     +-------------------+           |            |
  | Weather Station|  |                                     |            |
  | Sensors        |--+     +-------------------+           |            |
  | (9+ sensors)   |  |     |                   |  3s poll  | Device     |
  +----------------+  |     | sasymithrajulyv1  |---------->| Context    |
                      +---->| /users/{uid}/     |           |            |
  +----------------+  |     |   motors/         |           |            |
  | Valve Status   |--+     |   valves/         |           |            |
  |                |        |   sensorLog/      |           |            |
  +----------------+        +-------------------+           +------------+

  DOWNLINK (App -> Cloud -> Device)
  ==================================

  Mobile App               Firebase Cloud                  Field Devices
  +------------+           +-------------------+           +------------+
  | Toggle      |           |                   |           |            |
  | Motor/Valve |---------->| sasymithrajulyv1  |---------->| Motor      |
  |             |  fbPatch  | /users/{uid}/     |   MQTT/   | Valve      |
  | Configure   |---------->|   motors/{id}     |   HTTP/   | Station    |
  | Thresholds  |  fbPatch  |   valves/{id}     |   LoRa/   |            |
  |             |           |   cmdLog/         |   GSM     |            |
  | Add/Delete  |---------->|                   |           |            |
  | Devices     |  fbPush   +-------------------+           +------------+
  +------------+
```

---

## Connectivity Protocols

| Protocol | Use Case | Characteristics |
|----------|----------|----------------|
| **MQTT** | Primary IoT | Broker-based pub/sub, low latency, topic paths |
| **HTTP** | REST API | Request-response, used by app-to-cloud |
| **LoRa** | Long-range | Low-power, field-deployed sensors, km range |
| **GSM** | Cellular | SIM-equipped devices, wide coverage |

```typescript
type ConnectivityProtocol = 'mqtt' | 'http' | 'lora' | 'gsm';

interface DeviceConnectivity {
  protocol: ConnectivityProtocol;
  broker?: string;           // MQTT broker URL
  topic?: string;            // MQTT topic path
  lastSeen: string;          // ISO timestamp
  online: boolean;           // Current connectivity status
  signalStrength: number;    // RSSI / signal quality (0-100)
}
```

---

## Two Firebase Database Instances

### 1. User Data Database: `sasymithrajulyv1`

| Parameter | Value |
|-----------|-------|
| **Project ID** | sasymithrajulyv1 |
| **Region** | asia-southeast1 |
| **URL** | `https://sasymithrajulyv1-default-rtdb.asia-southeast1.firebasedatabase.app` |
| **Auth Domain** | sasymithrajulyv1.firebaseapp.com |
| **Purpose** | User profiles, device configurations, sensor logs, command logs, alerts |

### 2. Live IoT Database: `gsma7670c`

| Parameter | Value |
|-----------|-------|
| **Database** | gsma7670c |
| **Region** | asia-southeast1 |
| **URL** | `https://gsma7670c-default-rtdb.asia-southeast1.firebasedatabase.app` |
| **Purpose** | Real-time motor power readings, live weather data, fault messages |
| **Access** | Public read (no auth token required for GET) |

---

## Firebase REST API Layer

The app uses a custom REST API wrapper instead of the Firebase SDK, for a lighter APK bundle.

Source file: `web-src/lib/firebase.ts`

### Configuration

```typescript
const FB = {
  apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
  authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
  databaseURL: import.meta.env.VITE_FIREBASE_DATABASE_URL,
  projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
};
```

### API Functions

| Function | HTTP Method | Purpose | Returns |
|----------|-------------|---------|---------|
| `fbAuth(email, password, isNew)` | POST | Sign up or sign in | `AuthResponse` (idToken, localId) |
| `fbGet<T>(path, token)` | GET | Read data at path | `T \| null` |
| `fbSet<T>(path, data, token)` | PUT | Overwrite data at path | `void` |
| `fbPatch<T>(path, data, token)` | PATCH | Merge/update data at path | `void` |
| `fbPush<T>(path, data, token)` | POST | Add with auto-generated ID | `{ name: string } \| null` |
| `fbDel(path, token)` | DELETE | Remove data at path | `void` |

### URL Pattern

```
{databaseURL}/{path}.json?auth={idToken}
```

Example: `https://sasymithrajulyv1-default-rtdb.asia-southeast1.firebasedatabase.app/users/abc123/motors.json?auth=eyJ...`

### Authentication Endpoints

| Action | Endpoint |
|--------|----------|
| **Sign Up** | `https://identitytoolkit.googleapis.com/v1/accounts:signUp?key={API_KEY}` |
| **Sign In** | `https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key={API_KEY}` |

Request body:
```json
{
  "email": "user@example.com",
  "password": "password123",
  "returnSecureToken": true
}
```

Response:
```typescript
interface AuthResponse {
  localId: string;       // Firebase UID
  email: string;
  idToken: string;       // JWT token for API calls
  refreshToken: string;  // For token refresh
  expiresIn: string;     // Token TTL in seconds
}
```

Error handling:
- `EMAIL_EXISTS` -> "This email is already registered."
- `INVALID_LOGIN_CREDENTIALS` -> "Invalid email or password."
- `WEAK_PASSWORD` -> "Password must be at least 6 characters."

---

## Complete Database Path Structure

### User Data Database (sasymithrajulyv1)

```
users/
  {uid}/
    profile/
      name: string              (required)
      role: string              (required: farmer|technician|admin|seed)
      phone?: string
      district?: string
      mandal?: string
      village?: string
      farm?: string
      crop?: string
      acreage?: string
      soilType?: string
      irrigationType?: string
      previousCrop?: string
      sowingDate?: string
      expectedHarvest?: string
      createdAt: string

    motors/
      {motorId}/                (auto-generated push ID)
        name: string            (required)
        deviceId: string        (required, e.g., "MOT-001-A1")
        run: boolean
        hp: string              (good|warning|critical)
        pwr: number             (kW)
        hrs: number             (runtime hours)
        kwh: number             (energy consumed)
        startAt: string
        stopAt: string
        phV: { r, y, b }       (phase voltages)
        phI: { r, y, b }       (phase currents)
        pf: { o, r, y, b }     (power factors)
        starDelta: number
        forceRun: boolean
        mode: string
        prot: { dryRun, overload, overV, underV, restart }
        notif: { on, off, fault }
        alerts: [{ msg, sev }]
        createdAt: string

    valves/
      {valveId}/                (auto-generated push ID)
        name: string            (required)
        deviceId: string        (required, e.g., "VLV-001-A1")
        isOpen: boolean
        flow: number
        togAt: string
        mid: string             (motor ID)
        zone: string
        stationId: string
        primaryStationId?: string
        secondaryStationId?: string
        cropType: string
        color: string
        controlMode: string
        forceRun: boolean
        safeMode: boolean
        auto: { on, L1, L2, L3, env }
        qty: { liters, cyclic }
        sched: { recur, start, end, dur }
        valveAlerts: { pest, disease, lowM, excess }
        cropSurvey?: { ... }
        createdAt: string

    stations/
      {stationId}/              (auto-generated push ID)
        name: string            (required)
        deviceId: string        (required, e.g., "WS-001-MAIN")
        location: string
        online: boolean
        lastSeen: string
        sensors: number
        cropGroup?: string
        associatedValves?: string[]
        associatedMotor?: string
        createdAt: string

    alerts/
      {alertId}/                (auto-generated push ID)
        name: string            (required)
        type: string            (required: pest|disease|weather)
        sev: string             (high|medium|low)
        conf: number            (confidence 0-100)
        at: string              (timestamp)
        area: string            (zone/location)
        action: string          (recommendation)
        ack: boolean            (acknowledged)

    sensorLog/
      {stationId}/
        {readingId}/            (auto-generated push ID)
          ts: string
          airT: number
          soilT: number
          humid: number
          soilM: number
          light: number
          leafW: number
          wind: number
          rain: number
          batt: number
          soilM1?: number
          soilM2?: number
          soilM3?: number
          leafW2?: number

    cmdLog/
      {commandId}/              (auto-generated push ID)
        type: string            (motor|valve)
        deviceId: string
        action: string          (start|stop|open|close)
        ts: string
```

### Live IoT Database (gsma7670c)

```
/
  isRunning: boolean                    (motor running status)

  Motor-Values/
    {pushId}/
      Phase1Voltage: number
      Phase1Current: number
      Phase1Power: number
      Phase1PowerFactor: number
      Phase2Voltage: number
      Phase2Current: number
      Phase2Power: number
      Phase2PowerFactor: number
      Phase3Voltage: number
      Phase3Current: number
      Phase3Power: number
      Phase3PowerFactor: number

  weatherData/
    {pushId}/
      airTemperature: number
      humidity: number
      leafWetness: number
      leafWetness2: number
      lightIntensity: number
      soilMoisture: number
      soilMoisture2: number
      soilMoisture3: number
      soilTemperature: number

  message/
    {pushId}/
      message: string                  (fault/error text)
```

---

## Polling & Sync Intervals

| Data Type | Interval | Source Database | Method |
|-----------|----------|----------------|--------|
| Motor/Valve state | 3 seconds | sasymithrajulyv1 | fbGet() polling |
| Live power/weather/faults | 5 seconds | gsma7670c | fetch() polling |
| Sensor readings push | 15 seconds | sasymithrajulyv1 | fbPush() write |

### Motor/Valve State Sync (3s)

```
DeviceContext polling loop:
  Every 3 seconds:
    1. fbGet("users/{uid}/motors") -> motor array
    2. fbGet("users/{uid}/valves") -> valve array
    3. Compare with previous state (JSON.stringify)
    4. Only update React state if data changed
```

### Live IoT Data Sync (5s)

```
LiveDataContext polling loop:
  Every 5 seconds:
    1. fetch("{gsma7670c_URL}/.json") -> full database snapshot
    2. Parse /Motor-Values -> latest PowerReading + last 50 history
    3. Parse /weatherData -> latest WeatherReading + last 50 history
    4. Parse /message -> latest FaultMessage + last 20 history
    5. Parse /isRunning -> motor running boolean
    6. Compute totalPower, avgVoltage, avgCurrent
```

### Sensor Data Push (15s)

```
DeviceContext sensor simulation:
  Every 15 seconds:
    For each weather station:
      1. Generate sensor reading (mkWeather())
      2. fbPush("users/{uid}/sensorLog/{stationId}", reading)
      3. Update local sensorLog state (keep last 100 per station)
```

---

## Command Lifecycle

### Simple Command (Runtime)

```typescript
interface CommandLog {
  type: 'valve' | 'motor';
  deviceId: string;
  action: string;      // 'start' | 'stop' | 'open' | 'close'
  ts: string;          // ISO timestamp
}
```

### Full Command Schema (Production)

```typescript
type CommandAction = 'start' | 'stop' | 'open' | 'close' | 'set' | 'calibrate' | 'reboot';
type CommandStatus = 'pending' | 'sent' | 'acknowledged' | 'executed' | 'failed' | 'timeout';
type CommandSource = 'user' | 'schedule' | 'automation' | 'ai';

interface Command {
  commandId: string;
  ts: number;
  deviceId: string;
  farmId: string;

  action: CommandAction;
  params?: Record<string, unknown>;

  source: {
    type: CommandSource;
    userId?: string;
    ruleId?: string;
  };

  status: CommandStatus;

  response?: {
    receivedAt: number;
    executedAt: number;
    result: 'success' | 'error';
    message?: string;
  };
}
```

### Command Status Flow

```
  COMMAND LIFECYCLE
  =================

  [pending] --send--> [sent] --ack--> [acknowledged] --exec--> [executed]
                         |                  |
                         +--timeout-------->+--fail--> [failed]
                                                       [timeout]
```

---

## Database Security Rules

File: `database.rules.json`

```json
{
  "rules": {
    "users": {
      "$uid": {
        ".read": "$uid === auth.uid",
        ".write": "$uid === auth.uid",

        "profile": {
          ".validate": "newData.hasChildren(['name', 'role'])"
        },
        "motors": {
          "$motorId": {
            ".validate": "newData.hasChildren(['name', 'deviceId'])"
          }
        },
        "valves": {
          "$valveId": {
            ".validate": "newData.hasChildren(['name', 'deviceId'])"
          }
        },
        "stations": {
          "$stationId": {
            ".validate": "newData.hasChildren(['name', 'deviceId'])"
          }
        },
        "alerts": {
          "$alertId": {
            ".validate": "newData.hasChildren(['name', 'type'])"
          }
        },
        "sensorLog": {
          ".read": true,
          ".write": true
        },
        "cmdLog": {
          ".read": true,
          ".write": true
        }
      }
    }
  }
}
```

Key rules:
- Users can only read/write their own data (`$uid === auth.uid`)
- Profile requires `name` and `role` fields
- Motors, valves, stations require `name` and `deviceId`
- Alerts require `name` and `type`
- sensorLog and cmdLog are publicly readable/writable (within user scope)

---

## Authentication Flow

```
  AUTHENTICATION FLOW
  ====================

  User                  App                    Firebase Auth
  |                     |                      |
  |  email + password   |                      |
  |-------------------->|                      |
  |                     |  POST /signIn        |
  |                     |--------------------->|
  |                     |                      |
  |                     |  { idToken, uid }    |
  |                     |<---------------------|
  |                     |                      |
  |                     |  Store token         |
  |                     |  (in-memory only)    |
  |                     |                      |
  |                     |  GET profile         |
  |                     |  /users/{uid}/profile|
  |                     |--------------------->| Firebase RTDB
  |                     |                      |
  |  Dashboard loaded   |  { name, role, ... } |
  |<--------------------|<---------------------|
```

- Token stored in React state (in-memory, not persisted)
- Token passed as query parameter: `?auth={idToken}`
- No refresh token handling in current version

---

## PWA & Caching Strategy

From `vite.config.ts`:
- Service worker enabled via `vite-plugin-pwa`
- Auto-update strategy
- Firebase auth endpoints cached with `NetworkFirst` strategy:
  - Pattern: `identitytoolkit.googleapis.com`
  - Falls back to cache if offline (allows cached auth)
- Manifest configured with app icons and theme colors
