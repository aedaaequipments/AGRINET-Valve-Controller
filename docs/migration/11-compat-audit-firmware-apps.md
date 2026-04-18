# Compatibility Audit — Firmware ↔ Apps ↔ MQTT Spec v1

**Date:** 2026-04-18
**Status:** Reference document, informs Phase 7 migration plan (see `12-migration-plan.md`).

Audit of the three-way alignment between current motor gateway firmware emit, current admin dashboard + mobile app consumption, and the target MQTT Spec v1 (`02-mqtt-spec-v1.md`).

---

## 1. Topic Hierarchy Gap

| Area | Firmware emits | Admin UI subscribes | Mobile subscribes | Spec v1 target |
|------|----------------|---------------------|--------------------|-----------------|
| Motor telemetry | `data/{farm}/{dev}/telemetry` | `data/#` | `data/+/+/telemetry` | `agrinet/data/{farm}/{master}/telemetry` |
| Weather | `data/{farm}/{station}/weather` **(flat)** | `data/#` | `data/+/+/weather` | `agrinet/data/{farm}/{master}/weather/{station}` **(nested)** |
| Valve | `data/{farm}/{valve}/telemetry` **(flat)** | — | — | `agrinet/data/{farm}/{master}/valve/{valve}` **(nested)** |
| Heartbeat | `status/{farm}/{dev}/heartbeat` | `status/#` | `status/+/+/ack` ⚠ | `agrinet/status/{farm}/{master}/heartbeat` |
| Alert | `alert/{farm}/{dev}/warn` | `alert/#` | `alert/+/+/warn` | `agrinet/alert/{farm}/{master}/{sev}` |
| Command (downlink) | subs `cmd/+/{dev}/set` | — | — | subs `agrinet/cmd/{farm}/{master}/set` |
| Cmdlog | `data/{farm}/{dev}/cmdlog` | — | — | `agrinet/data/{farm}/{master}/cmdlog` |

**Impact:** moving to `agrinet/` prefix is a **single-line fix** on the apps because both already use wildcard subscriptions. The parent/child nesting for weather and valve is the bigger semantic change.

---

## 2. Envelope Gap

All current payloads are raw JSON. Spec v1 wraps in an envelope:

```json
{
  "v": 1,
  "ts": 1745030000,
  "src": "MOT-001",
  "parent": "MOT-001",
  "data": { /* current payload fields */ }
}
```

App impact: parsers must read `.data.pwr` instead of `.pwr`. One small change per consumer.

---

## 3. Field-Level Compatibility (Motor Telemetry)

| Concept | Firmware emits | Admin reads | Mobile reads | Spec v1 pick |
|---------|---------------|-------------|--------------|--------------|
| Run state | `"run": true` (bool) | `run` ✅ | `run` ✅ | keep |
| Phase voltage | `"phV":{"r":230,...}` (int) | `phV.r/y/b` ✅ | `phV.r/y/b` ✅ | keep |
| Phase current | `"phI":{"r":"4.2"}` **(string)** | expects number | expects number | **emit as number** |
| Power factor | `"pf":{"o":"0.85"}` **(string)** | expects number | expects number | **emit as number** |
| Power (kW) | `"pwr":"2.80"` **(string)** | expects number | expects number | **emit as number** |
| Energy (kWh) | `"kwh":"125.3"` **(string)** | expects number | expects number | **emit as number** |
| Hours | `"hrs":"45.2"` **(string)** | expects number | expects number | **emit as number** |
| Health | `"hp":"good"` (string) | `hp` ✅ | `hp` ✅ | keep |
| Mode | `"mode":1` (int) | — | `"auto"\|"manual"` ⚠ | **emit as string** "MANUAL/AUTO/SCHEDULE" |
| Force run | `"forceRun":false` | — | ✅ | keep |
| Safe mode | `"safeMode":false` | — | ✅ | keep |
| Is running | `"isRunning":true` | — | — | keep (duplicate of `run`) |

---

## 4. Field-Level Compatibility (Weather)

