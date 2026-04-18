# AGRINET Server Stack — Implementation Progress

**Last updated:** 2026-04-18 11:56 IST

Live implementation tracker across Pi4 (MQTT broker) and Pi5 (application server).

---

## Overall Phase Plan

| Phase | Status | Summary |
|-------|--------|---------|
| 0 — Foundation (Pi4 broker, Tailscale, gateway, Samba) | ✅ DONE | |
| 1 — Pi5 SSD migration + Docker stack rewrite | ✅ DONE | Everything on `/mnt/ssd/docker-data/` |
| 2 — MQTT spec v1 frozen | ✅ DONE | in `admn-vend-dshbrd-doc` repo |
| 3 — Pi4 ACL aligned with spec, `backend_api` user | ✅ DONE | flat-file auth (go-auth deferred) |
| 4 — PostgreSQL schema + admin user + farm01 seed | ✅ DONE | 17 tables incl. audit partitions |
| 5a — Grafana dashboards (InfluxDB + Postgres sources) | ✅ DONE | live telemetry chart, farms table |
| 5b — Backup automation (daily pg_dump, weekly Influx) | ✅ DONE | cron scheduled |
| 6 — Device provisioning (CLI + Pi4 apply script) | ✅ DONE | queue-based, Samba mount |
| 8 — Backend API (FastAPI) | 🟡 IN PROGRESS | auth + /me + /admin/devices drafted |
| 7 — Firmware `cloud_sync.c` updates | ⬜ pending | envelope, timestamps, nested topics |
| 9 — Grafana dashboard provisioning (JSON) | ⬜ pending | 6 prebuilt dashboards per spec |
| 10 — Node-RED automation flows | ⬜ pending | heartbeat alerts, soil→valve rules |
| 11 — Security hardening (Nginx TLS, fail2ban) | ⬜ pending | |
| 12 — Monitoring (Uptime Kuma, watchdogs) | ⬜ pending | |
| 13 — End-to-end field test with real STM32 | ⬜ pending | |
| 14 — Mobile/web app integration | ⬜ pending | |

---

## Infrastructure State

### Network

```
Phone ── USB tether ──► Pi4 (192.168.50.1)
                         ├── Mosquitto MQTT :1883
                         ├── DHCP + DNS (dnsmasq)
                         ├── NAT gateway (iptables MASQUERADE)
                         └── eth0 ──► D-Link DSL-2730U (AP mode)
                                       ├── Pi5 (192.168.50.112)
                                       ├── Mac/iPhones
                                       └── (future) STM32 devices
```

Tailscale overlay: Pi4 `100.123.139.67`, Pi5 `100.114.37.75`.

### Pi4 — MQTT broker

- Mosquitto 2.0.21, listener 1883 (TCP) + 9001 (WS)
- Auth: flat-file `/etc/mosquitto/passwd`, ACL at `/etc/mosquitto/acl`
- Users: `backend_api` (full), `agrinet` (legacy full), `farmer_farm01` (scoped), `dev_farm01_MOT-001` (per-device)
- Provisioning queue mounted from Pi5 via CIFS at `/mnt/provisioning-queue`
- Apply script: `/usr/local/sbin/apply-mqtt-users.sh` (run on demand or via cron)
- Log: `/var/log/mosquitto-provisioning.log`

### Pi5 — Application server

Docker Compose stack at `~/agrinet-stack/`:

| Service | Version | Port | State |
|---------|---------|------|-------|
| postgres | 16-alpine | 5432 | ✅ 17 tables, 4 custom roles |
| influxdb | 2.7 | 8086 | ✅ org=sasyamithra, bucket=agrinet |
| telegraf | 1.32 | — | ✅ 6 MQTT consumers, `backend_api` auth |
| grafana | 11.3.0 | 3000 | ✅ 2 data sources (Influx + PG) |
| nodered | latest | 1880 | ✅ idle (flows TBD) |
| backend-api | 0.1 | 8000 | 🟡 drafted, pending build |

Volumes persisted at `/mnt/ssd/docker-data/*`.

### Directory layout on Pi5

```
/mnt/ssd/
├── docker-data/              # container volumes
│   ├── postgres/    (999:999)
│   ├── influxdb/
│   ├── influxdb-config/
│   ├── grafana/    (472:472)
│   ├── nodered/    (1000:1000)
│   ├── telegraf/
│   └── nginx/certs/
├── backups/
│   ├── postgres/             # daily *.dump.gz (30d retention)
│   ├── influxdb/             # weekly backups (12w retention)
│   └── nodered/
├── provisioning-queue/       # Samba-shared to Pi4
│   ├── pending/
│   ├── applied/
│   └── failed/
└── firmware/motor-gw/        # future OTA binaries

/mnt/hdd1tb/                  # 1TB HDD (existing data + rescue workspace)
└── rescued-sdc/photorec/     # PhotoRec carving from corrupt 320GB drive

~/agrinet-stack/
├── .env                      # all secrets (chmod 600, gitignored)
├── docker-compose.yml
├── postgres/init.sql         # full schema bootstrap
├── telegraf/telegraf.conf
├── backend-api/              # FastAPI app (draft)
│   ├── Dockerfile
│   ├── requirements.txt
│   └── app/
│       ├── __init__.py
│       ├── config.py
│       ├── db.py
│       ├── auth.py
│       └── main.py
└── scripts/
    ├── .venv/                # Python venv (bcrypt, psycopg2-binary)
    ├── provision-device.py   # standalone CLI (works w/o backend API)
    ├── backup-daily.sh       # crontab 0 2 * * *
    └── backup-weekly.sh      # crontab 0 3 * * 0
```

