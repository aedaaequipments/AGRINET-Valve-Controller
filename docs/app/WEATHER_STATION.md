# Weather Station - Hardware & Sensor System

## 1. Station Hardware Specifications

### Device Identity

| Parameter | Value |
|-----------|-------|
| **Device ID Format** | `WS-{NNN}-{LOCATION}` |
| **Example IDs** | WS-001-MAIN, WS-002-ORCH |
| **Default Sensor Count** | 9 sensors per station |
| **Reading Interval** | 15 seconds (configurable via `VITE_SENSOR_POLL_INTERVAL`) |
| **Connectivity** | Online/offline tracking with lastSeen timestamp |

### Runtime Data Model

```typescript
interface WeatherStation {
  id: string;                    // Firebase push ID (auto-generated)
  deviceId: string;              // Hardware ID, e.g., "WS-001-MAIN"
  name: string;                  // Display name, e.g., "Main Field Station"
  location: string;              // Location description, e.g., "North Field"
  zone?: string;                 // Optional zone assignment
  online: boolean;               // Current connectivity status
  lastSeen: string;              // ISO timestamp of last communication
  sensors: number;               // Number of sensors (default: 9)
  cropGroup?: string;            // Crop grouping, e.g., "paddy", "cotton"
  associatedValves?: string[];   // Valve IDs linked to this station
  associatedMotor?: string;      // Motor ID for safety interlocks
  coordinates?: { lat: number; lng: number };
  createdAt: string;
}
```

---

## 2. Sensor Inventory & Specifications

### Installed Sensor Types

| Sensor | Field | Abbreviation | Range | Precision | Unit |
|--------|-------|-------------|-------|-----------|------|
| Air Temperature | airT | at | -10 to +60 | +/-0.5 | C |
| Soil Temperature | soilT | st | -10 to +70 | +/-1.0 | C |
| Humidity | humid | h | 0-100 | +/-2 | % |
| Soil Moisture | soilM | sm | 0-100 | +/-1-2 | % VWC |
| Light Intensity | light | l | 0-10000+ | +/-5% | lux |
| Leaf Wetness #1 | leafW | lw | 0-100 | +/-5 | % |
| Leaf Wetness #2 | leafW2 | - | 0-100 | +/-5 | % |
| Wind Speed | wind | ws | 0-50 | +/-0.5 | m/s |
| Wind Direction | - | wd | 0-360 | +/-5 | degrees |
| Rainfall | rain | rf | 0-500 | +/-0.1 | mm/hr |
| Battery | batt | bt | 0-100 | +/-1 | % |
| pH | - | ph | 0-14 | +/-0.2 | - |
| EC (Electrical Conductivity) | - | ec | 0-5000 | +/-2% | uS/cm |

### Full SensorType Enumeration

```typescript
type SensorType =
  | 'soil_moisture' | 'soil_temperature' | 'air_temperature'
  | 'humidity' | 'light' | 'leaf_wetness'
  | 'wind_speed' | 'wind_direction' | 'rainfall'
  | 'ph' | 'ec' | 'battery'
  | 'voltage_r' | 'voltage_y' | 'voltage_b'
  | 'current_r' | 'current_y' | 'current_b'
  | 'power_factor' | 'flow_rate' | 'pressure';
```

---

## 3. Weather Data Structure

### Base Weather Data

```typescript
interface WeatherData {
  airT: number;     // Air Temperature (C)
  soilT: number;    // Soil Temperature (C)
  humid: number;    // Humidity (%)
  soilM: number;    // Soil Moisture (% VWC)
  light: number;    // Light Intensity (lux)
  leafW: number;    // Leaf Wetness (%)
  wind: number;     // Wind Speed (m/s)
  rain: number;     // Rainfall (mm)
  batt: number;     // Battery Level (%)
}
```

### Extended Sensor Reading

```typescript
interface SensorReading extends WeatherData {
  ts: string;             // ISO timestamp
  soilM1?: number;        // Stage 1 - Topsoil (0-15cm)
  soilM2?: number;        // Stage 2 - Mid (15-30cm)
  soilM3?: number;        // Stage 3 - Deep (30-45cm)
  leafW2?: number;        // Second leaf wetness sensor
}
```

---

## 4. 3-Stage Soil Moisture System

This is a critical feature for automated irrigation decisions.

### Depth Profile

```
  SOIL DEPTH PROFILE
  ==================

  Surface
  |
  |  L1 (0-15cm)    soilM1    Topsoil / Germination Zone
  |  +-----------+
  |  |           |   Source: soilM (base reading from sensor)
  |  +-----------+
  |
  |  L2 (15-30cm)   soilM2    Root Zone [PRIMARY DECISION LAYER]
  |  +-----------+
  |  |           |   Derivation: soilM * 0.92
  |  +-----------+
  |
  |  L3 (30-45cm)   soilM3    Deep Root Zone
  |  +-----------+
  |  |           |   Derivation: soilM * 0.85
  |  +-----------+
  |
  Deep
```

