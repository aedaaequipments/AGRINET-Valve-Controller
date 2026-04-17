# Pi5 Server Stack — Live Implementation Progress

Keep this open in a split pane while running commands on Pi5.

---

## Overall Phase Plan

- [x] **Phase 0** — Foundation (Pi4 MQTT, Tailscale, Pi4 gateway, Samba)
- [x] **Phase 2** — MQTT spec frozen (see `05-documentation/dashboard-docs/server-architecture/02-mqtt-spec-v1.md`)
- [x] **Phase 1** — Pi5 SSD migration + compose rewrite ✅ DONE
- [x] **Phase 4** — PostgreSQL schema + admin user + farm01 seeded ✅ DONE
- [ ] **Phase 3** — Pi4 broker hardening (ACL per spec) ← **NEXT**
- [ ] **Phase 5** — InfluxDB buckets + Telegraf refinement
- [ ] **Phase 6** — Device provisioning CLI
- [ ] **Phase 7** — Firmware cloud_sync.c updates
- [ ] **Phase 8** — Backend API (FastAPI)
- [ ] **Phase 9** — Grafana dashboards provisioning
- [ ] **Phase 10** — Node-RED automation flows
- [ ] **Phase 11** — Security hardening (Nginx TLS, fail2ban)
- [ ] **Phase 12** — Monitoring (Uptime Kuma, watchdogs)
- [ ] **Phase 13** — End-to-end field test
- [ ] **Phase 14** — Mobile/web app integration

---

## Phase 1 — SSD Migration ✅ DONE

All steps 1.1 through 1.9 completed. Stack healthy on SSD.

## Phase 4 — PostgreSQL ✅ DONE

- `postgres/init.sql` written (248 lines, 13 tables per spec)
- postgres:16-alpine service added to docker-compose.yml
- Schema loaded on first start (17 relations including audit_log partitions)
- 4 custom roles passwords set from `.env`:
  - backend_api, cmdlog_bridge, mosquitto_auth, grafana_reader
- Admin user seeded: `venkatesh` / `venkatesh@aedaa.in` / role=admin
- Farm seeded: `farm01` / AEDAA Test Farm / owner=venkatesh

---

## Current Directory State on Pi5

```
/mnt/ssd/
├── docker-data/               # all container volumes (correct UIDs set)
│   ├── postgres/    (999:999)
│   ├── influxdb/
│   ├── influxdb-config/
│   ├── grafana/     (472:472)
│   ├── nodered/     (1000:1000)
│   ├── telegraf/
│   ├── cmdlog-bridge/
│   ├── backend-api/
│   ├── gotify/
│   └── nginx/certs/
├── backups/
│   ├── postgres/
│   ├── influxdb/
│   └── nodered/
└── firmware/motor-gw/

/mnt/hdd1tb/
└── rescued-sdc/photorec/      # PhotoRec carving running (~17h total)

~/agrinet-stack/                # project root
├── docker-compose.yml          # to be rewritten per spec
├── telegraf/telegraf.conf      # to be rewritten per spec
├── nginx/nginx.conf            # Phase 11
├── postgres/init.sql           # Phase 4
└── scripts/                    # backup scripts, provisioning CLI
```

---

## Next Session — Phase 3 OR Phase 5

**Phase 3 (Pi4 ACL hardening):** switch Pi4 Mosquitto from flat-file to go-auth plugin
reading from the PostgreSQL `mqtt_users` / `mqtt_acls` tables. This lets us provision
devices via DB without editing `/etc/mosquitto/passwd`.

**Phase 5 (Provisioning CLI):** build `scripts/provision-device.py` that:
- registers a new device in `devices` table
- creates matching `mqtt_users` + `mqtt_acls` rows (bcrypt password)
- prints UART provisioning config for field technician

Recommended order: Phase 3 first (so provisioning CLI can write directly to DB instead
of also needing to touch Mosquitto files).

---

## Spec References (read-only, do not edit)

Located at `~/Documents/git-repo/sasyamithra-working-code-full/05-documentation/dashboard-docs/server-architecture/`

| # | File | What's in it |
|---|------|--------------|
| 01 | `01-revised-plan-v3.md`     | Master plan |
| 02 | `02-mqtt-spec-v1.md`        | Topic schema, payload envelope, regex |
| 03 | `03-pi4-mqtt-broker.md`     | Mosquitto ACL + go-auth plugin |
| 04 | `04-pi5-server-stack.md`    | Docker compose, Telegraf, Nginx, backups |
| 05 | `05-postgresql-schema.md`   | 11 tables, roles, retention |
| 06 | `06-influxdb-schema.md`     | Buckets, retention, downsampling |
| 07 | `07-device-provisioning.md` | Register farms/devices |
| 08 | `08-cmdlog-bridge.md`       | Command ACK pipeline |

---

## Deviations from Spec (documented)

| Spec says | Actual | Why |
|---|---|---|
| User `admin` at `/home/admin/` | User `aedaa-pi5-serv` at `/home/aedaa-pi5-serv/` | User chose to keep existing user |
| Pi4 LAN at 192.168.1.5 | Pi4 LAN at 192.168.50.1 (gateway mode) | Pi4 is also internet gateway via phone USB tether; LAN is 192.168.50.x |
| Org `sasyamithra` / bucket `agrinet` | Will match spec on rebuild | Old data was `agrinet`/`farm_data`, wiping clean |
| InfluxDB data existed | Wiped for clean start | Only test data, clean slate preferred |

---

## Open Issues to Resolve Later

- `sdc` (320GB HDD) btrfs is corrupt — PhotoRec rescue running on hdd1tb
- Port forwarding needed on field router for STM32 to reach Pi4 over 4G
- DuckDNS or similar for dynamic IP (if ISP gives dynamic)
- Mosquitto password used `root:mosquitto` ownership instead of spec's root:root (permission compromise for current flat-file auth; go-auth plugin later fixes this)
