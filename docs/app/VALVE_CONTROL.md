# Valve Control - Hardware & Irrigation System

## 1. Valve Hardware Specifications

### Device Identity

| Parameter | Value |
|-----------|-------|
| **Device ID Format** | `VLV-{NNN}-{ZONE}` |
| **Example IDs** | VLV-001-A1, VLV-002-B1, VLV-003-C1 |
| **Valve Types** | Solenoid, Motorized, Manual |
| **Diameter Range** | 20-75mm (configurable) |
| **Flow Measurement** | Liters/minute |

### Entity Relationships

```
  VALVE ENTITY RELATIONSHIPS
  ==========================

  Valve ----[mid]----> Motor (1:1, required)
    |
    +--[primaryStationId]----> Weather Station (1:1, required for auto mode)
    |
    +--[secondaryStationId]--> Weather Station (1:1, optional validation)
    |
    +--[cropSurvey]----------> CropSurvey (embedded 1:1)
    |
    +--[valveAlerts]----------> Alert preferences (embedded)
```

### Runtime Data Model

```typescript
interface Valve {
  id: string;                    // Firebase push ID (auto-generated)
  deviceId: string;              // Hardware ID, e.g., "VLV-001-A1"
  name: string;                  // Display name, e.g., "North Field Drip"
  isOpen: boolean;               // Current open/closed state
  flow: number;                  // Current flow rate (L/min)
  togAt: string;                 // ISO timestamp of last toggle
  mid: string;                   // Motor ID this valve connects to
  zone: string;                  // Zone name
  stationId: string;             // Legacy station ID field
  primaryStationId?: string;     // Primary weather station (irrigation decisions)
  secondaryStationId?: string;   // Optional secondary (validation/averaging)
  cropType: string;              // e.g., "Paddy", "Cotton", "Mango"
  color: string;                 // Hex color for UI (default: "#3B82F6")
  controlMode: 'auto' | 'quantity' | 'schedule' | 'manual';
  forceRun: boolean;             // Override safety checks
  safeMode: boolean;             // Safe mode enabled (default: true)
  auto: ValveAuto;               // Auto mode configuration
  qty: ValveQuantity;            // Quantity mode configuration
  sched: ValveSchedule;          // Schedule mode configuration
  valveAlerts: ValveAlerts;      // Alert preferences
  cropSurvey?: CropSurvey;      // Comprehensive crop data
  createdAt: string;
}
```

---

## 2. Four Control Modes

### 2.1 Auto Mode - 3-Stage Soil Moisture Automation

The most sophisticated control mode, using multi-depth soil moisture thresholds.

```typescript
interface SoilLevelThreshold {
  min: number;       // Trigger irrigation when below this (%)
  max: number;       // Stop irrigation when above this (%)
  enabled: boolean;  // Enable/disable this depth level
}

interface ValveAuto {
  on: boolean;                    // Enable auto irrigation
  stage?: 1 | 2 | 3;             // Legacy single-stage (backwards compat)
  min?: number;                   // Legacy single threshold
  max?: number;                   // Legacy single threshold
  L1?: SoilLevelThreshold;       // Surface (0-10cm)
  L2?: SoilLevelThreshold;       // Root Zone (15-20cm) - PRIMARY
  L3?: SoilLevelThreshold;       // Deep (25-30cm)
  env: boolean;                   // Environmental adjustment enabled
}
```

### Auto-Irrigation Logic

```
  3-STAGE AUTO-IRRIGATION DECISION SYSTEM
  ========================================

  Soil Depth      Threshold Range    Default     Role
  -----------     ---------------    -------     ----
  L1 (0-10cm)     min: 25%, max: 55%   Amber     Surface monitoring
  Surface

  L2 (15-20cm)    min: 35%, max: 70%   Green     PRIMARY CONTROLLER
  Root Zone                                       Controls start/stop
  [PRIMARY]

  L3 (25-30cm)    min: 30%, max: 60%   Blue      Deep root monitoring
  Deep

  DECISION LOGIC:
  +----------------------------------------------------+
  | IF any enabled level drops BELOW its min:           |
  |   --> START irrigation                              |
  |                                                     |
  | IF L2 (root zone, PRIMARY) reaches its max:         |
  |   --> STOP irrigation                               |
  |                                                     |
  | L2 is always the PRIMARY stop controller.           |
  +----------------------------------------------------+

  ENVIRONMENTAL ADJUSTMENT (when env: true):
  +----------------------------------------------------+
  | Adjusts thresholds based on:                        |
  |   - Air temperature                                 |
  |   - Soil temperature                                |
  |   - Humidity                                        |
  |   - Light intensity                                 |
  | Estimates irrigation duration from current readings |
  +----------------------------------------------------+
```

### 2.2 Quantity Mode

Fixed water volume delivery per cycle.

```typescript
interface ValveQuantity {
  liters: number;      // Target water volume (0-5000 liters)
  cyclic: boolean;     // Repeat cycle (true) or one-time (false)
}
```

| Parameter | Range | Default |
|-----------|-------|---------|
| Target Volume | 0-5000 L | 1000 L |
| Cyclic Repeat | on/off | off |

