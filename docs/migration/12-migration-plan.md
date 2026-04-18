# Migration Plan — Firmware v1 + Server + Apps

**Date:** 2026-04-18
**Precursor:** [`11-compat-audit-firmware-apps.md`](./11-compat-audit-firmware-apps.md)

Actionable, step-by-step plan to migrate the running stack to MQTT Spec v1, with no code changes in this doc — just the plan.

---

## 0. Guiding Principles

- **No app-side forklift.** Apps already use camelCase and wildcard MQTT subs; changes are mostly prefix + envelope unwrap.
- **Server is stable.** Telegraf + Pi4 ACL are already v1-ready. Don't touch them.
- **Firmware dual-publish** during transition so mobile/admin can switch at their own pace.
- **Per-user MQTT creds** for mobile app (farmer), separate from device creds.
- **Passwords hashed always.** bcrypt cost 12 for everything.

---

## 1. Firmware Changes (AGRINET-Motor-Gateway)

### 1.1 Scope

All edits in `src/cloud_sync.c` + minor additions to `src/flash_config.h/c`. Nothing in `main.c`, `motor_control.c`, `power_monitor.c`, or any STM8 firmware.

### 1.2 Feature List (ordered by dependency)

1. **Unix time from GSM** (`gsm_driver.c` helper)
   - New function `GSM_GetUnixTime()` that queries `AT+CCLK?`, parses to Unix seconds UTC
   - Cache last-known time + device tick; recompute on each publish to avoid hammering AT
   - Fallback: `ts = 0` if never synced

2. **Envelope builder** (`cloud_sync.c` new static function)
   - `BuildEnvelopeHeader(buf, size, src, parent)` — writes `{"v":1,"ts":...,"src":"...","parent":"...","data":` (no closing brace, data follows)
   - Every existing builder (`BuildMotorJson`, `BuildWeatherJson`, `BuildValveJson`, `BuildAlertJson`) wraps its output
   - Close with `}}` after inner object

