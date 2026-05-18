// =============================================================================
// hal/wifi_share.cpp  —  WiFi AP + ESPAsyncWebServer photo gallery
//
// Routes:
//   GET  /           → self-contained HTML gallery page (PROGMEM)
//   GET  /manifest   → JSON array of {id, name} for all BMPs on SD
//   GET  /thumb/{n}  → 4× downsampled BMP, generated on-the-fly in PSRAM
//   GET  /photo/{n}  → full BMP streamed from SD in 4 KB chunks
//   DELETE /photo/{n}→ delete from SD + remove from in-memory index
// =============================================================================
#include "wifi_share.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SD_MMC.h>
#include <vector>

namespace {

// ── In-memory photo index ────────────────────────────────────────────────────
// Populated by scan_sd() on wifi_share_start().  Index into this vector is
// the {n} in all URL routes.
std::vector<String> s_photos;

// Server instance lives in regular heap (small object).
AsyncWebServer* s_server = nullptr;

// ── Embedded HTML gallery page ───────────────────────────────────────────────
// Stored in flash via PROGMEM.  Single-page app; zero external dependencies.
static const char GALLERY_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Maryam's Gallery</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:#0d0d0d;color:#f0f0f0;font-family:system-ui,sans-serif;min-height:100vh}
  #topbar{position:sticky;top:0;z-index:100;background:rgba(13,13,13,.95);
    backdrop-filter:blur(10px);border-bottom:1px solid #2a2a2a;
    padding:10px 14px;display:flex;flex-wrap:wrap;gap:8px;align-items:center}
  #topbar h1{font-size:1.1rem;color:#e91e8c;flex:1;white-space:nowrap}
  .action-btn{background:#1e1e1e;border:1px solid #333;color:#f0f0f0;
    padding:7px 13px;border-radius:8px;font-size:.82rem;cursor:pointer;
    transition:background .15s,border-color .15s}
  .action-btn:hover{background:#2a2a2a;border-color:#e91e8c}
  .action-btn.danger{border-color:#c0392b;color:#e74c3c}
  .action-btn.danger:hover{background:#2a1010;border-color:#e74c3c}
  #status{font-size:.8rem;color:#888;padding:6px 14px 0}
  #grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));
    gap:10px;padding:12px}
  .card{background:#1a1a1a;border-radius:10px;overflow:hidden;
    border:2px solid transparent;transition:border-color .15s;position:relative}
  .card.selected{border-color:#e91e8c}
  .thumb-wrap{position:relative;cursor:pointer;aspect-ratio:3/4;overflow:hidden;
    background:#111}
  .thumb-wrap img{width:100%;height:100%;object-fit:cover;display:block}
  .check-overlay{position:absolute;top:6px;left:6px;width:22px;height:22px;
    border-radius:50%;background:rgba(0,0,0,.6);border:2px solid #aaa;
    display:flex;align-items:center;justify-content:center;font-size:.85rem;
    transition:background .15s,border-color .15s}
  .card.selected .check-overlay{background:#e91e8c;border-color:#e91e8c}
  .card-actions{display:flex;gap:6px;padding:7px 8px}
  .card-actions a,.card-actions button{flex:1;text-align:center;padding:5px 0;
    border-radius:7px;font-size:.8rem;cursor:pointer;text-decoration:none;
    border:1px solid #333;background:#1e1e1e;color:#f0f0f0;
    transition:background .15s}
  .card-actions a:hover{background:#1a3a1a;border-color:#27ae60;color:#2ecc71}
  .card-actions button:hover{background:#3a1010;border-color:#c0392b;color:#e74c3c}
  /* Confirmation modal */
  #modal{display:none;position:fixed;inset:0;background:rgba(0,0,0,.75);
    z-index:999;align-items:center;justify-content:center}
  #modal-box{background:#1a1a1a;border:1px solid #333;border-radius:14px;
    padding:24px 20px;max-width:300px;width:90%;text-align:center}
  #modal-box p{margin-bottom:18px;line-height:1.5;color:#ccc}
  #modal-box .mb{display:flex;gap:10px;justify-content:center}
  #modal-box button{padding:10px 22px;border-radius:8px;font-size:.9rem;
    cursor:pointer;border:1px solid #333;background:#2a2a2a;color:#f0f0f0}
  #modal-box .confirm-del{background:#c0392b;border-color:#c0392b;color:#fff}
  #empty{text-align:center;padding:60px 20px;color:#555;font-size:.95rem}
</style>
</head>
<body>
<div id="topbar">
  <h1>📸 Maryam's Gallery</h1>
  <button class="action-btn" onclick="toggleSelectAll()">Select All</button>
  <button class="action-btn" onclick="downloadSelected()">⬇ Download</button>
  <button class="action-btn danger" onclick="deleteSelected()">🗑 Delete Selected</button>
  <button class="action-btn danger" onclick="promptDeleteAll()">🗑 Delete All</button>
</div>
<div id="status"></div>
<div id="grid"></div>

<div id="modal">
  <div id="modal-box">
    <p>Delete <strong>all photos</strong> from the SD card?<br>This cannot be undone.</p>
    <div class="mb">
      <button onclick="closeModal()">Cancel</button>
      <button class="confirm-del" onclick="confirmDeleteAll()">Delete All</button>
    </div>
  </div>
</div>

<script>
let photos = [];
let selected = new Set();

async function init() {
  setStatus('Loading...');
  try {
    const r = await fetch('/manifest');
    photos = await r.json();
    renderGrid();
    setStatus(photos.length + ' photo' + (photos.length !== 1 ? 's' : ''));
  } catch(e) { setStatus('Error loading gallery'); }
}

function setStatus(msg) { document.getElementById('status').textContent = msg; }

function renderGrid() {
  const grid = document.getElementById('grid');
  if (photos.length === 0) {
    grid.innerHTML = '<div id="empty">No photos on SD card</div>';
    return;
  }
  grid.innerHTML = '';
  photos.forEach(p => {
    const card = document.createElement('div');
    card.className = 'card' + (selected.has(p.id) ? ' selected' : '');
    card.id = 'card-' + p.id;
    card.innerHTML =
      '<div class="thumb-wrap" onclick="toggleSelect(' + p.id + ')">' +
        '<img src="/thumb/' + p.id + '" loading="lazy" alt="' + p.name + '">' +
        '<div class="check-overlay">' + (selected.has(p.id) ? '✓' : '') + '</div>' +
      '</div>' +
      '<div class="card-actions">' +
        '<a href="/photo/' + p.id + '" download="' + p.name + '">⬇</a>' +
        '<button onclick="deleteSingle(' + p.id + ')">🗑</button>' +
      '</div>';
    grid.appendChild(card);
  });
}

function toggleSelect(id) {
  if (selected.has(id)) selected.delete(id);
  else selected.add(id);
  renderGrid();
}

function toggleSelectAll() {
  if (selected.size === photos.length) selected.clear();
  else photos.forEach(p => selected.add(p.id));
  renderGrid();
}

async function deleteSingle(id) {
  const r = await fetch('/photo/' + id, {method:'DELETE'});
  if (r.ok) {
    photos = photos.filter(p => p.id !== id);
    selected.delete(id);
    renderGrid();
    setStatus(photos.length + ' photo' + (photos.length !== 1 ? 's' : ''));
  }
}

async function deleteSelected() {
  if (selected.size === 0) return;
  const ids = [...selected];
  setStatus('Deleting ' + ids.length + ' photo(s)...');
  for (const id of ids) {
    const r = await fetch('/photo/' + id, {method:'DELETE'});
    if (r.ok) { photos = photos.filter(p => p.id !== id); selected.delete(id); }
  }
  renderGrid();
  setStatus(photos.length + ' photo' + (photos.length !== 1 ? 's' : ''));
}

function promptDeleteAll() {
  document.getElementById('modal').style.display = 'flex';
}
function closeModal() {
  document.getElementById('modal').style.display = 'none';
}
async function confirmDeleteAll() {
  closeModal();
  setStatus('Deleting all photos...');
  const all = [...photos];
  for (const p of all) {
    await fetch('/photo/' + p.id, {method:'DELETE'});
  }
  photos = []; selected.clear();
  renderGrid();
  setStatus('All photos deleted');
}

function downloadSelected() {
  if (selected.size === 0) return;
  [...selected].forEach(id => {
    const p = photos.find(ph => ph.id === id);
    if (!p) return;
    const a = document.createElement('a');
    a.href = '/photo/' + id;
    a.download = p.name;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
  });
}

init();
</script>
</body>
</html>)rawhtml";

// ── BMP helpers ──────────────────────────────────────────────────────────────

static void write_u32_le(uint8_t* b, uint32_t v) {
    b[0] = v & 0xFF; b[1] = (v >> 8) & 0xFF;
    b[2] = (v >> 16) & 0xFF; b[3] = (v >> 24) & 0xFF;
}
static void write_u16_le(uint8_t* b, uint16_t v) {
    b[0] = v & 0xFF; b[1] = (v >> 8) & 0xFF;
}
static uint32_t read_u32_le(const uint8_t* b) {
    return b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}
static inline int bmp_stride(int w) { return (w * 3 + 3) & ~3; }

// ── Extract /path/{n} → integer index ────────────────────────────────────────
static int extract_idx(const String& url, int prefix_len) {
    if (url.length() <= (unsigned)prefix_len) return -1;
    return url.substring(prefix_len).toInt();
}

// ── SD scan ──────────────────────────────────────────────────────────────────
static void scan_sd() {
    s_photos.clear();
    File dir = SD_MMC.open("/");
    if (!dir || !dir.isDirectory()) {
        Serial.println("[SHARE] Cannot open SD root");
        return;
    }
    File f = dir.openNextFile();
    while (f) {
        if (!f.isDirectory()) {
            String name = String(f.name());
            String lower = name; lower.toLowerCase();
            if (lower.endsWith(".bmp")) {
                String full = name.startsWith("/") ? name : "/" + name;
                s_photos.push_back(full);
                Serial.printf("[SHARE] Indexed: %s\n", full.c_str());
            }
        }
        f = dir.openNextFile();
    }
    Serial.printf("[SHARE] %d photos indexed\n", (int)s_photos.size());
}

// ── Thumbnail handler: 4× downsampled BMP, generated in PSRAM ────────────────
static void serve_thumb(AsyncWebServerRequest* req, int idx) {
    if (idx < 0 || idx >= (int)s_photos.size()) { req->send(404); return; }

    File f = SD_MMC.open(s_photos[idx].c_str(), FILE_READ);
    if (!f) { req->send(404); return; }

    uint8_t hdr[54];
    if (f.read(hdr, 54) != 54 || hdr[0] != 'B' || hdr[1] != 'M') {
        f.close(); req->send(500); return;
    }

    int orig_w = (int)read_u32_le(hdr + 18);
    int orig_h = (int)read_u32_le(hdr + 22);
    if (orig_w <= 0 || orig_w > 1024 || orig_h <= 0 || orig_h > 1024) {
        f.close(); req->send(500); return;
    }

    const int SCALE   = 4;
    const int tw      = orig_w / SCALE;
    const int th      = orig_h / SCALE;
    const int src_str = bmp_stride(orig_w);
    const int dst_str = bmp_stride(tw);

    // Full output BMP in PSRAM: 54-byte header + pixel rows
    const size_t out_sz = 54 + (size_t)dst_str * th;
    uint8_t* out = (uint8_t*)ps_malloc(out_sz);
    uint8_t* row = (uint8_t*)ps_malloc(src_str);
    if (!out || !row) {
        if (out) free(out); if (row) free(row);
        f.close(); req->send(503); return;
    }

    // Build BMP file header
    memset(out, 0, 54);
    out[0] = 'B'; out[1] = 'M';
    write_u32_le(out + 2,  (uint32_t)out_sz);   // file size
    write_u32_le(out + 10, 54);                   // pixel data offset
    write_u32_le(out + 14, 40);                   // DIB header size
    write_u32_le(out + 18, (uint32_t)tw);         // width
    write_u32_le(out + 22, (uint32_t)th);         // height (bottom-up)
    write_u16_le(out + 26, 1);                    // colour planes
    write_u16_le(out + 28, 24);                   // bits per pixel
    write_u32_le(out + 34, (uint32_t)(dst_str * th)); // raw image size

    // Read source rows (BMP is bottom-up: src_row 0 = bottom of image).
    // We sample every SCALE-th source row → destination row dst_row.
    for (int src_row = 0; src_row < orig_h; src_row++) {
        int got = f.read(row, src_str);
        if (got != src_str) break;
        if (src_row % SCALE == 0) {
            int dst_row   = src_row / SCALE;               // 0 = bottom
            uint8_t* dst  = out + 54 + (size_t)dst_row * dst_str;
            for (int tx = 0; tx < tw; tx++) {
                int ox = tx * SCALE;
                dst[tx * 3 + 0] = row[ox * 3 + 0]; // B
                dst[tx * 3 + 1] = row[ox * 3 + 1]; // G
                dst[tx * 3 + 2] = row[ox * 3 + 2]; // R
            }
            // Pad row to 4-byte boundary
            for (int p = tw * 3; p < dst_str; p++) dst[p] = 0;
        }
        if ((src_row & 0x1F) == 0) yield();
    }
    free(row);
    f.close();

    // Stream from PSRAM using chunked response (index = byte offset)
    struct MemState { uint8_t* data; size_t len; };
    MemState* ms = new MemState{out, out_sz};

    AsyncWebServerResponse* resp = req->beginChunkedResponse("image/bmp",
        [ms](uint8_t* buf, size_t maxLen, size_t index) -> size_t {
            if (index >= ms->len) {
                free(ms->data);
                delete ms;
                return 0;
            }
            size_t toRead = min(maxLen, ms->len - index);
            memcpy(buf, ms->data + index, toRead);
            return toRead;
        }
    );
    req->send(resp);
}

// ── Full photo handler: stream raw BMP from SD in 4 KB chunks ────────────────
static void serve_photo(AsyncWebServerRequest* req, int idx) {
    if (idx < 0 || idx >= (int)s_photos.size()) { req->send(404); return; }

    const String& path = s_photos[idx];

    struct FileState { File f; };
    FileState* fs = new FileState();
    fs->f = SD_MMC.open(path.c_str(), FILE_READ);
    if (!fs->f) { delete fs; req->send(404); return; }

    // Extract filename for Content-Disposition
    String name = path;
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);

    size_t file_size = fs->f.size();

    AsyncWebServerResponse* resp = req->beginChunkedResponse("image/bmp",
        [fs](uint8_t* buf, size_t maxLen, size_t /*index*/) -> size_t {
            int got = fs->f.read(buf, maxLen);
            if (got <= 0) {
                fs->f.close();
                delete fs;
                return 0;
            }
            return (size_t)got;
        }
    );
    resp->addHeader("Content-Disposition",
                    String("attachment; filename=\"") + name + "\"");
    resp->addHeader("Content-Length", String(file_size));
    req->send(resp);
}

// ── Delete handler ────────────────────────────────────────────────────────────
static void serve_delete(AsyncWebServerRequest* req, int idx) {
    if (idx < 0 || idx >= (int)s_photos.size()) { req->send(404); return; }

    const String& path = s_photos[idx];
    if (SD_MMC.remove(path.c_str())) {
        s_photos.erase(s_photos.begin() + idx);
        // Re-index: client uses sequential IDs, so we just shrink the vector.
        // The client's JS removes the card and rebuilds its local array, so
        // future requests will use the updated indices correctly only if the
        // client reloads the manifest.  We keep it simple: IDs remain stable
        // within one session (client maps id → original position).
        // To avoid ID drift, we DON'T re-number; instead we set the slot to ""
        // so it returns 404 on subsequent requests.
        s_photos.insert(s_photos.begin() + idx, String(""));
        Serial.printf("[SHARE] Deleted index %d\n", idx);
        req->send(200, "text/plain", "OK");
    } else {
        Serial.printf("[SHARE] Delete failed: %s\n", path.c_str());
        req->send(500, "text/plain", "Delete failed");
    }
}

} // anonymous namespace

// ── Public API ───────────────────────────────────────────────────────────────

void wifi_share_start() {
    scan_sd();

    WiFi.mode(WIFI_AP);
    WiFi.softAP(SHARE_SSID, SHARE_PASSWORD);
    Serial.printf("[SHARE] AP: %s  IP: %s\n",
                  SHARE_SSID, WiFi.softAPIP().toString().c_str());

    s_server = new AsyncWebServer(SHARE_PORT);

    // ── GET / ─────────────────────────────────────────────────────────────
    s_server->on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", GALLERY_HTML);
    });

    // ── GET /manifest ─────────────────────────────────────────────────────
    s_server->on("/manifest", HTTP_GET, [](AsyncWebServerRequest* req) {
        String json = "[";
        bool first = true;
        for (int i = 0; i < (int)s_photos.size(); i++) {
            if (s_photos[i].isEmpty()) continue;      // deleted slot
            if (!first) json += ",";
            first = false;
            String name = s_photos[i];
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);
            json += "{\"id\":" + String(i) + ",\"name\":\"" + name + "\"}";
        }
        json += "]";
        req->send(200, "application/json", json);
    });

    // ── Dynamic routes via onNotFound ──────────────────────────────────────
    // /thumb/{n}, /photo/{n} (GET + DELETE)
    s_server->onNotFound([](AsyncWebServerRequest* req) {
        const String& url = req->url();
        if (url.startsWith("/thumb/")) {
            serve_thumb(req, extract_idx(url, 7));
        } else if (url.startsWith("/photo/")) {
            if (req->method() == HTTP_GET) {
                serve_photo(req, extract_idx(url, 7));
            } else if (req->method() == HTTP_DELETE) {
                serve_delete(req, extract_idx(url, 7));
            } else {
                req->send(405);
            }
        } else {
            req->send(404);
        }
    });

    s_server->begin();
    Serial.println("[SHARE] HTTP server started on port 80");
}

void wifi_share_stop() {
    if (s_server) {
        s_server->end();
        delete s_server;
        s_server = nullptr;
    }
    s_photos.clear();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("[SHARE] AP + server stopped");
}
