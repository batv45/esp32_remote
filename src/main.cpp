#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <RCSwitch.h>
#include "oled.h"

#define FW_NAME "TEKNO ROT BALANS - ESP32"
#define FW_VERSION "v1.0.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// ================== AYARLAR ==================
#define RESET_PIN 13
#define RESET_HOLD_MS 5000

#define RF_RX_PIN 22
#define RF_TX_PIN 25
#define RF_LISTEN_TIME 15000 // 10 saniye

#define RF_CODE_DOOR1 10931492 // kapi 1 sinyal kodu
#define RF_CODE_DOOR2 10931490 // kapi 2 sinyal kodu
#define RF_BIT_LENGTH 24
#define RF_PROTOCOL 1

// ================== GLOBAL ==================
WiFiManager wifiManager;
AsyncWebServer server(4561);

unsigned long resetStart = 0;
int lastShownSecond = -1;
bool resetInProgress = false;

RCSwitch rf = RCSwitch();
unsigned long rfStartTime = 0;
bool rfListenMode = true;
bool rfTxInitialized = false;

// Forward declaration
void WiFiEvent(WiFiEvent_t event);

// ================== HTML UI ==================
const char *INDEX_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Dükkan Panel</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 40px; }
    button { font-size: 20px; padding: 20px; margin: 15px; width: 200px; }
  </style>
</head>
<body>
  <h1>Dükkan Kontrol Paneli</h1>

  <button onclick="fetch('/door1')">Kapı 1</button><br>
  <button onclick="fetch('/door2')">Kapı 2</button>

</body>
</html>
)rawliteral";

void printBootBanner()
{
    Serial.println();
    Serial.println("==============================================");
    Serial.print("   ");
    Serial.println(FW_NAME);
    Serial.println("==============================================");

    Serial.print("   Firmware : ");
    Serial.println(FW_VERSION);

    Serial.print("   Build    : ");
    Serial.print(BUILD_DATE);
    Serial.print(" ");
    Serial.println(BUILD_TIME);
    Serial.print("   Chip ID  : ");
    Serial.println((uint32_t)ESP.getEfuseMac());

    Serial.println("----------------------------------------------");
}

void rfInitTX()
{
    if (rfTxInitialized)
        return;

    rf.enableTransmit(RF_TX_PIN); // SADECE BU
    rf.setProtocol(RF_PROTOCOL);
    rf.setPulseLength(350);

    rfTxInitialized = true;
    Serial.println("[RF] TX MODE AKTIF");
}

void sendDoor1()
{
    rfInitTX();
    Serial.println("[RF][TX] Door1 gonderiliyor");
    oledActiveResetInfo();
    oledShowStatus(true, false);
    rf.send(RF_CODE_DOOR1, RF_BIT_LENGTH);
}

void sendDoor2()
{
    rfInitTX();
    Serial.println("[RF][TX] Door2 gonderiliyor");
    oledActiveResetInfo();
    oledShowStatus(false, true);
    rf.send(RF_CODE_DOOR2, RF_BIT_LENGTH);
}

void configModeCallback(WiFiManager *myWiFiManager)
{

    Serial.println();
    Serial.println("==================================");
    Serial.println("   RESET MODE - AP AKTIF         ");
    Serial.println("==================================");

    oledShowApMode(myWiFiManager->getConfigPortalSSID()); // OLED: AP MODE AKTIF
}

// ================== SETUP ==================
void setup()
{
    Serial.begin(115200);
    printBootBanner();

    pinMode(RESET_PIN, INPUT_PULLUP);

    rf.enableReceive(RF_RX_PIN); // RX

    rfStartTime = millis();
    rfListenMode = true;

    oledInit();
    oledShowResetInfo(13);

    wifiManager.setDebugOutput(true);
    wifiManager.setTimeout(180);

    wifiManager.setAPCallback(configModeCallback);

    // WiFi baglan / captive portal
    if (!wifiManager.autoConnect("TEKNO_ESP32_AP"))
    {
        Serial.println();
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println("!!!  WIFI BAGLANAMADI - RESTART !!!");
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

        ESP.restart();
    }

    Serial.println("[WiFi] BAGLANDI");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // ================== WEB SERVER ==================
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", INDEX_HTML); });

    server.on("/door1", HTTP_GET, [](AsyncWebServerRequest *request)
              {
            sendDoor1(); 
    request->send(200, "text/plain", "OK"); });

    server.on("/door2", HTTP_GET, [](AsyncWebServerRequest *request)
              {     
    sendDoor2();
    request->send(200, "text/plain", "OK"); });

    server.begin();

    WiFi.onEvent(WiFiEvent);

    oledCancelResetInfo();
    oledShowIP(WiFi.localIP().toString());
    Serial.println("[WEB] SERVER STARTED");

    Serial.println();
    Serial.println("=============================");
    Serial.println("   RF DINLEME BASLADI 15sn    ");
    Serial.println("=============================");
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {

    case SYSTEM_EVENT_STA_CONNECTED:
        Serial.println("WiFi AP'ye baglandi");
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.print("IP alindi: ");
        Serial.println(WiFi.localIP());
        oledShowIP(WiFi.localIP().toString());
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi koptu!");
        oledBoot();
        break;

    default:
        break;
    }
}

// ================== LOOP ==================
void loop()
{

    // RESET BUTONU (5 sn)
    if (digitalRead(RESET_PIN) == LOW)
    {
        if (!resetInProgress)
        {
            resetInProgress = true;
            resetStart = millis();
            lastShownSecond = -1;

            Serial.println();
            Serial.println("==================================");
            Serial.println("   RESET BUTONU BASILDI          ");
            Serial.println("   5 SN BASILI TUTULUYOR         ");
            Serial.println("==================================");
        }

        unsigned long elapsed = millis() - resetStart;
        int secondsLeft = (RESET_HOLD_MS - elapsed) / 1000 + 1;
        if (secondsLeft < 0)
            secondsLeft = 0;

        int progressPercent = (elapsed * 100) / RESET_HOLD_MS;
        if (progressPercent > 100)
            progressPercent = 100;

        // OLED sadece saniye değişince güncellensin
        if (secondsLeft != lastShownSecond)
        {
            lastShownSecond = secondsLeft;
            oledShowResetCountdown(secondsLeft, progressPercent);
        }

        if (elapsed >= RESET_HOLD_MS)
        {
            Serial.println("[WiFi] AYARLAR SIFIRLANIYOR...");
            wifiManager.resetSettings();
            delay(300);
            ESP.restart();
        }
    }
    else
    {
        // Reset iptal
        if (resetInProgress)
        {
            resetInProgress = false;
            oledBoot(); // Reset iptal edildiğinde boot ekranını göster
        }
    }

    // ===== 10 SN RX DINLEME =====
    if (rfListenMode)
    {
        if (rf.available())
        {
            Serial.print("433 Received :: ");
            Serial.println(rf.getReceivedValue());
            rf.resetAvailable();
        }

        if (millis() - rfStartTime >= RF_LISTEN_TIME)
        {
            rfListenMode = false;

            Serial.println();
            Serial.println("=============================");
            Serial.println("      RF DINLEME KAPANDI      ");
            Serial.println("=============================");
        }

        return;
    }

    // ===== NORMAL MODE =====

    oledUpdate();
}
