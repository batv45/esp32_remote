/*********
  Based on Random Nerd Tutorials
  Cleaned & structured for future expansion
*********/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>

// ===================== SERVER =====================
AsyncWebServer server(80);

// ===================== FORM PARAMS =====================
const char *PARAM_SSID = "ssid";
const char *PARAM_PASS = "pass";
const char *PARAM_IP = "ip";
const char *PARAM_GATEWAY = "gateway";

// ===================== STORAGE PATHS =====================
const char *SSID_PATH = "/ssid.txt";
const char *PASS_PATH = "/pass.txt";
const char *IP_PATH = "/ip.txt";
const char *GATEWAY_PATH = "/gateway.txt";

// ===================== WIFI VARS =====================
String wifiSSID;
String wifiPASS;
String wifiIP;
String wifiGATEWAY;

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

// ===================== TIMING =====================
unsigned long wifiStartMillis;
const unsigned long WIFI_TIMEOUT = 10000;

// ===================== DEMO LED =====================
const int LED_PIN = 2;

// ===================== FILESYSTEM =====================
void initFS()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("[FS] Mount failed");
        return;
    }
    Serial.println("[FS] Mounted");
}

String readFile(const char *path)
{
    File file = LittleFS.open(path);
    if (!file)
        return "";
    String value = file.readStringUntil('\n');
    file.close();
    return value;
}

void writeFile(const char *path, const String &value)
{
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file)
        return;
    file.print(value);
    file.close();
}

// ===================== WIFI =====================
bool connectWiFi()
{
    if (wifiSSID == "" || wifiIP == "")
    {
        Serial.println("[WIFI] Missing credentials");
        return false;
    }

    WiFi.mode(WIFI_STA);

    localIP.fromString(wifiIP);
    localGateway.fromString(wifiGATEWAY);

    if (!WiFi.config(localIP, localGateway, subnet))
    {
        Serial.println("[WIFI] Config failed");
        return false;
    }

    WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
    Serial.println("[WIFI] Connecting...");

    wifiStartMillis = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - wifiStartMillis > WIFI_TIMEOUT)
        {
            Serial.println("[WIFI] Timeout");
            return false;
        }
        delay(300);
    }

    Serial.print("[WIFI] Connected IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

// ===================== WEB ROUTES =====================
void startWebServer()
{

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/index.html", "text/html"); });

    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *req)
              {
    digitalWrite(LED_PIN, HIGH);
    req->redirect("/"); });

    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *req)
              {
    digitalWrite(LED_PIN, LOW);
    req->redirect("/"); });

    server.serveStatic("/", LittleFS, "/");
    server.begin();

    Serial.println("[WEB] Server started");
}

// ===================== AP MODE =====================
void startAPMode()
{
    Serial.println("[AP] Starting setup portal");

    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP-WIFI-MANAGER-0012");

    Serial.print("[AP] IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/wifimanager.html", "text/html"); });

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *req)
              {

    if (req->hasParam(PARAM_SSID, true)) {
      wifiSSID = req->getParam(PARAM_SSID, true)->value();
      writeFile(SSID_PATH, wifiSSID);
    }

    if (req->hasParam(PARAM_PASS, true)) {
      wifiPASS = req->getParam(PARAM_PASS, true)->value();
      writeFile(PASS_PATH, wifiPASS);
    }

    if (req->hasParam(PARAM_IP, true)) {
      wifiIP = req->getParam(PARAM_IP, true)->value();
      writeFile(IP_PATH, wifiIP);
    }

    if (req->hasParam(PARAM_GATEWAY, true)) {
      wifiGATEWAY = req->getParam(PARAM_GATEWAY, true)->value();
      writeFile(GATEWAY_PATH, wifiGATEWAY);
    }

    req->send(200, "text/plain", "Saved. Restarting...");
    delay(1500);
    ESP.restart(); });

    server.begin();
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi bağlantısı koptu!");
        WiFi.reconnect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.print("WiFi bağlı. IP: ");
        Serial.println(WiFi.localIP());
        break;
    default:
        break;
    }
}

// ===================== SETUP =====================
void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    WiFi.onEvent(WiFiEvent);

    initFS();

    wifiSSID = readFile(SSID_PATH);
    wifiPASS = readFile(PASS_PATH);
    wifiIP = readFile(IP_PATH);
    wifiGATEWAY = readFile(GATEWAY_PATH);

    if (connectWiFi())
    {
        startWebServer();
    }
    else
    {
        startAPMode();
    }
}

unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 5000; // 5 saniye

// ===================== LOOP =====================
void loop()
{

    // mevcut loop kodların burada aynen kalsın
    // -----------------------------

    if (millis() - lastWifiCheck > wifiCheckInterval)
    {
        lastWifiCheck = millis();

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi koptu, yeniden bağlanılıyor...");
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}