3. **Topic builder** (`cloud_sync.c` `BuildTopic()` refactor)
   - Prefix all topics with `agrinet/`
   - New function `BuildChildTopic(kind, farmId, masterId, subType, subId)` → `agrinet/data/{farm}/{master}/{subType}/{subId}`
   - Used for weather and valve (master always = device's own ID)

4. **Payload field updates** (`cloud_sync.c`)
   - Motor telemetry: change numeric fields from `%d.%d` split-format strings to real `%.2f` floats (unquoted in JSON)
   - Motor telemetry: `mode` from `%d` to string via existing enum mapping
   - Weather: rename `airTemperature → airTemp`, `soilMoisture → soilM1`, `soilMoisture2 → soilM2`, `soilMoisture3 → soilM3`, `leafWetness → leafW1`, `leafWetness2 → leafW2`, `lightIntensity → light`
   - Weather: add fields `wind`, `rain`, `bat`, `rssi` (populated from `WeatherPayload_t` — update `agrinet_protocol.h` to carry them from LoRa node)

5. **Retained capability info on CONNECT** (`cloud_sync.c` `OnMqttConnected()` hook)
   - Build JSON: `{"v":1,"ts":...,"src":"MOT-001","data":{"model":"AGN-MGW-3PH","fw":"4.4.0","hw":"v2","caps":["motor","lora_master","power_monitor"],"valves":N,"weather_nodes":M}}`
   - Publish to `agrinet/status/{farm}/{master}/info` with `retain=true, QoS=1`

6. **LWT (Last Will and Testament)** (`gsm_driver.c` extension)
   - Extend `GSM_MqttConnect` to accept LWT topic + payload + QoS
   - LWT topic: `agrinet/status/{farm}/{master}/lastwill`
   - LWT payload: `{"v":1,"ts":0,"src":"MOT-001","data":{"online":false,"reason":"lwt"}}`
   - Registered with `AT+CMQTTWILLTOPIC` + `AT+CMQTTWILLMSG`

7. **Command UUID handling** (`cloud_sync.c` `OnMqttMessage()` parser)
   - Extract `uuid` from incoming command payload (nullable)
   - Include in `cmdlog` publish for server-side ACK matching

8. **Dual-publish flag** (`flash_config.h`)
   - Add `bool dualPublishLegacy` (default: `true`)
   - When `true`, every builder also publishes to the old topic scheme
   - OTA command `SET_DUAL_PUBLISH_LEGACY OFF` flips the bit; device commits to v1 only
   - Lets apps migrate independently

### 1.3 File-Level Diff Summary

| File | Function(s) touched | Est. LoC |
|------|---------------------|----------|
| `cloud_sync.c` | `BuildTopic`, `BuildMotorJson`, `BuildWeatherJson`, `BuildValveJson`, `BuildAlertJson`, `CloudSync_Heartbeat`, `CloudSync_LogCommand`, `OnMqttMessage`, new `BuildEnvelopeHeader`, new `PublishInfoRetained`, `OnMqttConnected` | ~140 |
| `cloud_sync.h` | expose `PublishInfoRetained` | 1 |
| `gsm_driver.c` | `GSM_MqttConnect` signature (LWT), new `GSM_GetUnixTime` | ~40 |
| `gsm_driver.h` | signatures | 5 |
| `flash_config.c/h` | `dualPublishLegacy` field + UART SET command | 10 |
| `agrinet_protocol.h` | `WeatherPayload_t`: add `wind_kmh`, `rain_mm`, `battery_mv`, `rssi_dbm` | 4 |

**Total: ~200 LoC, 6 files.**

### 1.4 Test Strategy

1. **Unit-level:** compile in PlatformIO, check no size regression
2. **Bench:** flash onto lab STM32, point at Pi4 broker, verify Telegraf ingests both old and new topics
3. **Field:** OTA-flash one device first, watch for 48h, then the rest
4. **Rollback:** `SET_DUAL_PUBLISH_LEGACY ON` via UART restores old behavior

---

## 2. Weather / Valve STM8 Firmware

**No changes needed.** LoRa nodes already send the payload struct to the master. Master reformats into MQTT JSON. The field **renames** happen in master firmware only.

If battery/RSSI fields aren't already in LoRa payload, weather-station STM8 firmware gets a small addition:

| File | Change |
|------|--------|
| `weather-station/src/lora_tx.c` | Populate `WeatherPayload_t.battery_mv` from ADC (existing) |
| `weather-station/src/lora_tx.c` | Populate `rssi_dbm` from last RX RSSI (available from SX127x driver) |

Estimated ~15 LoC. Independent change, can happen after master firmware migration.

---

## 3. Server-Side Changes (sasyamithra-server Node.js + Pi5 stack)

### 3.1 Node.js Backend (`sasyamithra-server`)

This is the **existing production backend** with SQLite + Fastify + MQTT handler. It sits alongside the new FastAPI Pi5 backend.

| File | Change | Reason |
|------|--------|--------|
| `mqtt-handler.js` | Subscribe to `agrinet/#` (in addition to `data/#`) | Accept both v0 and v1 during dual-publish |
| `mqtt-handler.js` | Topic parser: handle nested `agrinet/data/{farm}/{master}/weather/{station}` | Attribute LoRa child messages to parent master |
| `mqtt-handler.js` | Unwrap envelope: `if payload.v === 1, payload = payload.data` | Compatibility layer |
| `adapters/*.js` | Field renames: `airTemperature → airTemp`, etc. | Match new schema |
| `db/schema.sql` | **No changes** — SQLite columns can stay snake_case | SQL convention |
| `routes/vendor.js` `/motors/{id}/toggle` | When publishing MQTT command, include a generated `uuid` | ACK tracking |
| `routes/admin.js` | Expose `device_info` table/endpoint for retained capability messages | Device registry |

**Estimated:** ~50 LoC across 3 files. One afternoon.

### 3.2 Pi5 FastAPI Backend (`sasyamithra-server/pi5-backend/backend-api`)

**No schema changes** — Postgres tables already match spec v1 (see `05-postgresql-schema.md`).

New endpoints to add:

| Method | Path | Purpose |
|--------|------|---------|
| `GET /api/farms` | list farms user can access (own + shared) | Mobile home screen |
| `GET /api/farms/{id}/devices` | devices in farm | Motor list per farm |
| `GET /api/devices/{id}/live` | latest telemetry from InfluxDB | Real-time panel |
| `GET /api/devices/{id}/history` | historical telemetry from InfluxDB (with range param) | Charts |
| `POST /api/devices/{id}/command` | publish MQTT command, insert row in `commands` table | Control |
| `POST /api/admin/users` | create user with bcrypt password | Admin |
| `POST /api/admin/farms` | create farm | Admin |
| `POST /api/auth/refresh` | exchange refresh token for new access token | Session continuity |

Each new endpoint:
- Follows existing `current_user` / `require_admin` dependency pattern
- Audit-logs to `audit_log` table
- Validates inputs via Pydantic

**Estimated:** ~300 LoC, new file `app/routes/devices.py`, `app/routes/farms.py`, etc.

### 3.3 Telegraf + InfluxDB

**No change.** Already configured for v1 topic schema with envelope JSON unwrapping via `json_time_key: ts`.

One future optimization when traffic grows:
- Split single `agrinet` bucket into per-measurement buckets with different retention (per `06-influxdb-schema.md`)
- Add downsampling tasks: raw → 1-minute avg (1 year) → 1-hour avg (5 years)

Not urgent for pilot.

### 3.4 Mosquitto ACL

**Schema-ready.** Per-device users already get scoped to `agrinet/data/{farm}/{device}/#` etc. No change.

---

## 4. App-Side Changes

### 4.1 Admin Dashboard (`agrinet-admin` React)

**Topic subscriptions** (`src/hooks/useMqttFeed.ts`):
- `data/#` → `agrinet/data/#`
- `status/#` → `agrinet/status/#`
- `alert/#` → `agrinet/alert/#`

**MQTT message handler** (same file):
- Unwrap envelope: `const payload = msg.v === 1 ? msg.data : msg`
- Topic parser: extract `farmId` and `masterId` from `agrinet/data/{farmId}/{masterId}/...`

**API client** (`src/api/client.ts`):
- Base URL stays `/api`, routed via Nginx (future) or Vite proxy (dev)
- Add refresh-token interceptor calling `/api/auth/refresh` on 401 (currently just logs out)

**Components:**
- Remove defensive `deviceId || device_id` fallbacks (~15 occurrences)
- Remove defensive `lastSeen || last_seen` fallbacks
- Field renames in WeatherStation detail page: `airTemp` now direct from payload, not renamed during fetch

**Estimated:** ~30 LoC, 5-10 files.

### 4.2 Mobile App (`sasyamithra-android` Capacitor)

**Topic subscriptions** (`src/contexts/LiveDataContext.tsx`):
- `data/+/+/telemetry` → `agrinet/data/+/+/telemetry`
- `data/+/+/weather` → `agrinet/data/+/+/weather/+` (note added `/+` for station)
- `data/+/+/power` → (if still used, prefix)
- `status/+/+/ack` → `agrinet/status/+/+/heartbeat` (rename semantics)

**Payload handlers** (same contexts):
- Unwrap envelope
- Drop `snake_case || camelCase` normalizer (payloads now consistent camelCase)
- Drop `parseFloat` wrappers around numeric fields (now pre-parsed numbers)

**Auth flow** (`src/lib/api.ts`):
- Already handles access+refresh tokens ✅
- No change to login/register, but login response now returns `{accessToken, refreshToken, mqttUsername, mqttPassword, user}` — the mobile MQTT credentials are per-user (`farmer_{farmId}` MQTT user). Backend mints these at registration or on first login.

**Config** (`src/lib/config.ts`):
- `API_BASE_URL` and `MQTT_WS_URL` already environment-driven ✅
- Update `.env.development` to point at Pi5 for local testing

**Estimated:** ~40 LoC, 3-5 files.

### 4.3 iOS App (`sasyamithra-ios`)

Assuming iOS mirrors Android (Capacitor web-src is typically shared), changes mirror §4.2. If iOS has separate `src/` directory, apply same edits there.

**Estimated:** ~40 LoC if separate codebase.

---

## 5. User & Password Strategy (recap, no change needed)

| User type | Storage | Password hash | MQTT role |
|-----------|---------|---------------|-----------|
| **Admin** (`venkatesh`) | `users` table | bcrypt cost 12 | no MQTT credentials (admin uses backend API, not MQTT directly) |
| **Farmer** | `users` table | bcrypt cost 12 | separate `mqtt_users.farmer_{farmId}` record, ACL scoped to their farm |
| **Field device** (STM32) | `mqtt_users` table | bcrypt cost 12 | scoped to `agrinet/{kind}/{farm}/{device}/#` |
| **Backend services** | `.env` vars (Postgres role + MQTT user) | plain in .env, bcrypt in mqtt_users | `backend_api` superuser in Mosquitto |

Backend API mints farmer MQTT credentials **on user registration**:

1. User POSTs `/api/auth/register` with email + desired password
2. Backend bcrypts user password → `users.password_hash`
3. Backend generates random MQTT password (`secrets.token_urlsafe(24)`)
4. Backend bcrypts MQTT password → `mqtt_users.password_hash`
5. Registration response includes `{accessToken, refreshToken, mqttUsername, mqttPassword}` (plaintext MQTT pw returned ONCE, stored on device via Capacitor Preferences encrypted storage)
6. Queue file written for Pi4 to apply new farmer MQTT user (same pipeline as device provisioning)

**Password rotation:** `POST /api/users/me/rotate-mqtt` regenerates — old pw invalidated, new pw returned once.

---

## 6. Data Mapping: Firmware → SQL → API → UI

**Example: motor voltage (R phase)**

| Layer | Representation |
|-------|----------------|
| Firmware float | `powerMonitor.r.voltage = 230.5f` |
| MQTT payload | `{"v":1,...,"data":{"phV":{"r":230.5,...}}}` |
| Telegraf → InfluxDB | `motor_telemetry` measurement, field `phV_r` (Telegraf flattens nested), `_value = 230.5` |
| Pi5 backend `GET /api/devices/MOT-001/live` | `{"phV":{"r":230.5,...}}` (reconstituted from Influx, dot-to-object) |
| React admin UI | `motor.phV.r` rendered in `ControlCenterPage` |
| React Native mobile | `motor.phV.r` rendered in `MotorPhaseCard` |

**Invariant:** `phV.r` is the single name across all five layers. Telegraf flattening (`phV_r`) is the only exception, reversed by the read API.

### 6.1 Postgres ↔ API camelCase bridge

Postgres columns stay snake_case. FastAPI converts to camelCase via Pydantic model alias:

```python
# Example (planned, not yet implemented)
class FarmOut(BaseModel):
    model_config = ConfigDict(populate_by_name=True, alias_generator=to_camel)
    id: int
    farm_id: str       # serializes to "farmId"
    name: str
    owner_id: int      # serializes to "ownerId"
    area_acres: Decimal  # serializes to "areaAcres"
    created_at: datetime  # serializes to "createdAt"
```

Same pattern for `DeviceOut`, `NodeOut`, `AlertOut`, `UserOut`, etc.

---

## 7. Sequencing & Milestones

| Week | Milestone | Who |
|------|-----------|-----|
| 1 | Firmware edits (`cloud_sync.c`) | Firmware |
| 1 | Bench test dual-publish on lab STM32 | Firmware + test rig |
| 2 | Deploy firmware v4.5.0 to 1 field device, observe 48h | Firmware + field |
| 2 | Pi5 backend: add `/api/farms`, `/api/devices`, `/api/auth/refresh` | Server |
| 2 | Admin dashboard: topic prefix + envelope unwrap | Frontend |
| 3 | Mobile app: topic prefix + envelope unwrap + rename drops | Frontend |
| 3 | `sasyamithra-server` Node.js MQTT handler: dual topic subs + envelope unwrap | Backend |
| 3 | OTA-flash remaining field devices | Firmware |
| 4 | Flip `dualPublishLegacy = false` across fleet | Firmware |
| 4 | Remove v0 topic subs from all apps | Frontend |
| 5 | Full v1 mode, legacy code paths deleted | All |

---

## 8. Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Firmware upgrade bricks device | Field revisit needed | OTA with known-good rollback; `SET_DUAL_PUBLISH_LEGACY ON` + reboot restores |
| App ships before firmware ready | App sees no data | Dual-publish window ensures overlap; app defaults to v1 but falls back to v0 parse if `.v` field missing |
| Field devices can't reach OTA server | Stuck on v4.4 | Keep `dualPublishLegacy=true` as default until all devices confirmed updated |
| GSM time (`AT+CCLK`) unreliable | `ts=0` on some devices | Server stamps `ts_server` on ingest as backup; InfluxDB indexes both |
| Postgres schema migration needed mid-flight | DB downtime | All new endpoints are additive; no column rename/drop planned |
| Farmer MQTT creds leaked via logs | Unauthorized access | Refuse to log `mqttPassword`; encrypted storage on mobile; rotate via API |

---

## 9. Documentation Updates Triggered

When this plan completes:

- `02-mqtt-spec-v1.md` — confirm matches emitted format; add any field rename footnotes
- `05-postgresql-schema.md` — no change (schema was built for v1)
- `10-functional-flow.md` — update Flow B with real v1 topic names once firmware ships
- `99-implementation-status.md` — bump Phase 7 to ✅ and note field rename changes
- This doc (`12-migration-plan.md`) — retained for history; archive link in each phase section when complete

---

## 10. Out-of-Scope (Explicitly Deferred)

- Mosquitto go-auth plugin migration (flat file scales to pilot)
- InfluxDB bucket split + downsampling (single bucket fine at pilot scale)
- cmdlog-bridge Python worker (commands table exists but ACK tracking not critical for MVP)
- OTA firmware binary hosting on Pi5 (manual SWIM flashing OK until fleet > 10)
- Nginx TLS / Let's Encrypt (LAN-only pilot OK on plain HTTP)
- iOS app fork (if it diverges from Android codebase, treat as separate Phase 7.x)

---

## 11. What This Plan Does NOT Require Changing

- Pi5 Postgres schema
- Pi5 Telegraf config
- Pi5 InfluxDB setup
- Pi4 Mosquitto base config or ACL template
- `pi5-backend/docker-compose.yml` structure
- Backup scripts
- Node-RED heartbeat-monitor flow (topic-agnostic via wildcard)

The server side of the house is ready. This plan exclusively moves firmware + apps forward.
