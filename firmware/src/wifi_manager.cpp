#include "wifi_manager.h"
#include "config.h"
#include "nvs_config.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="id"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Setup DAVA</title>
<style>
  body{font-family:system-ui,sans-serif;background:#0f172a;color:#f1f5f9;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0}
  .card{background:#1e293b;border:1px solid #334155;border-radius:12px;padding:2rem;width:320px}
  h2{margin-bottom:1.5rem;color:#3b82f6;font-size:1.2rem}
  label{display:block;margin-bottom:.3rem;font-size:.85rem;color:#94a3b8}
  input{width:100%;padding:.5rem .7rem;background:#334155;border:1px solid #475569;border-radius:6px;color:#f1f5f9;font-size:.95rem;margin-bottom:1rem;box-sizing:border-box}
  button{width:100%;padding:.6rem;background:#3b82f6;color:#fff;border:none;border-radius:6px;font-size:1rem;font-weight:600;cursor:pointer}
  .msg{margin-top:1rem;font-size:.85rem;text-align:center;color:#22c55e;display:none}
</style></head><body>
<div class="card">
  <h2>Setup WiFi DAVA</h2>
  <form id="f" method="POST" action="/save">
    <label>Nama Device (Alias)</label>
    <input name="alias" required placeholder="contoh: Ruang Server" />
    <label>Nama WiFi (SSID)</label>
    <input name="ssid" required placeholder="Nama WiFi" />
    <label>Password WiFi</label>
    <input name="password" type="password" placeholder="Password WiFi" />
    <button type="submit">Simpan &amp; Reboot</button>
  </form>
  <div class="msg" id="msg">Tersimpan! Device akan reboot...</div>
</div>
<script>
document.getElementById('f').addEventListener('submit',function(e){
  document.getElementById('msg').style.display='block';
});
</script>
</body></html>
)rawliteral";

void wifiProvisioningStart(const String& apName) {
  Serial.println("[WiFi] Starting provisioning AP: " + apName);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName.c_str(), AP_PASSWORD[0] ? AP_PASSWORD : nullptr);

  DNSServer dns;
  dns.start(53, "*", WiFi.softAPIP());

  AsyncWebServer server(80);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", PORTAL_HTML);
  });

  // Captive portal redirects
  server.onNotFound([](AsyncWebServerRequest* req) {
    req->redirect("http://192.168.4.1/");
  });

  bool saved = false;
  server.on("/save", HTTP_POST, [&saved](AsyncWebServerRequest* req) {
    String alias    = req->hasParam("alias",    true) ? req->getParam("alias",    true)->value() : "";
    String ssid     = req->hasParam("ssid",     true) ? req->getParam("ssid",     true)->value() : "";
    String password = req->hasParam("password", true) ? req->getParam("password", true)->value() : "";

    if (ssid.length() > 0) {
      nvsSave(alias, ssid, password);
      saved = true;
      req->send(200, "text/html",
        "<html><body style='font-family:system-ui;background:#0f172a;color:#f1f5f9;text-align:center;padding-top:4rem'>"
        "<h2 style='color:#22c55e'>Tersimpan!</h2><p>Device akan reboot dalam 3 detik...</p></body></html>");
    } else {
      req->send(400, "text/plain", "SSID tidak boleh kosong");
    }
  });

  server.begin();
  Serial.println("[WiFi] Portal IP: " + WiFi.softAPIP().toString());

  unsigned long start = millis();
  while (!saved && (millis() - start) < PORTAL_TIMEOUT_MS) {
    dns.processNextRequest();
    delay(5);
  }

  server.end();

  if (saved) {
    Serial.println("[WiFi] Config saved, rebooting...");
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("[WiFi] Portal timeout, rebooting...");
    ESP.restart();
  }
}
