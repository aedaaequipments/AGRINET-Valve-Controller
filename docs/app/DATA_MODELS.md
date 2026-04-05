# Data Models - Complete Type Definitions & Schemas

## Entity Relationship Diagram

```
  ENTITY RELATIONSHIPS
  ====================

  User (profile)
    |
    +--[uid]--> motors/{motorId}
    |              ^
    |              |  [mid]
    +--[uid]--> valves/{valveId}
    |              |
    |              +--[primaryStationId]--> stations/{stationId}
    |              +--[secondaryStationId]--> stations/{stationId}
    |              +--[cropSurvey] (embedded)
    |              +--[valveAlerts] (embedded)
    |
    +--[uid]--> stations/{stationId}
    |              +--[associatedValves] --> valves[]
    |              +--[associatedMotor] --> motor
    |              +--[cropGroup] --> groups valves by crop
    |
    +--[uid]--> alerts/{alertId}
    |
    +--[uid]--> sensorLog/{stationId}/{readingId}
    |
    +--[uid]--> cmdLog/{commandId}
```

---

## Runtime Types (web-src/types/index.ts)

### User & Authentication

```typescript
interface User {
  uid: string;
  email: string;
  token: string;          // Firebase JWT idToken
  profile: UserProfile;
}

interface UserProfile {
  name: string;
  phone?: string;
  role: 'farmer' | 'technician' | 'admin' | 'seed';
  district?: string;
  mandal?: string;
  village?: string;
  farm?: string;
  crop?: string;
  acreage?: string;
  soilType?: string;
  irrigationType?: string;
  previousCrop?: string;
  sowingDate?: string;
  expectedHarvest?: string;
  createdAt: string;
}
```

### Motor

```typescript
interface PhaseData {
  r: number;              // Red phase
  y: number;              // Yellow phase
  b: number;              // Blue phase
}

interface PowerFactor {
  o: number;              // Overall
  r: number;              // Red phase
  y: number;              // Yellow phase
  b: number;              // Blue phase
}

interface MotorProtection {
  dryRun: number;         // Seconds (default: 30)
  overload: number;       // Amps (default: 15)
  overV: number;          // Volts (default: 260)
  underV: number;         // Volts (default: 180)
  restart: number;        // Seconds (default: 60)
}

interface MotorNotifications {
  on: boolean;            // Power ON notification
  off: boolean;           // Power OFF notification
  fault: boolean;         // Fault notification
}

interface MotorAlert {
  msg: string;            // Alert message text
  sev: 'warning' | 'critical';
}

interface Motor {
  id: string;             // Firebase push ID
  deviceId: string;       // Hardware ID (MOT-xxx-xx)
  name: string;
  run: boolean;           // Running state
  hp: 'good' | 'warning' | 'critical';  // Health
  pwr: number;            // Power (kW)
  hrs: number;            // Runtime (hours)
  kwh: number;            // Energy (kWh)
  startAt: string;        // Last start ISO timestamp
  stopAt: string;         // Last stop ISO timestamp
  phV: PhaseData;         // Phase voltages
  phI: PhaseData;         // Phase currents
  pf: PowerFactor;        // Power factors
  starDelta: number;      // Soft-start delay (seconds)
  forceRun: boolean;      // Override safety
  mode: 'auto' | 'manual';
  prot: MotorProtection;
  notif: MotorNotifications;
  alerts: MotorAlert[];
  waterPumped?: number;   // Optional volume tracking
  createdAt: string;
}
```

### Valve

