#include "oled.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>

// ===== OLED PINLER =====
#define OLED_MOSI 21
#define OLED_CLK 23
#define OLED_DC 17
#define OLED_RST 16
#define OLED_CS 5

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define RESET_INFO_TIME 5000

static bool resetInfoActive = false;
static unsigned long resetInfoStart = 0;

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    OLED_MOSI,
    OLED_CLK,
    OLED_DC,
    OLED_RST,
    OLED_CS);

// =======================

void oledInit()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC))
    {
        Serial.println("OLED baslatilamadi");
        return;
    }
    display.clearDisplay();
    display.display();
}

void oledActiveResetInfo()
{
    resetInfoActive = true;
    resetInfoStart = millis();
}
void oledCancelResetInfo()
{
    resetInfoActive = false;
}

void oledBoot()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        oledShowIP(WiFi.localIP().toString());
        return;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ESP32 Basladi");
    display.println("WiFi bekleniyor...");
    display.display();
}

void oledShowResetCountdown(int secondsLeft, int progressPercent)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("RESET MODU");
    display.println("BASILI TUTULUYOR");

    display.setCursor(0, 24);
    display.print("Kalan: ");
    display.print(secondsLeft);
    display.println(" sn");

    // Progress bar
    int barWidth = 100;
    int barHeight = 8;
    int barX = 14;
    int barY = 50;

    display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

    int fillWidth = (barWidth * progressPercent) / 100;
    display.fillRect(barX + 1, barY + 1, fillWidth - 2, barHeight - 2, SSD1306_WHITE);

    display.display();
}

void oledShowIP(String ip)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("WiFi OK");
    display.println("IP:");
    display.println(ip);
    display.display();
}

void oledShowStatus(bool door1, bool door2)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println("SIGNAL:");

    if (door1 == true)
    {
        display.println("DOOR 1");
    }
    else if (door2 == true)
    {
        display.println("DOOR 2");
    }
    display.display();
}

void oledShowApMode(String apName)
{
    // Açılış reset bilgisini iptal et
    resetInfoActive = false;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println("=== RESET MODE ===");
    display.println("");
    display.println("AP MODE AKTIF");
    display.println("");
    display.println(apName);
    display.println("");
    display.println(WiFi.softAPIP().toString());

    display.display();
}

void oledShowResetInfo(uint8_t resetPin)
{
    resetInfoActive = true;
    resetInfoStart = millis();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println("BASLANGIC BILGI");
    display.println("----------------");
    display.println("WiFi RESET PIN:");
    display.setTextSize(1);
    display.setCursor(0, 52);
    display.println("5 sn basili tut");

    display.setTextSize(2);
    display.setCursor(0, 30);
    display.print("GPIO ");
    display.print(resetPin);

    display.setTextSize(1);
    display.display();
}

void oledUpdate()
{
    if (resetInfoActive)
    {
        if (millis() - resetInfoStart >= RESET_INFO_TIME)
        {
            resetInfoActive = false;
            oledBoot(); //   5 sn sonra otomatik normal boot ekranı
        }
    }
}