### Stage Configuration

| Stage | Depth | Field | Role | Derivation | Default Thresholds |
|-------|-------|-------|------|------------|-------------------|
| L1 | 0-15cm | soilM1 | Surface / germination | soilM (base) | min: 25%, max: 55% |
| L2 | 15-30cm | soilM2 | Root zone (PRIMARY) | soilM * 0.92 | min: 35%, max: 70% |
| L3 | 30-45cm | soilM3 | Deep roots | soilM * 0.85 | min: 30%, max: 60% |

- Range: 0-100% volumetric water content
- L2 (root zone) is the PRIMARY decision layer
- When L2 drops below min threshold, irrigation triggers
- When L2 reaches max threshold, irrigation stops

### Moisture Status Color Coding

| Status | Condition | Color |
|--------|-----------|-------|
| Optimal | Within min-max range | Green |
| Low | Below min threshold | Red/Amber |
| High | Above max threshold | Blue |

---

## 5. Dual Leaf Wetness Sensors

| Sensor | Position | Field | Purpose |
|--------|----------|-------|---------|
| LW1 | Upper canopy | leafW | Primary pest/disease risk indicator |
| LW2 | Lower canopy | leafW2 | Secondary validation sensor |

- Range: 0-100% (wetness hours / intensity)
- High leaf wetness (>60%) is a major trigger for pest and disease risk
- Both sensors contribute to risk scoring algorithms

---

## 6. Sensor Data Uplink

### Main Database Uploads (sasymithrajulyv1)

| Parameter | Value |
|-----------|-------|
| **Path** | `users/{uid}/sensorLog/{stationId}/{readingId}` |
| **Method** | `fbPush()` (auto-generated push ID) |
| **Interval** | Every 15 seconds (configurable via `VITE_SENSOR_POLL_INTERVAL`) |
| **Retention** | Last 100 readings per station (older trimmed client-side) |

Full reading payload:
```javascript
{
  ts: "2024-03-24T10:30:45.000Z",
  airT: 28,
  soilT: 22,
  humid: 65,
  soilM: 45,
  light: 8500,
  leafW: 35,
  wind: 2.5,
  rain: 0,
  batt: 92,
  soilM1: 45,      // Optional multi-stage
  soilM2: 41,
  soilM3: 38,
  leafW2: 38
}
```

### Live IoT Database Readings (gsma7670c)

| Parameter | Value |
|-----------|-------|
| **Path** | `/weatherData/{pushId}` |
| **Polling** | Every 5 seconds from app |
| **History Buffer** | Last 50 readings in app memory |

```typescript
interface WeatherReading {
  id: string;
  airTemperature: number;
  humidity: number;
  leafWetness: number;
  leafWetness2: number;
  lightIntensity: number;
  soilMoisture: number;
  soilMoisture2: number;      // Multi-stage soil moisture
  soilMoisture3: number;
  soilTemperature: number;
}
```

### Compact Storage Format

For bandwidth-optimized bulk storage:

```javascript
// Field abbreviation mapping
{
  ts: 'timestamp',      sm: 'soil_moisture',
  st: 'soil_temperature', at: 'air_temperature',
  h:  'humidity',        l:  'light',
  lw: 'leaf_wetness',   ws: 'wind_speed',
  wd: 'wind_direction', rf: 'rainfall',
  bt: 'battery'
}
```

```typescript
// Compact reading structure
interface CompactReading {
  ts: number;                           // Epoch timestamp
  d: string;                            // deviceId
  f: string;                            // farmId
  v: Record<string, number>;            // sensor values
  q?: { ss: number; bl: number };       // quality (signal, battery)
}
```

---

## 7. Derived Calculations

Advanced analytics derived from raw sensor data:

```typescript
derived?: {
  vpd?: number;    // Vapor Pressure Deficit (kPa)
                   // Indicates plant transpiration stress
                   // High VPD = dry air, plants need more water

  eto?: number;    // Reference Evapotranspiration (mm/day)
                   // Estimates crop water demand
                   // Used for irrigation scheduling

  gdd?: number;    // Growing Degree Days
                   // Tracks crop thermal maturity
                   // Accumulated heat units for growth stage prediction
}
```

---

## 8. Pest & Disease Risk Algorithms

### Pest Risk Scoring

Calculated from weather conditions on each valve card:

| Factor | Condition | Points |
|--------|-----------|--------|
| High Humidity | humidity > 80% | 30 |
| High Leaf Wetness | leafW > 60% | 35 |
| Favorable Temperature | 20-30 C range | 20 |

Maximum score: ~85 points

| Score Range | Risk Level | Badge Color |
|-------------|-----------|-------------|
| < 30 | Low | Green |
| 30-60 | Medium | Amber |
| > 60 | High | Red |

