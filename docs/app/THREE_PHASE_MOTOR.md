# 3-Phase Motor - Hardware & Control System

## 1. Motor Hardware Specifications

### Device Identity

| Parameter | Value |
|-----------|-------|
| **Device ID Format** | `MOT-{NNN}-{ZONE}` |
| **Example IDs** | MOT-001-A1, MOT-002-B2, MOT-003-C1 |
| **Phase Configuration** | 3-phase AC (R, Y, B) |
| **Supported Phases** | 1 or 3 phase (primary focus: 3-phase) |

### Runtime Data Model

```typescript
interface Motor {
  id: string;                // Firebase push ID (auto-generated)
  deviceId: string;          // Hardware ID, e.g., "MOT-001-A1"
  name: string;              // Display name, e.g., "Primary Borewell Pump"
  run: boolean;              // Current running state
  hp: 'good' | 'warning' | 'critical';  // Health status
  pwr: number;               // Current power consumption (kW)
  hrs: number;               // Total runtime hours
  kwh: number;               // Total energy consumed (kWh)
  startAt: string;           // ISO timestamp of last start
  stopAt: string;            // ISO timestamp of last stop
  phV: PhaseData;            // Phase voltages {r, y, b}
  phI: PhaseData;            // Phase currents {r, y, b}
  pf: PowerFactor;           // Power factors {o, r, y, b}
  starDelta: number;         // Star-delta soft-start delay (seconds, default: 5)
  forceRun: boolean;         // Override safety checks
  mode: 'auto' | 'manual';  // Operating mode
  prot: MotorProtection;     // Protection thresholds
  notif: MotorNotifications; // Notification preferences
  alerts: MotorAlert[];      // Active alerts list
  waterPumped?: number;      // Optional water volume tracking
  createdAt: string;         // ISO timestamp
}
```

---

## 2. Phase Monitoring System

### Phase Data Structures

```typescript
interface PhaseData {
  r: number;    // Red phase
  y: number;    // Yellow phase
  b: number;    // Blue phase
}

interface PowerFactor {
  o: number;    // Overall power factor
  r: number;    // Red phase PF
  y: number;    // Yellow phase PF
  b: number;    // Blue phase PF
}
```

### Monitoring Parameters

| Parameter | Phase R | Phase Y | Phase B | Overall | Unit |
|-----------|---------|---------|---------|---------|------|
| Voltage | 180-260 | 180-260 | 180-260 | - | V |
| Current | 0-50 | 0-50 | 0-50 | - | A |
| Power | - | - | - | Sum(3 phases) | kW |
| Power Factor | 0.0-1.0 | 0.0-1.0 | 0.0-1.0 | 0.0-1.0 | - |

### Live Computed Values (from LiveDataContext)

```
totalPower  = Phase1Power + Phase2Power + Phase3Power
avgVoltage  = (Phase1Voltage + Phase2Voltage + Phase3Voltage) / 3
avgCurrent  = (Phase1Current + Phase2Current + Phase3Current) / 3
```

### Typical Operating Values (Demo Reference)

```
MOT-001-A1: Primary Borewell Pump
  Power:    2.8 kW
  Voltages: R=231V, Y=229V, B=230V
  Currents: R=4.2A, Y=4.1A, B=4.3A
  PF:       0.92 overall
  Runtime:  1245.5 hours
  Energy:   3420.8 kWh
```

---

## 3. Motor Protection System

### Protection Parameters

```typescript
interface MotorProtection {
  dryRun: number;     // Dry-run cutoff timeout (seconds)
  overload: number;   // Overload current threshold (amps)
  overV: number;      // Over-voltage threshold (volts)
  underV: number;     // Under-voltage threshold (volts)
  restart: number;    // Auto-restart delay (seconds)
}
```

| Protection | Field | Default | Range | Description |
|-----------|-------|---------|-------|-------------|
| Dry Run | dryRun | 30 s | 5-120 s | Shuts off motor if no water flow detected within timeout |
| Overload | overload | 15 A | 1-50 A | Trips when any phase current exceeds threshold |
| Over Voltage | overV | 260 V | 230-300 V | Trips when any phase voltage exceeds limit |
| Under Voltage | underV | 180 V | 150-220 V | Trips when any phase voltage drops below limit |
| Auto Restart | restart | 60 s | 10-300 s | Delay before automatic restart after fault clearance |

### Star-Delta Soft Start

- Configurable delay in seconds (default: 5)
- Reduces starting current during motor startup
- Motor starts in star configuration, then switches to delta after delay

