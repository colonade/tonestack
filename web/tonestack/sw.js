// Offline-Zwischenspeicher. Nach Änderungen an den Seiten VERSION erhöhen.
const VERSION = "2026-10-29-3";
const FILES = [
  "./",
  "index.html",
  "assets/fonts/archivo-latin.woff2",
  "yamaha-b100ii/b100ii-worklet.js",
  "laney-aor50/laney-worklet.js",
  "yamaha-b100ii/",
  "yamaha-b100ii/index.html",
  "yamaha-b100ii/manifest.webmanifest",
  "assets/icons/yamaha-b100ii-192.png",
  "assets/icons/yamaha-b100ii-512.png",
  "laney-aor50/",
  "laney-aor50/index.html",
  "laney-aor50/manifest.webmanifest",
  "assets/icons/laney-aor50-192.png",
  "assets/icons/laney-aor50-512.png"
];
self.addEventListener("install", e => {
  e.waitUntil(caches.open(VERSION).then(c => c.addAll(FILES)).then(() => self.skipWaiting()));
});
self.addEventListener("activate", e => {
  e.waitUntil(caches.keys().then(ks => Promise.all(ks.filter(k => k !== VERSION).map(k => caches.delete(k)))).then(() => self.clients.claim()));
});
// Netz zuerst (immer aktuell), bei fehlender Verbindung aus dem Zwischenspeicher
self.addEventListener("fetch", e => {
  if (e.request.method !== "GET" || new URL(e.request.url).origin !== location.origin) return;
  e.respondWith(fetch(e.request).then(r => {
    if (r.ok) { const copy = r.clone(); caches.open(VERSION).then(c => c.put(e.request, copy)); }
    return r;
  }).catch(() => caches.match(e.request, {ignoreSearch: true})));
});