```typescript
interface SoilLevelThreshold {
  min: number;            // Irrigation trigger (%)
  max: number;            // Irrigation stop (%)
  enabled: boolean;
}

interface ValveAuto {
  on: boolean;
  stage?: 1 | 2 | 3;     // Legacy single-stage
  min?: number;           // Legacy single threshold
  max?: number;           // Legacy single threshold
  L1?: SoilLevelThreshold;  // Surface (0-10cm)
  L2?: SoilLevelThreshold;  // Root zone (15-20cm) PRIMARY
  L3?: SoilLevelThreshold;  // Deep (25-30cm)
  env: boolean;           // Environmental adjustment
}

interface ValveQuantity {
  liters: number;         // Target volume (0-5000 L)
  cyclic: boolean;        // Repeat cycles
}

interface ValveSchedule {
  recur: boolean;         // Recurring schedule
  date?: string;          // One-time date
  start: string;          // HH:MM
  end: string;            // HH:MM
  dur: number;            // Duration (minutes)
}

interface ValveAlerts {
  pest: boolean;
  disease: boolean;
  lowM: boolean;          // Low moisture
  excess: boolean;        // Excess water
}

interface Valve {
  id: string;
  deviceId: string;       // Hardware ID (VLV-xxx-xx)
  name: string;
  isOpen: boolean;
  flow: number;           // L/min
  togAt: string;          // Last toggle timestamp
  mid: string;            // Motor ID (foreign key)
  zone: string;
  stationId: string;      // Legacy station reference
  primaryStationId?: string;
  secondaryStationId?: string;
  cropType: string;
  color: string;          // Hex color (default: #3B82F6)
  controlMode: 'auto' | 'quantity' | 'schedule' | 'manual';
  forceRun: boolean;
  safeMode: boolean;
  auto: ValveAuto;
  qty: ValveQuantity;
  sched: ValveSchedule;
  valveAlerts: ValveAlerts;
  cropSurvey?: CropSurvey;
  createdAt: string;
}
```

### Crop Survey

```typescript
interface CropSurvey {
  // Farm Info
  farmName?: string;
  totalAcres?: number;
  village?: string;
  mandal?: string;
  district?: string;
  state?: string;
  coordinates?: { lat: number; lng: number };

  // Soil Profile
  soilType?: 'black_cotton' | 'red' | 'alluvial' | 'laterite' | 'sandy' | 'clayey' | 'loamy' | 'mixed';
  soilPh?: number;
  organicCarbonPct?: number;
  ecDsM?: number;
  npkAvailable?: { n: number; p: number; k: number };
  lastSoilTestDate?: string;

  // Crop Details
  cropName: string;
  variety?: string;
  sowingDate: string;
  cropAgeDays?: number;
  growthStage?: 'germination' | 'seedling' | 'vegetative' | 'tillering' | 'flowering' | 'grain_filling' | 'maturity';
  expectedHarvest?: string;
  croppingPattern?: 'kharif' | 'rabi' | 'zaid' | 'annual' | 'perennial';
  previousCrop?: string;
  plantedArea?: number;

  // Irrigation Profile
  irrigationMethod?: 'drip' | 'sprinkler' | 'flood' | 'furrow' | 'center_pivot' | 'rainfed';
  waterSource?: 'borewell' | 'canal' | 'river' | 'pond' | 'tank' | 'rainwater';
  irrigationFrequencyDays?: number;
  irrigationTiming?: string[];
  irrigationDurationMin?: number;

  // Fertigation History
  fertigationType?: 'organic' | 'chemical' | 'mixed' | 'drip_fertigation' | 'manual_broadcast';
  previousFertigationType?: string;
  applications?: FertigationRecord[];

  // Meta
  surveyedAt?: string;
  lastUpdated?: string;
}

interface FertigationRecord {
  product: string;
  dosageKgPerAcre: number;
  date: string;
  method: 'drip' | 'broadcast' | 'foliar' | 'band';
  growthStage?: string;
}
```

### Weather Station

```typescript
interface WeatherStation {
  id: string;
  deviceId: string;       // Hardware ID (WS-xxx-xx)
  name: string;
  location: string;
  zone?: string;
  online: boolean;
  lastSeen: string;
  sensors: number;        // Default: 9
  cropGroup?: string;     // Groups valves by crop type
  associatedValves?: string[];
  associatedMotor?: string;
  coordinates?: { lat: number; lng: number };
  createdAt: string;
}
```

### Weather & Sensor Data

```typescript
interface WeatherData {
  airT: number;           // Air Temperature (C)
  soilT: number;          // Soil Temperature (C)
  humid: number;          // Humidity (%)
  soilM: number;          // Soil Moisture (%)
  light: number;          // Light Intensity (lux)
  leafW: number;          // Leaf Wetness (%)
  wind: number;           // Wind Speed (m/s)
  rain: number;           // Rainfall (mm)
  batt: number;           // Battery (%)
}

interface SensorReading extends WeatherData {
  ts: string;             // ISO timestamp
  soilM1?: number;        // Stage 1 topsoil (0-15cm)
  soilM2?: number;        // Stage 2 mid (15-30cm)
  soilM3?: number;        // Stage 3 deep (30-45cm)
  leafW2?: number;        // Second leaf wetness sensor
}
```

### Alert

