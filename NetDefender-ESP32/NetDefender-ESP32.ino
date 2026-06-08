#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <esp_pm.h>

// TODO: REMEMBER TO CHANGE THIS BEFORE FLASHING!!!
const char* ap_ssid = "Netdefender-Node";
const char* ap_password = "admin12345678";
const char* ssid_internet = "YOUR_WIFI_NAME"; 
const char* password_internet = "YOUR_WIFI_PASS";
#define BOTtoken "123456789:ABCdefGhIJKlmNoPQ"
#define CHAT_ID "987654321"

// global control flags (dirty way)
bool idsMode = true;
bool spectrumMonitor = true;
bool secureNode2FA = false;
bool telegramAlerts = false;
bool sdLogging = false;

WebServer server(80);
WiFiClientSecure tgClient;
UniversalTelegramBot bot(BOTtoken, tgClient);

// global variables for non-blocking delay timers
unsigned long t_tg = 0;
unsigned long t_ids = 0;
unsigned long t_spec = 0;
unsigned long t_sd = 0;
int w_retry = 0; 

void setup() {
  Serial.begin(115200);
  delay(600); // boot delay just in case
  Serial.println("\n*** STARTING CORE v2.2 ***");

  // power management (modem sleep hack to save battery)
  esp_pm_config_esp32_t pm;
  pm.max_freq_mhz = 240;
  pm.min_freq_mhz = 80; // DO NOT DROP BELOW 80 OR WIFI WILL CRASH
  pm.light_sleep_enable = true;
  if (esp_pm_configure(&pm) == ESP_OK) {
    Serial.println("power config applied");
  }

  // init AP for local panel
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.setSleep(true); // radio sleep on
  Serial.print("Local IP: ");
  Serial.println(WiFi.softAPIP());

  // connect to home router for internet
  tgClient.setInsecure(); // ignore SSL cert validation bypass
  WiFi.begin(ssid_internet, password_internet);
  Serial.print("connecting sta");
  
  // connection retry loop
  while (WiFi.status() != WL_CONNECTED && w_retry < 12) {
    delay(1000);
    Serial.print(".");
    w_retry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\ninternet ok, bot armed");
    telegramAlerts = true;
    bot.sendMessage(CHAT_ID, "node online (battery saver mode)", "");
  } else {
    Serial.println("\nconnection failed, tg disabled");
    telegramAlerts = false; // block crash loops
  }

  // http routes
  server.on("/", handleMainPanel);
  server.on("/toggle", handleStateToggle);
  server.begin();

  // console manual menu
  Serial.println("\nCommands:");
  Serial.println("status | ids on | ids off | spec on | spec off | tg on | tg off | sd on | sd off");
}

void loop() {
  server.handleClient();
  readSerial();       
  runTgBot();     

  // engine loops
  if (idsMode) doIDS();
  if (spectrumMonitor) doSpectrum();
  if (sdLogging) doSD();
}

// ugly raw serial input parser
void readSerial() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "status") {
      Serial.println("\n--- STATE ---");
      Serial.print("IDS: "); Serial.println(idsMode ? "1" : "0");
      Serial.print("SPEC: "); Serial.println(spectrumMonitor ? "1" : "0");
      Serial.print("TG: "); Serial.println(telegramAlerts ? "1" : "0");
      Serial.print("SD: "); Serial.println(sdLogging ? "1" : "0");
      Serial.print("2FA: "); Serial.println(secureNode2FA ? "LOCK" : "OPEN");
    } 
    else if (cmd == "ids on")   idsMode = true;
    else if (cmd == "ids off")  idsMode = false;
    else if (cmd == "spec on")  spectrumMonitor = true;
    else if (cmd == "spec off") spectrumMonitor = false;
    else if (cmd == "tg on")    telegramAlerts = true;
    else if (cmd == "tg off")   telegramAlerts = false;
    else if (cmd == "sd on")    sdLogging = true;
    else if (cmd == "sd off")   sdLogging = false;
    else if (cmd == "2fa on")   secureNode2FA = true;
    else if (cmd == "2fa off")  secureNode2FA = false;
  }
}

