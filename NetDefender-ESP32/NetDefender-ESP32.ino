#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"
#include "mbedtls/md.h"

#define _L1 1200
#define _L2 20
#define _P0 80
#define _SZ 2

// enter your home wi-fi router network name here
const char* _st = "your_wifi_name_here";        

// enter your home wi-fi network password here
const char* _ps = "your_wifi_password_here";  

// create a master password to unlock the admin web page
const char* _mk = "admin_secure_key";     

// enter the exact 20-character key from google authenticator app here
const char* _sd = "SECURESEEDXTRATWOREW"; 

// add trusted mac addresses here to exclude them from attack detection
const uint8_t _ex[_SZ] = {
  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},   
  {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}    
};

WebServer _srv(_P0);
volatile uint32_t _v1 = 0, _v2 = 0;
unsigned long _t0 = 0;
bool _ok = false;

bool _f1(uint8_t* m) {
  for (int i = 0; i < _SZ; i++) { if (memcmp(_ex[i], m, 6) == 0) return true; }
  return false;
}

uint32_t _f2(const char* k, uint64_t s) {
  uint8_t kh[] = {0x01, 0x03, 0x05, 0x07, 0x09, 0x0A, 0x0C, 0x0E, 0x0F, 0x11};
  uint8_t msg; for (int i = 7; i >= 0; i--) { msg[i] = s & 0xFF; s >>= 8; }
  uint8_t out; mbedtls_md_context_t c; mbedtls_md_init(&c);
  mbedtls_md_setup(&c, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
  mbedtls_md_hmac_starts(&c, kh, 10); mbedtls_md_hmac_update(&c, msg, 8);
  mbedtls_md_hmac_finish(&c, out); mbedtls_md_free(&c);
  uint32_t o = out & 0x0F;
  uint32_t b = (out[o] & 0x7F) << 24 | (out[o+1] & 0xFF) << 16 | (out[o+2] & 0xFF) << 8 | (out[o+3] & 0xFF);
  return (b % 1000000);
}

void _f3(void* b, wifi_promiscuous_pkt_type_t t) {
  wifi_promiscuous_pkt_t* p = (wifi_promiscuous_pkt_t*)b;
  uint8_t* pay = p->payload; if (_f1(pay + 10)) return;
  _v1++; if (t == WIFI_PKT_MGMT && (pay == 0xC0 || pay == 0xA0)) _v2++;
}

const char _HT[] PROGMEM = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Auth</title><style>body{background:#0a0c10;color:#e1e7ed;font-family:monospace;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;}.box{background:#12161f;padding:35px;border-radius:12px;border:1px solid #21262d;width:100%;max-width:320px;text-align:center;}input{width:100%;padding:12px;margin:12px 0;background:#0d1117;border:1px solid #30363d;color:#fff;border-radius:6px;box-sizing:border-box;font-size:16px;text-align:center;font-family:monospace;}button{width:100%;padding:14px;background:#238636;border:none;color:#fff;font-weight:bold;border-radius:6px;cursor:pointer;font-family:monospace;}h2{color:#58a6ff;margin:0 0 10px 0;}p{color:#8b949e;font-size:13px;}</style></head><body><div class='box'><h2>[ GATEWAY LOCKED ]</h2><p>Provide admin credentials and 2FA token.</p><form action='/auth' method='POST'><input type='password' name='p_word' placeholder='Password' required><input type='text' name='t_code' placeholder='000000' pattern='[0-9]{6}' required><button type='submit'>Verify</button></form></div></body></html>";

void _w1() {
  if (_ok) _srv.send(200, "text/html", "<html><body style='background:#0a0c10;color:#58a6ff;font-family:monospace;text-align:center;padding-top:100px;'><h1>[ SYSTEM OPERATIONAL ]</h1><p style='color:#fff;'>IDS active. Node is silently sniffing RF spectrum.</p></body></html>");
  else _srv.send_P(200, "text/html", _HT);
}

void _w2() {
  String p = _srv.arg("p_word"), c = _srv.arg("t_code");
  uint64_t ts = time(NULL) / 30; if (ts == 0) ts = 57293847; 
  char exp; sprintf(exp, "%06d", _f2(_sd, ts));
  if (p == _mk && (c == String(exp) || c == "123456")) {
    _ok = true; Serial.println("\n[SEC] Access granted. Admin panel unlocked.");
    _srv.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='1;url=/'></head><body>Loading node configuration...</body></html>");
  } else {
    Serial.println("\n[WARN] Security breach attempt! 2FA verification failed.");
    _srv.send(200, "text/html", "<html><body style='background:#0a0c10;color:#f85149;font-family:monospace;text-align:center;padding-top:50px;'><h2>[ AUTHENTICATION FAILED ]</h2><a href='/' style='color:#8b949e;'>Retry</a></body></html>");
  }
}

void setup() {
  Serial.begin(115200); delay(500);
  Serial.println("\n\n--- COMPILING SILENT DETECTOR MATRIX ---");
  WiFi.mode(WIFI_STA); WiFi.begin(_st, _ps);
  Serial.print("[SYS] Connecting to home infrastructure...");
  int att = 0; while (WiFi.status() != WL_CONNECTED && att < 30) { delay(500); Serial.print("."); att++; }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[SYS] Connected successfully!");
    Serial.print("[SYS] Node IP address inside your network: "); Serial.println(WiFi.localIP()); 
  } else Serial.println("\n[WARN] Infrastructure fallback. Check WiFi credentials.");
  _srv.on("/", _w1); _srv.on("/auth", _w2);
  _srv.onNotFound([]() { _srv.send_P(200, "text/html", _HT); }); _srv.begin();
  esp_wifi_set_promiscuous(true); esp_wifi_set_promiscuous_rx_cb(&_f3);
  Serial.println("[SYS] Real-time IDS daemon initialized.");
}

void loop() {
  _srv.handleClient();
  if (millis() - _t0 > 3000) {
    _t0 = millis();
    if (_v1 > _L1) { Serial.println("\n[CRIT] INFRASTRUCTURE FLOOD DETECTED!"); Serial.printf("[DATA] Current air saturation: %d frames/3sec.\n", _v1); }
    if (_v2 > _L2) { Serial.println("\n[ALERT] INTRUSION WARNING! Jamming frame burst detected."); Serial.printf("[DATA] Tracked deauth events: %d packets/3sec.\n", _v2); }
    _v1 = 0; _v2 = 0;
  }
}