### Force Run Override

When `forceRun: true`, the motor bypasses safety protection checks. Use with caution.

---

## 4. Motor Health & Fault System

### Health States

| State | Description | Visual Indicator |
|-------|-------------|-----------------|
| `good` | All parameters within normal range | Green badge |
| `warning` | One or more parameters approaching threshold | Amber badge |
| `critical` | Protection threshold exceeded | Red badge |

### Motor Alerts

```typescript
interface MotorAlert {
  msg: string;                    // Alert message text
  sev: 'warning' | 'critical';   // Severity level
}
```

Examples:
- `{ msg: "Maintenance due in 15 days", sev: "warning" }`
- `{ msg: "Overcurrent detected on Phase R", sev: "critical" }`

### Fault Types

| Fault | Trigger Condition | Source |
|-------|------------------|--------|
| Dry Run | No water flow detected within `dryRun` timeout | Device sensor |
| Overcurrent | Phase current > `overload` threshold | Phase monitoring |
| Overvoltage | Phase voltage > `overV` threshold | Phase monitoring |
| Undervoltage | Phase voltage < `underV` threshold | Phase monitoring |
| Phase Imbalance | Significant variance between R/Y/B readings | Live IoT data |
| Motor Fault | General hardware fault | Fault message pipeline |

### Fault Message Pipeline

```
  Device Sensor --> gsma7670c /message/{pushId} --> App (5s poll) --> Fault Banner
```

Fault messages stored at `/message/{pushId}/message` in the live IoT database.

---

## 5. Motor Data Uplink (Device to Cloud)

### Live Power Readings (gsma7670c Database)

| Parameter | Value |
|-----------|-------|
| **Database** | gsma7670c |
| **URL** | `https://gsma7670c-default-rtdb.asia-southeast1.firebasedatabase.app` |
| **Path** | `/Motor-Values/{pushId}` |
| **Polling Interval** | 5 seconds |
| **History Buffer** | Last 50 readings in app memory |

```typescript
interface PowerReading {
  id: string;
  Phase1Current: number;
  Phase1Power: number;
  Phase1PowerFactor: number;
  Phase1Voltage: number;
  Phase2Current: number;
  Phase2Power: number;
  Phase2PowerFactor: number;
  Phase2Voltage: number;
  Phase3Current: number;
  Phase3Power: number;
  Phase3PowerFactor: number;
  Phase3Voltage: number;
}
```

### Additional Live Data Nodes

| Path | Type | Description |
|------|------|-------------|
| `/isRunning` | boolean | Motor running status at database root |
| `/message/{pushId}/message` | string | Fault/error messages |
| `/weatherData/{pushId}` | object | Weather sensor readings |

### Motor Status Sync (sasymithrajulyv1 Database)

| Parameter | Value |
|-----------|-------|
| **Path** | `users/{uid}/motors/{motorId}` |
| **Polling Interval** | 3 seconds |
| **Synced Fields** | run, pwr, hrs, kwh, phV, phI, pf, startAt, stopAt |
| **Comparison** | JSON stringify - only updates if data changed |

---

## 6. Motor Data Downlink (App to Device)

### Toggle Command Flow

When user toggles motor on/off:

**Step 1: State Update** (PATCH to Firebase)
```
Path: users/{uid}/motors/{motorId}
Method: fbPatch()
```

Starting motor:
```javascript
{
  run: true,
  pwr: 1.5-3.0,                    // Calculated power (kW)
  startAt: "2024-03-24T10:30:45Z",
  phV: { r: 228-234, y: 229-233, b: 227-232 },
  phI: { r: 3.5-5.0, y: 3.3-4.8, b: 3.5-5.0 }
}
```

Stopping motor:
```javascript
{
  run: false,
  pwr: 0,
  stopAt: "2024-03-24T10:30:45Z",
  phV: { r: 0, y: 0, b: 0 },
  phI: { r: 0, y: 0, b: 0 }
}
```

**Step 2: Command Log** (POST to Firebase)
```
Path: users/{uid}/cmdLog
Method: fbPush()
```

```javascript
{
  type: 'motor',
  deviceId: 'MOT-001-A1',
  action: 'start',         // or 'stop'
  ts: '2024-03-24T10:30:45Z'
}
```

### Full Command Schema (Production)