| Concept | Firmware emits | Admin reads | Mobile reads | Spec v1 pick |
|---------|---------------|-------------|--------------|--------------|
| Air temp | `airTemperature` | `airTemp` ⚠ | `airT` ⚠ | **rename to `airTemp`** |
| Soil temp | `soilTemperature` | — | `soilT` ⚠ | **rename to `soilTemp`** |
| Humidity | `humidity` | `humidity` ✅ | `humid` ⚠ | **keep `humidity`** |
| Soil moisture L1 | `soilMoisture` | `soilM1` ⚠ | `soilM1` ⚠ | **rename to `soilM1`** |
| Soil moisture L2 | `soilMoisture2` | `soilM2` ⚠ | `soilM2` ⚠ | **rename to `soilM2`** |
| Soil moisture L3 | `soilMoisture3` | `soilM3` ⚠ | `soilM3` ⚠ | **rename to `soilM3`** |
| Leaf wetness 1 | `leafWetness` | `leafWet1` ⚠ | `leafW` ⚠ | **rename to `leafW1`** |
| Leaf wetness 2 | `leafWetness2` | — | `leafW2` ⚠ | **rename to `leafW2`** |
| Light | `lightIntensity` | `light` ⚠ | `light` ⚠ | **rename to `light`** |
| Wind | — (missing) | `wind` | `wind` | **add** |
| Rain | — (missing) | `rain` | `rain` | **add** |
| Battery | — (missing) | `battery` | `batt` ⚠ | **add as `bat`** |
| RSSI | — (missing) | `signal` | — | **add** |

---

## 5. Field-Level Compatibility (Valve)

| Concept | Firmware emits | Admin reads | Mobile reads | Spec v1 pick |
|---------|---------------|-------------|--------------|--------------|
| Is open | `"isOpen":true` (bool) | `isOpen` ✅ | `isOpen` ✅ | keep |
| Flow rate | `"flow":"2.5"` **(string)** | `flow` as number | `flow` as number | **emit as number** |

---

## 6. Incoming Command Compatibility

Firmware callback parses: `run`, `forceRun`, `safeMode`, `starDelta`, `overV`, `underV`, `overload`, `dryRun`, `restart`, `mode`.

Current app→server: REST (`POST /motors/{id}/toggle` with `{run: bool}`) — backend MQTT-publishes.

**Spec v1 addition:** every command payload gets a `uuid` field for ACK matching via `cmdlog` topic.

---

## 7. Variable Naming Inconsistency

| Concept | Current state | Spec v1 |
|---------|---------------|---------|
| Device identifier | `deviceId` (mostly) + `device_id` defensive | **`deviceId`** in MQTT/API, `device_id` in SQL |
| Farm identifier | `farmId` (mostly) + `farm_id` SQL | **`farmId`** in MQTT/API |
| Timestamp | `lastSeen` + `last_seen` inconsistent | **`lastSeenAt`** ISO 8601 UTC |
| Area | `area_acres` (admin) + `acreage` (mobile) | **`areaAcres`** |
| Crop | `crop_type` (admin) + `cropType` (mobile) | **`cropType`** |
| Severity | `sev` short (admin + mobile) | keep `sev` |
| Acknowledged | `ack` (everyone) | keep |

SQL columns stay snake_case (standard); backend API response layer converts to camelCase.

---

## 8. Firmware Fields Missing That Apps Want

| Field | Admin component | Mobile interface | Recommended source |
|-------|-----------------|-------------------|---------------------|
| `bat` (battery %) | ClusterDetail | WeatherData.batt | LoRa child node → forwarded by master |
| `rssi` | ClusterDetail.signal | — | LoRa link RSSI |
| `wind` | ClusterDetail | WeatherData.wind | Weather station sensor |
| `rain` | ClusterDetail | WeatherData.rain | Weather station sensor |
| `signal` (CSQ) | OverviewPage `signal` | — | STM32 heartbeat (already present) |

---

## 9. Incremental Migration Path (recommended)

1. **Firmware dual-publish** — emit old + new topics in parallel for ~1 week
2. **Apps switch** — update topic subscriptions + parsers
3. **Firmware drops old emission** — via OTA config flag
4. **Done** — server only sees v1

No downtime, no device re-flash needed mid-cycle. Each step is reversible.

---

## 10. Size Estimate

| Component | LoC change | Files |
|-----------|------------|-------|
| Firmware `cloud_sync.c` | ~140 | 1 file |
| Admin dashboard | ~30 | 5-10 components |
| Mobile app | ~40 | 3-5 contexts |
| Backend (sasyamithra-server Node.js) | ~20 | `mqtt-handler.js` |
| Telegraf config | already v1-ready | — |

Total: ~230 LoC across the whole stack. Feasible in 1-2 focused sessions per sub-project.