### 2.3 Schedule Mode

Time-based irrigation scheduling.

```typescript
interface ValveSchedule {
  recur: boolean;     // Recurring schedule (true) or one-time (false)
  date?: string;      // Date for one-time execution (optional)
  start: string;      // Start time (HH:MM format)
  end: string;        // End time (HH:MM format)
  dur: number;        // Duration in minutes
}
```

| Parameter | Range | Default |
|-----------|-------|---------|
| Start Time | HH:MM | 06:00 |
| End Time | HH:MM | 07:00 |
| Duration | 5-480 minutes | 60 min |
| Recurring | on/off | off |

### 2.4 Manual Mode

Direct toggle control. User presses open/close button in the app. No automation logic.

---

## 3. Valve Data Uplink (Device to Cloud)

### State Synchronization

| Parameter | Value |
|-----------|-------|
| **Path** | `users/{uid}/valves/{valveId}` |
| **Polling Interval** | 3 seconds |
| **Method** | `fbGet()` with JSON comparison |
| **Synced Fields** | isOpen, flow, togAt |

Only updates React state if valve data has changed (prevents unnecessary re-renders).

---

## 4. Valve Data Downlink (App to Device)

### Toggle Command Flow

**Step 1: State Update** (PATCH to Firebase)
```
Path: users/{uid}/valves/{valveId}
Method: fbPatch()
```

Opening valve:
```javascript
{
  isOpen: true,
  flow: 5-18,                       // Random flow rate (L/min)
  togAt: "2024-03-24T10:30:45Z"
}
```

Closing valve:
```javascript
{
  isOpen: false,
  flow: 0,
  togAt: "2024-03-24T10:30:45Z"
}
```

**Step 2: Command Log** (POST to Firebase)
```
Path: users/{uid}/cmdLog
Method: fbPush()
```

```javascript
{
  type: 'valve',
  deviceId: 'VLV-001-A1',
  action: 'open',              // or 'close'
  ts: '2024-03-24T10:30:45Z'
}
```

### Configuration Updates

Control mode changes, threshold adjustments, and schedule modifications are patched directly to the valve object via `fbPatch()`.

---

## 5. Valve Alert System

```typescript
interface ValveAlerts {
  pest: boolean;       // Pest alert notifications (default: true)
  disease: boolean;    // Disease alert notifications (default: true)
  lowM: boolean;       // Low moisture alerts (default: true)
  excess: boolean;     // Excess water alerts (default: true)
}
```

All alerts default to `true` (enabled). Configurable per-valve in ValveSettingsDialog.

---

## 6. Crop Survey Integration

Comprehensive agricultural data collection per valve zone:

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
  soilType?: 'black_cotton' | 'red' | 'alluvial' | 'laterite'
           | 'sandy' | 'clayey' | 'loamy' | 'mixed';
  soilPh?: number;                // pH value
  organicCarbonPct?: number;      // Organic carbon percentage
  ecDsM?: number;                 // Electrical conductivity (dS/m)
  npkAvailable?: {                // Available nutrients
    n: number;                    // Nitrogen (kg/ha)
    p: number;                    // Phosphorus (kg/ha)
    k: number;                    // Potassium (kg/ha)
  };
  lastSoilTestDate?: string;

  // Crop Details
  cropName: string;               // REQUIRED
  variety?: string;               // Crop variety
  sowingDate: string;             // REQUIRED - ISO date
  cropAgeDays?: number;           // Calculated crop age
  growthStage?: 'germination' | 'seedling' | 'vegetative'
              | 'tillering' | 'flowering' | 'grain_filling' | 'maturity';
  expectedHarvest?: string;
  croppingPattern?: 'kharif' | 'rabi' | 'zaid' | 'annual' | 'perennial';
  previousCrop?: string;
  plantedArea?: number;           // Acres

  // Irrigation Profile
  irrigationMethod?: 'drip' | 'sprinkler' | 'flood' | 'furrow'
                   | 'center_pivot' | 'rainfed';
  waterSource?: 'borewell' | 'canal' | 'river' | 'pond' | 'tank' | 'rainwater';
  irrigationFrequencyDays?: number;
  irrigationTiming?: string[];
  irrigationDurationMin?: number;

  // Fertigation History
  fertigationType?: 'organic' | 'chemical' | 'mixed'
                  | 'drip_fertigation' | 'manual_broadcast';
  previousFertigationType?: string;
  applications?: FertigationRecord[];

  // Meta
  surveyedAt?: string;
  lastUpdated?: string;
}

interface FertigationRecord {
  product: string;                // Product name
  dosageKgPerAcre: number;       // Application rate
  date: string;                  // Application date
  method: 'drip' | 'broadcast' | 'foliar' | 'band';
  growthStage?: string;          // Stage at time of application
}
```

This data improves pest/disease prediction accuracy and enables personalized irrigation recommendations.

---

## 7. Production Device Schema

Extended schema for production deployment (from database.ts):

```typescript
type ValveControlMode = 'auto' | 'schedule' | 'quantity' | 'manual';