---

## Verified End-to-End Flow

1. Run `provision-device.py` on Pi5 → device row in Postgres, MQTT user+ACLs, queue file
2. `apply-mqtt-users.sh` on Pi4 → reads queue via CIFS, adds to `passwd` + `acl`, reloads Mosquitto
3. `mosquitto_pub -u dev_farm01_MOT-001` on Pi4 with the minted password → accepted only for allowed topics
4. Telegraf on Pi5 subscribed to `agrinet/+/+/+/...` → parses envelope, extracts tags, writes to InfluxDB
5. Grafana dashboard refreshes → graph shows published values

Tested with published `pwr=7.7` using `dev_farm01_MOT-001` creds; appeared in InfluxDB query and Grafana panel.

---

## Backup Strategy

| What | Frequency | Script | Retention |
|------|-----------|--------|-----------|
| PostgreSQL `pg_dump -Fc` gzipped | Daily 02:00 | `backup-daily.sh` | 30 days |
| Node-RED `flows.json` copy | Daily 02:00 | `backup-daily.sh` | 30 days |
| InfluxDB `influx backup` | Weekly Sun 03:00 | `backup-weekly.sh` | 12 weeks |
| Stack config tarball | Weekly Sun 03:00 | `backup-weekly.sh` | 12 weeks |

Logged to `/mnt/ssd/backups/backup.log`.

---

## Secrets (`~/agrinet-stack/.env`)

All 14 secrets stored here, `chmod 600`, gitignored.

| Key | Purpose |
|-----|---------|
| POSTGRES_PASSWORD | postgres superuser |
| BACKEND_API_DB_PASSWORD | FastAPI → Postgres |
| CMDLOG_BRIDGE_DB_PASSWORD | future cmdlog-bridge → Postgres |
| MOSQUITTO_GOAUTH_DB_PASSWORD | future go-auth plugin |
| GRAFANA_READER_DB_PASSWORD | Grafana → Postgres read-only |
| INFLUXDB_ADMIN_PASSWORD | Influx admin UI |
| INFLUXDB_ADMIN_TOKEN | full API access |
| INFLUXDB_TELEGRAF_TOKEN | (placeholder — rotate to scoped) |
| INFLUXDB_API_TOKEN | (placeholder — rotate to scoped) |
| MQTT_BACKEND_PASSWORD | Mosquitto `backend_api` |
| MQTT_CMDLOG_PASSWORD | Mosquitto `cmdlog_bridge` |
| GRAFANA_ADMIN_PASSWORD | Grafana admin UI |
| GOTIFY_ADMIN_PASSWORD | future Gotify |
| JWT_SECRET | backend API JWT signing |

---

## Deviations From Spec

| Spec says | Actual | Reason |
|-----------|--------|--------|
| User `admin` at `/home/admin/` | `aedaa-pi5-serv` at `/home/aedaa-pi5-serv/` | kept existing user |
| Pi4 LAN at `192.168.1.5` | Pi4 LAN at `192.168.50.1` | Pi4 doubles as gateway via USB tether |
| go-auth plugin for Mosquitto | flat-file auth | plugin build risky on live broker; pilot scale OK with flat file |
| Mosquitto passwd `root:root 640` | `root:mosquitto 640` | Mosquitto 2.0 warns otherwise |
| `influxdb` org `agrinet` | org `sasyamithra`, bucket `agrinet` | matched spec on rebuild |

---

## Open Issues

- **PhotoRec rescue** on 1TB HDD still running (~17h total, then review carved files)
- **`sdc` (320GB HDD)** btrfs unrecoverable at fs level — photorec is only option
- **Port forwarding** for 4G STM32 → Pi4: not yet configured (office router needs to forward :1883)
- **DuckDNS or similar** for dynamic public IP: pending
- **Firmware** still emits legacy topic/payload format — needs Phase 7 updates

---

## Next Session Entry Points

- **Phase 8 (Backend API):** finish build + login test via curl, add /me and /admin/devices endpoints
- **Phase 7 (Firmware):** update `cloud_sync.c` to emit new envelope + nested topics
- **Phase 10 (Node-RED):** build heartbeat-timeout → Gotify alert flow
- **Pi4 cron:** automate `apply-mqtt-users.sh` every minute so provisioning is fully hands-free