```typescript
interface Alert {
  id: string;
  name: string;
  type: 'pest' | 'disease' | 'weather';
  sev: 'high' | 'medium' | 'low';
  conf: number;           // Confidence 0-100
  at: string;             // ISO timestamp
  area: string;           // Zone/location affected
  action: string;         // Recommended action text
  ack: boolean;           // Acknowledged flag
}
```

### Command Log

```typescript
interface CommandLog {
  type: 'valve' | 'motor';
  deviceId: string;
  action: string;         // start|stop|open|close
  ts: string;             // ISO timestamp
}

type ToastType = 'success' | 'error' | 'warning';
```

---

## Live Data Types (web-src/contexts/LiveDataContext.tsx)

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

interface WeatherReading {
  id: string;
  airTemperature: number;
  humidity: number;
  leafWetness: number;
  leafWetness2: number;
  lightIntensity: number;
  soilMoisture: number;
  soilMoisture2: number;
  soilMoisture3: number;
  soilTemperature: number;
}

interface FaultMessage {
  id: string;
  message: string;
}
```

---

## Production Database Schema (web-src/types/database.ts)

### Base Device

```typescript
type DeviceType = 'motor' | 'valve' | 'weather_station';
type DeviceStatus = 'active' | 'maintenance' | 'offline' | 'retired';
type ConnectivityProtocol = 'mqtt' | 'http' | 'lora' | 'gsm';

interface BaseDevice {
  deviceId: string;
  farmId: string;
  type: DeviceType;
  hardware: {
    manufacturer: string;
    model: string;
    firmwareVersion: string;
    macAddress: string;
    simNumber?: string;
  };
  connectivity: {
    protocol: ConnectivityProtocol;
    broker?: string;
    topic?: string;
    lastSeen: string;
    online: boolean;
    signalStrength: number;
  };
  location: {
    zone: string;
    coordinates?: { lat: number; lng: number };
    installationDate: string;
  };
  meta: {
    createdAt: string;
    updatedAt: string;
    status: DeviceStatus;
  };
}
```

### Sensor

```typescript
type SensorType =
  | 'soil_moisture' | 'soil_temperature' | 'air_temperature'
  | 'humidity' | 'light' | 'leaf_wetness'
  | 'wind_speed' | 'wind_direction' | 'rainfall'
  | 'ph' | 'ec' | 'battery'
  | 'voltage_r' | 'voltage_y' | 'voltage_b'
  | 'current_r' | 'current_y' | 'current_b'
  | 'power_factor' | 'flow_rate' | 'pressure';

interface Sensor {
  sensorId: string;
  deviceId: string;
  farmId: string;
  type: SensorType;
  unit: string;
  range: { min: number; max: number; precision: number; };
  thresholds: {
    warningLow?: number;
    warningHigh?: number;
    criticalLow?: number;
    criticalHigh?: number;
  };
  calibration: { offset: number; scale: number; lastCalibrated: string; };
  meta: { createdAt: string; status: 'active' | 'faulty' | 'replaced'; };
}
```

### Sensor Reading (Production)

```typescript
interface SensorReading {
  ts: number;
  deviceId: string;
  farmId: string;
  values: Record<string, number>;
  quality: {
    signalStrength: number;
    batteryLevel: number;
    errors: string[];
  };
  derived?: {
    vpd?: number;     // Vapor Pressure Deficit
    eto?: number;     // Evapotranspiration
    gdd?: number;     // Growing Degree Days
  };
}

// Compact reading for bulk storage
interface CompactReading {
  ts: number;
  d: string;                        // deviceId
  f: string;                        // farmId
  v: Record<string, number>;        // values
  q?: { ss: number; bl: number };   // quality (signal, battery)
}
```

### Command (Production)

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
  source: { type: CommandSource; userId?: string; ruleId?: string; };
  status: CommandStatus;
  response?: {
    receivedAt: number;
    executedAt: number;
    result: 'success' | 'error';
    message?: string;
  };
}
```

### Alert (Production)

```typescript
type AlertType = 'pest' | 'disease' | 'weather' | 'device' | 'irrigation' | 'system';
type AlertSeverity = 'info' | 'warning' | 'critical';
type AlertSource = 'sensor' | 'rule' | 'ai' | 'schedule' | 'manual';

interface Alert {
  alertId: string;
  ts: number;
  farmId: string;
  deviceId?: string;
  type: AlertType;
  severity: AlertSeverity;
  source: { type: AlertSource; model?: string; ruleId?: string; };
  data: {
    title: string;
    message: string;
    confidence?: number;
    affectedArea?: string;
    recommendations: string[];
    relatedSensors?: string[];
    triggerValues?: Record<string, number>;
  };
  status: {
    acknowledged: boolean;
    acknowledgedBy?: string;
    acknowledgedAt?: number;
    resolved: boolean;
    resolvedAt?: number;
    notes?: string;
  };
}
```