interface ValveDevice extends BaseDevice {
  type: 'valve';

  config: {
    valveType: 'solenoid' | 'motorized' | 'manual';
    diameter: number;          // Pipe diameter (mm)
    flowRate: number;          // Rated flow (L/min or m3/hr)
    motorId: string;           // Linked motor device ID
    stationId: string;         // Linked weather station ID
  };

  control: {
    mode: ValveControlMode;
    autoConfig?: {
      minMoisture: number;
      maxMoisture: number;
      growthStage: 1 | 2 | 3;
      useEnvironmental: boolean;
    };
    scheduleConfig?: {
      recurring: boolean;
      startTime: string;
      duration: number;
      days: number[];          // Day-of-week array
    };
    quantityConfig?: {
      targetLiters: number;
      cyclic: boolean;
    };
  };

  state: {
    open: boolean;
    flowRate: number;          // Current L/min
    totalVolume: number;       // Cumulative liters
    lastToggleAt: string;
  };

  cropInfo: {
    cropType: string;
    variety: string;
    plantedDate: string;
  };
}
```

---

## 8. Normalization & Defaults

When a valve is created or loaded from Firebase:

| Field | Default Value |
|-------|--------------|
| deviceId | `''` (or auto-generated `VLV-xxx`) |
| name | `'Unknown Valve'` |
| isOpen | `false` |
| flow | `0` |
| togAt | Current ISO timestamp |
| mid | `''` |
| zone | `''` |
| stationId | `''` |
| cropType | `'Unknown'` |
| color | `'#3B82F6'` |
| controlMode | `'manual'` |
| forceRun | `false` |
| safeMode | `true` |
| auto | `{ on: true, stage: 1, min: 30, max: 45, env: true }` |
| qty | `{ liters: 1000, cyclic: false }` |
| sched | `{ recur: false, start: '06:00', end: '07:00', dur: 60 }` |
| valveAlerts | `{ pest: true, disease: true, lowM: true, excess: true }` |

---

## 9. Sub-part: Valve UI

### Valve Card Layout

Each valve is displayed as a card showing:
- Name and device ID
- Open/Closed toggle badge (green/gray)
- Zone and crop type
- Associated motor name
- Weather station link with online status indicator
- **Weather Conditions 6-Grid:**

| Cell | Data | Display |
|------|------|---------|
| Soil Moisture | % VWC | Status color (green/amber/red) |
| Air Temperature | C | Numeric value |
| Humidity | % | Numeric value |
| Leaf Wetness | % | Numeric value |
| Pest Risk | Low/Med/High | Colored badge |
| Disease Risk | Low/Med/High | Colored badge |

- Risk alert banner (shown when pest or disease risk is High)
- Action buttons row:

| Button | Opens | Purpose |
|--------|-------|---------|
| Schedule | ConfigureValveDialog | Configure valve operation mode |
| Weather | WeatherStationDialog | Link weather station |
| Motor | Motor control | Motor management |
| Settings | ValveSettingsDialog | Valve settings |

### ConfigureValveDialog

**Tabs:** Configure | View Schedules

**Configure Tab:**

4 mode selector buttons with icons:
- Auto (leaf icon)
- Quantity (droplet icon)
- Schedule (clock icon)
- Manual (hand icon)
- End Crop button (terminates crop cycle)

**Auto Mode UI:**
- 3 depth sections (L1, L2, L3) each with:
  - Enable/disable toggle
  - Min threshold slider (%)
  - Max threshold slider (%)
  - Current value display with status color
- L2 marked as "PRIMARY" with distinct border
- Environmental adjustment toggle
- Estimated irrigation duration display
- Auto decision logic summary text

**Quantity Mode UI:**
- Water volume slider (0-5000 liters)
- Cyclic toggle

**Schedule Mode UI:**
- Recurring toggle
- Start time input (HH:MM)
- End time input (HH:MM)
- Duration slider (5-480 minutes)

**Manual Mode UI:**
- Informational text only (use toggle on card)

**Current Conditions Grid:**
- Shows current L1, L2, L3 moisture values with color coding
- Save and Cancel buttons

### ValveSettingsDialog (5 Tabs)

**General Tab:**
- Valve name (editable)
- Motor ID (dropdown selector)
- Crop type (input)
- Primary weather station (dropdown + status indicator)
- Secondary weather station (dropdown, optional)
- Force run toggle
- Safe mode toggle

**Schedule Tab:**
- Schedule configuration form

**Crop Info Tab:**
- Crop summary card (name, variety, age, growth stage, area, pattern)
- Soil & irrigation cards
- Fertigation history
- Add/Edit crop data button (opens CropInfoDialog)

**Notifications Tab:**
- Pest alerts toggle
- Disease alerts toggle
- Low moisture toggle
- Excess water toggle

**Appearance Tab:**
- Icon type selector: Default, Drip, Sprinkler, Flood
- Color picker (6 preset colors)
- Size slider
- Rotation slider

### Add Valve Modal

- Valve name input
- Motor selection dropdown (from existing motors)
- Station selection dropdown (from existing stations)
- Valve created with all default values