### Disease Risk Scoring

| Factor | Condition | Points |
|--------|-----------|--------|
| Temperature + Humidity Combo | Both elevated (temp 15-30, humid > 70%) | 40 |
| Extended Leaf Wetness | leafW > 60% prolonged period | 30 |
| Moderate Temperature | 15-25 C (optimal for many fungi) | 20 |

Maximum score: ~90 points

| Score Range | Risk Level | Badge Color |
|-------------|-----------|-------------|
| < 30 | Low | Green |
| 30-60 | Medium | Amber |
| > 60 | High | Red |

### Prediction Output (Weather Page)

- **Next Probable Pest**: name (e.g., "Aphids"), probability %, timeline
- **Next Probable Disease**: name (e.g., "Early Blight"), severity level
- **Agricultural Recommendations**:
  - Next irrigation timing
  - Next harvest estimate
  - Weeding schedule

---

## 9. Crop Group Association

Weather stations can serve multiple valves through crop grouping:

| Field | Purpose |
|-------|---------|
| `cropGroup` | Groups valves by crop type (e.g., "paddy", "cotton") |
| `associatedValves` | Explicit list of valve IDs linked to this station |
| `associatedMotor` | Motor ID for safety interlocks |

### Linking Rules

- A valve's `primaryStationId` determines which station drives its auto-irrigation
- A valve's `secondaryStationId` provides validation/averaging data
- When a station goes offline, linked valves show offline indicator
- Multiple valves can share the same station if they grow the same crop

---

## 10. Production Device Schema

Extended schema for production deployment:

```typescript
interface WeatherStationDevice extends BaseDevice {
  type: 'weather_station';

  config: {
    sensorTypes: SensorType[];     // Installed sensor types
    readingInterval: number;        // ms between sensor reads
    transmitInterval: number;       // ms between data transmits to cloud
  };

  calibration: {
    lastCalibrationDate: string;
    nextCalibrationDue: string;
    calibrationFactors: Record<string, number>;  // Per-sensor calibration
  };

  sensors: string[];               // Individual sensor IDs
}
```

### Individual Sensor Schema

```typescript
interface Sensor {
  sensorId: string;
  deviceId: string;                // Parent station device ID
  farmId: string;
  type: SensorType;
  unit: string;                    // e.g., "C", "%", "lux", "m/s"

  range: {
    min: number;
    max: number;
    precision: number;
  };

  thresholds: {
    warningLow?: number;
    warningHigh?: number;
    criticalLow?: number;
    criticalHigh?: number;
  };

  calibration: {
    offset: number;
    scale: number;
    lastCalibrated: string;
  };

  meta: {
    createdAt: string;
    status: 'active' | 'faulty' | 'replaced';
  };
}
```

---

## 11. Station Quality Metrics

```typescript
quality: {
  signalStrength: number;    // RSSI (0-100 scale)
  batteryLevel: number;      // % (0-100)
  errors: string[];          // List of detected errors
}
```

---

## 12. Normalization & Defaults

When a station is created or loaded from Firebase:

| Field | Default Value |
|-------|--------------|
| deviceId | `''` (or auto-generated `WS-xxx`) |
| name | `'Unknown Station'` |
| location | `''` |
| online | `true` |
| lastSeen | Current ISO timestamp |
| sensors | `9` |

---

## 13. Sub-part: Weather Station UI

### Sensor Grid Display

Cards organized in sections:

**Air & Environment Sensors:**
- Air temperature, humidity, light intensity, battery

**Soil Moisture (3-Stage):**
- L1 (Topsoil 0-15cm) with status indicator
- L2 (Mid 15-30cm) with status indicator
- L3 (Deep 30-45cm) with status indicator
- Soil temperature

**Leaf Wetness Sensors:**
- LW1 (Upper canopy)
- LW2 (Lower canopy)
- Wind speed, rainfall

### Sensor Log Table

Expandable per station, shows last 30 readings:

| Column | Field |
|--------|-------|
| Time | ts (relative or absolute) |
| Air C | airT |
| Soil C | soilT |
| Humid% | humid |
| S1% | soilM1 |
| S2% | soilM2 |
| S3% | soilM3 |
| Light% | light |
| LW1% | leafW |
| LW2% | leafW2 |
| Wind | wind |
| Rain | rain |
| Batt% | batt |

Moisture values are color-coded: green (optimal), amber (warning), red (critical).

### Pest & Disease Prediction Cards

- Next probable pest with probability and timeline
- Next probable disease with severity level
- Confidence percentages

### Agricultural Recommendations

- Next irrigation timing estimate
- Next harvest estimate
- Weeding schedule recommendation

### Station Management

- Add station modal: name, device ID, location
- Delete station with confirmation
- Station list with online/offline status indicators
- WeatherStationDialog: link stations to valve zones and crop groups
