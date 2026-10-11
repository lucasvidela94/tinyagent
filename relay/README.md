# TinyShell relay

Cloudflare Worker that receives what the Windows client sends:

- `POST /v1/report`: a problem report from `tinyshell reporte` / `/reporte`. The full report (system, recent errors, optionally the conversation) is stored **privately** in D1; a public issue with the redacted summary and the private ID is opened in `lucasvidela94/tinyagent` with the `reporte` label (GitHub emails the maintainer). Limits: 256 KB, 5 reports per IP per hour, 30 public issues per day (beyond that, reports are still stored).
- `POST /v1/event`: beta telemetry (`session_start`, `session_end`, `error`), metadata only, 4 KB, 300 per IP per hour.

IPs are stored only as a salted SHA-256 prefix. Deployed at `https://tinyshell-relay.lucasan-videla.workers.dev` (`RELAY_URL` in `src/config.h`).

## Deploy

```bash
cd relay
hush run --only CLOUDFLARE_API_TOKEN -- npx wrangler deploy
# one-time, already done:
#   npx wrangler d1 create tinyshell-relay && npx wrangler d1 execute tinyshell-relay --remote --file schema.sql
#   openssl rand -hex 32 | npx wrangler secret put IP_SALT
#   hush stdin TINYSHELL_GITHUB_TOKEN -- npx wrangler secret put GITHUB_TOKEN   (fine-grained: Issues read/write on tinyagent)
```

## Reading reports and telemetry

```bash
q() { hush run --only CLOUDFLARE_API_TOKEN -- npx wrangler d1 execute tinyshell-relay --remote --command "$1"; }
q "SELECT id, created, version, os, title, issue_url FROM reports ORDER BY id DESC LIMIT 20"
q "SELECT full_text FROM reports WHERE id = 1"
q "SELECT json_extract(data,'$.code') AS code, version, COUNT(*) AS n FROM events WHERE name='error' GROUP BY 1,2 ORDER BY n DESC"
q "SELECT version, COUNT(DISTINCT install_id) AS installs FROM events WHERE name='session_start' GROUP BY 1"
```
