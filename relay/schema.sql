-- TinyShell relay storage (Cloudflare D1). Private: only the maintainer can query it.
CREATE TABLE IF NOT EXISTS reports (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  created TEXT NOT NULL,
  ip_hash TEXT NOT NULL,
  install_id TEXT,
  version TEXT,
  os TEXT,
  title TEXT,
  public_text TEXT,
  full_text TEXT NOT NULL,
  issue_url TEXT
);
CREATE INDEX IF NOT EXISTS reports_ip_created ON reports (ip_hash, created);
CREATE INDEX IF NOT EXISTS reports_created ON reports (created);

CREATE TABLE IF NOT EXISTS events (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  created TEXT NOT NULL,
  ip_hash TEXT NOT NULL,
  install_id TEXT,
  version TEXT,
  os TEXT,
  name TEXT NOT NULL,
  data TEXT
);
CREATE INDEX IF NOT EXISTS events_ip_created ON events (ip_hash, created);
CREATE INDEX IF NOT EXISTS events_name_created ON events (name, created);
