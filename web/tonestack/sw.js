// Offline-Zwischenspeicher. Nach Änderungen an den Seiten VERSION erhöhen.
const VERSION = "2026-11-07-2";
const FILES = [
  "./",
  "index.html",
  "assets/fonts/archivo-latin.woff2",
  "yamaha-b100ii/b100ii-worklet.js",
  "laney-aor50/laney-worklet.js",
  "laney-aor50/laney-render.js",
  "yamaha-b100ii/",
  "yamaha-b100ii/index.html",
  "yamaha-b100ii/manifest.webmanifest",
  "assets/icons/yamaha-b100ii-192.png",
  "assets/icons/yamaha-b100ii-512.png",
  "laney-aor50/",
  "laney-aor50/index.html",
  "laney-aor50/manifest.webmanifest",
  "assets/icons/laney-aor50-192.png",
  "assets/icons/laney-aor50-512.png",
  "ampeg-b15nf/",
  "ampeg-b15nf/index.html",
  "ampfett-b15nf/",
  "ampfett-b15nf/index.html",
  "ampfett-b15nf/manifest.webmanifest",
  "ampfett-b15nf/b15nf-worklet.js",
  "ampfett-b15nf/b15nf-render.js",
  "assets/icons/ampfett-b15nf-192.png",
  "assets/icons/ampfett-b15nf-512.png",
  "gk-800rb/",
  "gk-800rb/index.html",
  "gk-800rb/manifest.webmanifest",
  "gk-800rb/gk-worklet.js",
  "gk-800rb/gk-render.js",
  "assets/icons/gk-800rb-192.png",
  "assets/icons/gk-800rb-512.png",
  "yamaha-g100/",
  "yamaha-g100/index.html",
  "yamaha-g100/manifest.webmanifest",
  "yamaha-g100/g100-worklet.js",
  "yamaha-g100/g100-render.js",
  "assets/icons/yamaha-g100-192.png",
  "assets/icons/yamaha-g100-512.png",
  "yamaha-b100iii/",
  "yamaha-b100iii/index.html",
  "yamaha-b100iii/manifest.webmanifest",
  "yamaha-b100iii/b3-worklet.js",
  "yamaha-b100iii/b3-render.js",
  "assets/icons/yamaha-b100iii-192.png",
  "assets/icons/yamaha-b100iii-512.png",
  "ampfett-svtcl/",
  "ampfett-svtcl/index.html",
  "ampfett-svtcl/manifest.webmanifest",
  "ampfett-svtcl/svtcl-worklet.js",
  "ampfett-svtcl/svtcl-render.js",
  "assets/icons/ampfett-svtcl-192.png",
  "assets/icons/ampfett-svtcl-512.png"
];
self.addEventListener("install", e => {
  e.waitUntil(caches.open(VERSION).then(c => c.addAll(FILES)).then(() => self.skipWaiting()));
});
self.addEventListener("activate", e => {
  e.waitUntil(caches.keys().then(ks => Promise.all(ks.filter(k => k !== VERSION).map(k => caches.delete(k)))).then(() => self.clients.claim()));
});
// Netz zuerst (immer aktuell), bei fehlender Verbindung aus dem Zwischenspeicher
self.addEventListener("fetch", e => {
  if (e.request.method !== "GET" || new URL(e.request.url).origin !== location.origin || new URL(e.request.url).pathname.includes("/download/")) return;   // Downloads nicht zwischenspeichern
  e.respondWith(fetch(e.request).then(r => {
    if (r.ok && r.status !== 206) { const copy = r.clone(); e.waitUntil(caches.open(VERSION).then(c => c.put(new Request(new URL(e.request.url).pathname), copy)).catch(() => {})); }   // ohne Abfrageteil speichern, Teilantworten nicht
    return r;
  }).catch(() => caches.match(e.request, {ignoreSearch: true}).then(m => m || (e.request.mode === "navigate" ? caches.match("./") : undefined))));
});