```typescript
type CommandAction = 'start' | 'stop' | 'set' | 'calibrate' | 'reboot';
type CommandStatus = 'pending' | 'sent' | 'acknowledged' | 'executed' | 'failed' | 'timeout';

interface Command {
  commandId: string;
  ts: number;
  deviceId: string;
  farmId: string;
  action: CommandAction;
  params?: Record<string, unknown>;
  source: {
    type: 'user' | 'schedule' | 'automation' | 'ai';
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

Command lifecycle:
```
[pending] --> [sent] --> [acknowledged] --> [executed]
                |               |
                +--[timeout]    +--[failed]
```

---

## 7. Motor Notifications

```typescript
interface MotorNotifications {
  on: boolean;     // Notify on motor power ON (default: true)
  off: boolean;    // Notify on motor power OFF (default: true)
  fault: boolean;  // Notify on motor fault (default: true)
}
```

All notifications default to `true` (enabled).

---

## 8. Normalization & Defaults

When a motor is created or loaded from Firebase, it is normalized to ensure all fields exist:

| Field | Default Value |
|-------|--------------|
| deviceId | `''` (or auto-generated `MOT-xxx`) |
| name | `'Unknown Motor'` |
| run | `false` |
| hp | `'good'` |
| pwr | `0` |
| hrs | `0` |
| kwh | `0` |
| startAt, stopAt | `''` |
| phV | `{ r: 0, y: 0, b: 0 }` |
| phI | `{ r: 0, y: 0, b: 0 }` |
| pf | `{ o: 0, r: 0, y: 0, b: 0 }` |
| starDelta | `5` |
| forceRun | `false` |
| mode | `'auto'` |
| prot | `{ dryRun: 30, overload: 15, overV: 260, underV: 180, restart: 60 }` |
| notif | `{ on: true, off: true, fault: true }` |
| alerts | `[]` |

---

## 9. Production Device Schema

Extended schema for production deployment (from database.ts):

```typescript
interface MotorDevice extends BaseDevice {
  type: 'motor';

  config: {
    ratedHP: number;            // Motor rated horsepower
    ratedVoltage: number;       // Rated voltage
    ratedCurrent: number;       // Rated current
    starDeltaDelay: number;     // Soft-start delay (seconds)
    phases: 1 | 3;              // Phase configuration
  };

  protection: {
    dryRunTimeout: number;      // Seconds
    overloadCurrent: number;    // Amps
    overVoltage: number;        // Volts
    underVoltage: number;       // Volts
    autoRestartDelay: number;   // Seconds
  };

  state: {
    running: boolean;
    power: number;              // kW
    runtime: number;            // Hours
    totalEnergy: number;        // kWh
    lastStartAt: string;
    lastStopAt: string;
  };

  sensors: string[];            // Linked sensor IDs
}
```

BaseDevice includes:
- `hardware`: manufacturer, model, firmwareVersion, macAddress, simNumber
- `connectivity`: protocol (mqtt/http/lora/gsm), broker, topic, lastSeen, online, signalStrength
- `location`: zone, coordinates, installationDate
- `meta`: createdAt, updatedAt, status (active/maintenance/offline/retired)

---

## 10. Sub-part: Motor UI

### Motor Card Layout

Each motor is displayed as a card showing:
- Name and device ID
- Status badge: "Running" (green) or "Idle" (gray)
- Live IoT indicator (green dot when connected to gsma7670c)
- Health status badge (good/warning/critical)
- Power consumption (kW)
- Runtime hours
- Energy consumed (kWh)
- Phase voltage display (R/Y/B in volts)
- Phase current display (R/Y/B in amps)
- Power factor (overall + per-phase)
- Toggle button (start/stop)
- Settings button

### Power Trend Chart

- Built with Recharts LineChart
- Shows last 20 power readings
- X-axis: time, Y-axis: power (kW)
- Updates with live data from gsma7670c

### Motor Fault Alert Banner

Displayed at top of motor card when faults detected. Shows fault message from live IoT database.

### MotorSettingsDialog (3 Tabs)

**General Tab:**
- Motor name (editable input)
- Device ID (read-only display)
- Star-delta delay (seconds input)
- Force run toggle
- Mode selector (auto/manual)

**Protection Tab:**
- Dry run cutoff timeout (seconds slider)
- Overload current threshold (amps slider)
- Over-voltage threshold (volts slider)
- Under-voltage threshold (volts slider)
- Auto-restart delay (seconds slider)

**Notifications Tab:**
- Power ON notification toggle
- Power OFF notification toggle
- Motor fault notification toggle

**Danger Zone:**
- Delete motor button with confirmation dialog

### Add Motor Modal

- Motor name input
- Device ID input (auto-generated if empty, format: MOT-xxx-xx)
- Motor is created with all default values