### Farm

```typescript
interface Farm {
  farmId: string;
  ownerId: string;
  info: {
    name: string;
    district: string;
    mandal: string;
    village: string;
    pincode: string;
    coordinates: { lat: number; lng: number };
    totalAcres: number;
    soilType: 'clay' | 'loam' | 'sandy' | 'silt' | 'mixed';
    waterSource: 'borewell' | 'canal' | 'river' | 'tank' | 'rain';
  };
  devices: { motors: string[]; valves: string[]; stations: string[]; };
  crops: { current: CropInfo[]; history: CropHistory[]; };
  meta: { createdAt: string; updatedAt: string; status: 'active' | 'inactive' | 'archived'; };
}
```

### Analytics Bucket

```typescript
type AnalyticsPeriod = 'hourly' | 'daily' | 'weekly' | 'monthly';

interface AnalyticsBucket {
  ts: number;
  farmId: string;
  period: AnalyticsPeriod;
  weather: {
    temperature: { min: number; max: number; avg: number };
    humidity: { min: number; max: number; avg: number };
    soilMoisture: { min: number; max: number; avg: number };
    rainfall: { total: number };
    sunlight: { total: number };
  };
  irrigation: {
    totalWater: number;
    totalRuntime: number;
    cycleCount: number;
    efficiency?: number;
  };
  energy: {
    totalKwh: number;
    peakPower: number;
    avgPowerFactor: number;
    runtime: number;
  };
  alerts: {
    total: number;
    bySeverity: { info: number; warning: number; critical: number };
    byType: Record<string, number>;
  };
  predictions?: {
    pestRisk: number;
    diseaseRisk: number;
    irrigationNeeded: number;
    nextIrrigationAt?: number;
  };
}
```

---

## Field Abbreviation Mapping (Compact Storage)

```typescript
const FIELD_ABBREV: Record<string, string> = {
  ts: 'timestamp',
  sm: 'soil_moisture',
  st: 'soil_temperature',
  at: 'air_temperature',
  h:  'humidity',
  l:  'light',
  lw: 'leaf_wetness',
  ws: 'wind_speed',
  wd: 'wind_direction',
  rf: 'rainfall',
  bt: 'battery',
  vr: 'voltage_r',
  vy: 'voltage_y',
  vb: 'voltage_b',
  ir: 'current_r',
  iy: 'current_y',
  ib: 'current_b',
  pf: 'power_factor',
  pw: 'power',
};
```

---

## Normalization Functions

The app normalizes all data from Firebase to ensure complete objects with defaults.

### Motor Defaults

| Field | Default Value |
|-------|--------------|
| name | 'Unknown Motor' |
| run | false |
| hp | 'good' |
| pwr, hrs, kwh | 0 |
| phV, phI | { r: 0, y: 0, b: 0 } |
| pf | { o: 0, r: 0, y: 0, b: 0 } |
| starDelta | 5 |
| forceRun | false |
| mode | 'auto' |
| prot | { dryRun: 30, overload: 15, overV: 260, underV: 180, restart: 60 } |
| notif | { on: true, off: true, fault: true } |

### Valve Defaults

| Field | Default Value |
|-------|--------------|
| name | 'Unknown Valve' |
| isOpen | false |
| flow | 0 |
| color | '#3B82F6' |
| controlMode | 'manual' |
| safeMode | true |
| auto | { on: true, stage: 1, min: 30, max: 45, env: true } |
| qty | { liters: 1000, cyclic: false } |
| sched | { recur: false, start: '06:00', end: '07:00', dur: 60 } |
| valveAlerts | { pest: true, disease: true, lowM: true, excess: true } |

### Station Defaults

| Field | Default Value |
|-------|--------------|
| name | 'Unknown Station' |
| online | true |
| sensors | 9 |

### Alert Defaults

| Field | Default Value |
|-------|--------------|
| name | 'Unknown Alert' |
| type | 'weather' |
| sev | 'low' |
| conf | 0 |
| ack | false |
