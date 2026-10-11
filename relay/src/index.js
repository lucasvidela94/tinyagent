// TinyShell relay: receives problem reports and beta telemetry from the Windows client.
// POST /v1/report -> full report saved privately in D1; a public issue with the sanitized
//                    summary is opened in GitHub (GitHub emails the maintainer).
// POST /v1/event  -> telemetry event (metadata only) saved in D1.
// Secrets: GITHUB_TOKEN (issues:write on GITHUB_REPO), IP_SALT (random).

const MAX_REPORT = 256 * 1024;
const MAX_EVENT = 4 * 1024;
const REPORTS_PER_IP_HOUR = 5;
const EVENTS_PER_IP_HOUR = 300;
const PUBLIC_ISSUES_PER_DAY = 30;
const EVENT_NAMES = new Set(["session_start", "session_end", "error"]);

class HttpError extends Error {
  constructor(status, message) {
    super(message);
    this.status = status;
  }
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    try {
      if (request.method === "GET" && url.pathname === "/") return reply({ ok: true, service: "tinyshell-relay" });
      if (request.method !== "POST") throw new HttpError(405, "method not allowed");
      if (url.pathname === "/v1/report") return reply(await report(request, env));
      if (url.pathname === "/v1/event") return reply(await event(request, env));
      throw new HttpError(404, "not found");
    } catch (err) {
      const status = err instanceof HttpError ? err.status : 500;
      if (status === 500) console.error(JSON.stringify({ msg: "unhandled", path: url.pathname, error: String(err) }));
      return reply({ error: status === 500 ? "internal error" : err.message }, status);
    }
  },
};

function reply(body, status = 200) {
  return new Response(JSON.stringify(body), { status, headers: { "content-type": "application/json" } });
}

// Reads at most max bytes, so an oversized or endless body cannot exhaust memory.
async function readJson(request, max) {
  const declared = Number(request.headers.get("content-length") || "0");
  if (declared > max) throw new HttpError(413, "too large");
  if (!request.body) throw new HttpError(400, "empty body");
  const reader = request.body.getReader();
  const chunks = [];
  let size = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    size += value.byteLength;
    if (size > max) {
      await reader.cancel();
      throw new HttpError(413, "too large");
    }
    chunks.push(value);
  }
  const bytes = new Uint8Array(size);
  let offset = 0;
  for (const c of chunks) {
    bytes.set(c, offset);
    offset += c.byteLength;
  }
  try {
    const parsed = JSON.parse(new TextDecoder().decode(bytes));
    if (parsed && typeof parsed === "object" && !Array.isArray(parsed)) return parsed;
  } catch {}
  throw new HttpError(400, "invalid json");
}

function str(value, max) {
  return typeof value === "string" ? value.slice(0, max) : "";
}

async function ipHash(request, env) {
  const ip = request.headers.get("cf-connecting-ip") || "unknown";
  const data = new TextEncoder().encode(`${env.IP_SALT || ""}:${ip}`);
  const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", data));
  return [...digest.slice(0, 12)].map((b) => b.toString(16).padStart(2, "0")).join("");
}

async function countSince(env, table, where, binds, seconds) {
  const since = new Date(Date.now() - seconds * 1000).toISOString();
  const row = await env.DB.prepare(`SELECT COUNT(*) AS n FROM ${table} WHERE ${where} AND created > ?`)
    .bind(...binds, since)
    .first();
  return row ? row.n : 0;
}

// Defense in depth: the client already redacts, but nothing key-shaped reaches GitHub.
function redact(text) {
  return text
    .replace(/sk-[A-Za-z0-9_\-]{8,}/g, "sk-<redactado>")
    .replace(/(Bearer\s+)[A-Za-z0-9._\-]{8,}/gi, "$1<redactado>")
    .replace(/~~~/g, "~ ~ ~");
}

async function report(request, env) {
  const body = await readJson(request, MAX_REPORT);
  const ip = await ipHash(request, env);
  if ((await countSince(env, "reports", "ip_hash = ?", [ip], 3600)) >= REPORTS_PER_IP_HOUR)
    throw new HttpError(429, "too many reports, try again later");
  const now = new Date().toISOString();
  const title = str(body.title, 120).replace(/[\r\n]+/g, " ").trim() || "reporte sin titulo";
  const publicText = redact(str(body.public, 8000));
  const fullText = str(body.full, MAX_REPORT);
  if (!fullText) throw new HttpError(400, "missing full report");
  const insert = await env.DB.prepare(
    "INSERT INTO reports (created, ip_hash, install_id, version, os, title, public_text, full_text) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
  )
    .bind(now, ip, str(body.install_id, 64), str(body.version, 32), str(body.os, 160), title, publicText, fullText)
    .run();
  const id = insert.meta.last_row_id;

  let issueUrl = null;
  if ((await countSince(env, "reports", "issue_url IS NOT NULL", [], 86400)) < PUBLIC_ISSUES_PER_DAY) {
    issueUrl = await openIssue(env, id, title, publicText);
    if (issueUrl) await env.DB.prepare("UPDATE reports SET issue_url = ? WHERE id = ?").bind(issueUrl, id).run();
  }
  console.log(JSON.stringify({ msg: "report", id, version: body.version, issue: issueUrl }));
  return { id, issue_url: issueUrl };
}

async function openIssue(env, id, title, publicText) {
  if (!env.GITHUB_TOKEN || !env.GITHUB_REPO) return null;
  const res = await fetch(`https://api.github.com/repos/${env.GITHUB_REPO}/issues`, {
    method: "POST",
    headers: {
      authorization: `Bearer ${env.GITHUB_TOKEN}`,
      accept: "application/vnd.github+json",
      "x-github-api-version": "2022-11-28",
      "user-agent": "tinyshell-relay",
      "content-type": "application/json",
    },
    body: JSON.stringify({
      title: `[reporte] ${title}`,
      body: `Reporte enviado desde TinyShell (informe completo privado: #${id}).\n\n~~~text\n${publicText}\n~~~\n`,
      labels: ["reporte"],
    }),
  });
  if (!res.ok) {
    console.error(JSON.stringify({ msg: "github issue failed", id, status: res.status, body: (await res.text()).slice(0, 300) }));
    return null;
  }
  return (await res.json()).html_url || null;
}

async function event(request, env) {
  const body = await readJson(request, MAX_EVENT);
  const name = str(body.name, 32);
  if (!EVENT_NAMES.has(name)) throw new HttpError(400, "unknown event");
  const ip = await ipHash(request, env);
  if ((await countSince(env, "events", "ip_hash = ?", [ip], 3600)) >= EVENTS_PER_IP_HOUR)
    throw new HttpError(429, "too many events");
  const data = body.data && typeof body.data === "object" ? JSON.stringify(body.data).slice(0, 2048) : null;
  await env.DB.prepare(
    "INSERT INTO events (created, ip_hash, install_id, version, os, name, data) VALUES (?, ?, ?, ?, ?, ?, ?)",
  )
    .bind(new Date().toISOString(), ip, str(body.install_id, 64), str(body.version, 32), str(body.os, 160), name, data)
    .run();
  return { ok: true };
}