// telegram loop
void runTgBot() {
  if (millis() - t_tg > 2500) { // check tg query queue every 2.5s
    int updates = bot.getUpdates(bot.last_message_received + 1);

    while (updates) {
      for (int i = 0; i < updates; i++) {
        String uid = String(bot.messages[i].chat_id);
        
        if (uid != CHAT_ID) {
          bot.sendMessage(uid, "denied", "");
          continue;
        }

        String msg = bot.messages[i].text;

        if (msg == "/status") {
          String s = "Status:\n";
          s += "IDS: " + String(idsMode ? "ON" : "OFF") + "\n";
          s += "SPEC: " + String(spectrumMonitor ? "ON" : "OFF") + "\n";
          s += "SD: " + String(sdLogging ? "ON" : "OFF") + "\n";
          s += "2FA: " + String(secureNode2FA ? "LOCKED" : "OPEN");
          bot.sendMessage(uid, s, "");
        } 
        else if (msg == "/ids_on")   { idsMode = true; bot.sendMessage(uid, "ids on", ""); }
        else if (msg == "/ids_off")  { idsMode = false; bot.sendMessage(uid, "ids off", ""); }
        else if (msg == "/spec_on")  { spectrumMonitor = true; bot.sendMessage(uid, "spec on", ""); }
        else if (msg == "/spec_off") { spectrumMonitor = false; bot.sendMessage(uid, "spec off", ""); }
        else if (msg == "/sd_on")    { sdLogging = true; bot.sendMessage(uid, "sd on", ""); }
        else if (msg == "/sd_off")   { sdLogging = false; bot.sendMessage(uid, "sd off", ""); }
      }
      updates = bot.getUpdates(bot.last_message_received + 1);
    }
    t_tg = millis();
  }
}

// dirty hardcoded HTML page layout
void handleMainPanel() {
  String page = "<html><head><title>Panel</title><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<style>body{background:#111;color:#fff;font-family:monospace;text-align:center;} .card{background:#222;max-width:350px;margin:50px auto;padding:20px;} ";
  page += ".r{display:flex;justify-content:space-between;margin-bottom:15px;} .b{color:#fff;text-decoration:none;padding:5px 10px;} .g{background:green;} .d{background:red;}</style></head><body>";
  page += "<div class='card'><h2>Netdefender</h2>";
  
  page += "<div class='r'><span>IDS</span><a class='b " + String(idsMode ? "g" : "d") + "' href='/toggle?set=ids'>" + (idsMode ? "ON" : "OFF") + "</a></div>";
  page += "<div class='r'><span>Spectrum</span><a class='b " + String(spectrumMonitor ? "g" : "d") + "' href='/toggle?set=spec'>" + (spectrumMonitor ? "ON" : "OFF") + "</a></div>";
  page += "<div class='r'><span>SD Card</span><a class='b " + String(sdLogging ? "g" : "d") + "' href='/toggle?set=sd'>" + (sdLogging ? "ON" : "OFF") + "</a></div>";
  page += "<div class='r'><span>Telegram</span><a class='b " + String(telegramAlerts ? "g" : "d") + "' href='/toggle?set=tg'>" + (telegramAlerts ? "ON" : "OFF") + "</a></div>";
  page += "<div class='r'><span>2FA Lock</span><a class='b " + String(secureNode2FA ? "g" : "d") + "' href='/toggle?set=2fa'>" + (secureNode2FA ? "LOCK" : "OPEN") + "</a></div>";
  
  page += "</div></body></html>";
  server.send(200, "text/html", page);
}

void handleStateToggle() {
  if (server.hasArg("set")) {
    String arg = server.arg("set");
    if (arg == "ids")  idsMode = !idsMode;
    if (arg == "spec") spectrumMonitor = !spectrumMonitor;
    if (arg == "sd")   sdLogging = !sdLogging;
    if (arg == "tg")   telegramAlerts = !telegramAlerts;
    if (arg == "2fa")  secureNode2FA = !secureNode2FA;
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// core routines
void doIDS() {
  if (millis() - t_ids > 6000) {
    // paste raw register/sniffer code here later
    Serial.println("[IDS] sniffing");
    t_ids = millis();
  }
}

void doSpectrum() {
  if (millis() - t_spec > 7000) {
    // paste channel hop loop here later
    Serial.println("[SPEC] hopping");
    t_spec = millis();
  }
}

void doSD() {
  if (millis() - t_sd > 10000) {
    // paste file write blocks here later
    Serial.println("[SD] syncing log");
    t_sd = millis();
  }
}
