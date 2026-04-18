# Valve Controller Migration Plan — MQTT Spec v1

Local copies of the plan docs, mirrored from
`admn-vend-dshbrd-doc/server-architecture/`.

---

## Read These

1. **[11-compat-audit-firmware-apps.md](./11-compat-audit-firmware-apps.md)** — what fields change
2. **[12-migration-plan.md](./12-migration-plan.md)** — how to change them
   - **§2 Weather / Valve STM8 Firmware** is your primary reference

---

## Valve-Controller–Only Summary (§2 of the plan)

### What the plan needs from this repo

The valve controller (STM8) does **not** speak MQTT directly — it sends `ValvePayload_t` over LoRa to the master gateway, which formats as MQTT JSON. So **most v1 changes happen in the master gateway firmware, not here.**

However, the master needs one new field the LoRa payload must carry:

| New field in MQTT | Source on STM8 | Change needed |
|-------------------|----------------|---------------|
| `bat` (battery mV or %) | On-board ADC reading | **populate `battery_mv` in `ValvePayload_t`** |

### Field rename in the MQTT layer (NOT in STM8 code)

Current MQTT valve payload: `{"isOpen": true, "flow": "2.5"}` (flow as string).

New MQTT valve payload: `{"isOpen": true, "flow": 2.5, "bat": 3700}` (flow as number, bat added).

All three changes happen in **master gateway firmware** `cloud_sync.c` `BuildValveJson()`. The STM8 just needs to populate the raw number in the shared payload struct.

### Estimated scope in this repo

| Task | LoC | Files |
|------|-----|-------|
| Ensure `battery_mv` populated in `ValvePayload_t` when sending LoRa beacon | ~5 | `lora_tx.c` |
| Add field to `agrinet_protocol.h` if missing | ~2 | `agrinet_protocol.h` (shared) |

**Total: ~5-10 LoC.**

### Dependency ordering

1. Add/verify `battery_mv` field in shared `agrinet_protocol.h`
2. Valve controller STM8 firmware: populate field in `lora_tx.c`
3. Master gateway firmware then emits it to MQTT (per §1 of main plan)

Valve firmware update is **independent** of master gateway update — ship in either order. Master will emit `bat:0` until STM8 is flashed.

### Test strategy

1. Compile — STM8S003 flash is tight, watch for regression
2. Bench test: connect LoRa dev kit, capture beacon, decode, verify `battery_mv` is non-zero

---

## What Does NOT Change in This Repo

- I2C slave protocol (covered by valve controller's own spec doc, not affected by MQTT v1)
- Motor drive logic (L293D)
- Limit switch handling
- Flash config layout
- UART command set

**Net change: tiny.** The MQTT v1 spec mostly affects how data is represented on the wire between master and server — not inside the valve node itself.

---

## Sync Notes

To refresh these copies from master:

```bash
SRC=~/Documents/git-repo/sasyamithra-working-code-full/05-documentation/dashboard-docs/server-architecture
cp $SRC/11-compat-audit-firmware-apps.md docs/migration/
cp $SRC/12-migration-plan.md docs/migration/
git add docs/migration/ && git commit -m "Sync migration plan"
```

Master source: https://github.com/aedaaequipments/admn-vend-dshbrd-doc/tree/main/server-architecture
