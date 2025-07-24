#include "Arduino.h"
//#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <TZ.h>
#include <AddrList.h>
#include <BearSSLHelpers.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include<stdio.h>
#include<string.h>
#include <math.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OLED_SSD1306_Chart.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     (-1) // Reset pin # (or -1 if sharing Arduino reset pin)

OLED_SSD1306_Chart display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/**
 * https://www.dzombak.com/blog/2021/10/https-requests-with-a-root-certificate-store-on-esp8266-arduino/
 * https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
 */

BearSSL::Session tlsSession;

const char* ssid = "YOUR WIFI SSID";
const char* password = "YOUR WIFI PASS";
WiFiClientSecure client;

#include "cert.h"

X509List cert(cert_DigiCert_Global_Root_G2);
const char* http_host = "data.elexon.co.uk";
const int http_port = 443;

const uint16_t ciphers[] PROGMEM = { BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256 };

#define TZ TZ_Europe_London

void setup()
{
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Starting...");
    display.display();

    Serial.begin(921600);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setHostname("elexon-price-monitor");

/*
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    */
    for (bool configured = false; !configured;) {
        for (auto addr : addrList)
            if ((configured = !addr.isLocal()
                 && addr.isV6() // uncomment when IPv6 is mandatory
                 // && addr.ifnumber() == STATION_IF
                 )) { break; }
        Serial.print('.');
        delay(500);
    }

    Serial.println();
    for (auto addr : addrList) {
        Serial.println(addr.toString());
    }
    WiFi.printDiag(Serial);

    display.clearDisplay();
    display.setCursor(0, 10);
    display.println(WiFi.localIP());
    display.display();
    delay(500);


    // NTP
    Serial.print("Waiting for NTP time sync: ");

    configTime(TZ, "ubuntu.pool.ntp.org", "opnsense.pool.ntp.org", "time.nist.gov");

    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {  // Wait for realistic value
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }

    Serial.println();

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("NTP time: ");
    Serial.print(asctime(&timeinfo));

    display.clearDisplay();
    display.setCursor(0, 10);
    display.println(asctime(&timeinfo));
    display.display();

    Serial.print("SDK: ");
    Serial.println(system_get_sdk_version());
    Serial.print("Core: ");
    Serial.println(ESP.getCoreVersion());
    Serial.print("CPU freq: ");
    Serial.println(ESP.getCpuFreqMHz());
    Serial.print("CPU full ver: ");
    Serial.println(ESP.getFullVersion());

    //Serial.printf("Using certificate: %s\n", cert_DigiCert_Global_Root_G2);
    //client.setTrustAnchors(&cert);

    client.probeMaxFragmentLength(http_host, http_port, 512);
    Serial.print("MFLN=");
    if (client.getMFLNStatus()) {
        Serial.println("supported");
        client.setBufferSizes(512, 512);
    } else { Serial.println("not supported"); }

    //client.setCiphersLessSecure();
    //client.setSSLVersion(BR_TLS12, BR_TLS12);
    //client.setCiphers(ciphers, sizeof(ciphers));
    client.setInsecure();

    display.setChartCoordinates(0, 60);
    display.setChartWidthAndHeight(96, 48);
    display.setXIncrement(2);
    display.setYLimits(0, 100);
    //display.setYLimitLabels("50", "100");
    //display.setYLabelsVisible(true);
    display.setAxisDivisionsInc(12, 6);
    display.setPlotMode(SINGLE_PLOT_MODE);
    display.setLineThickness(NORMAL_LINE);
    display.drawChart();
}

JsonDocument doc;
char date_from[20];
char date_to[20];
char url[200];
const char provider[] = "APXMIDP";

void loop()
{

    //String url = "https://data.elexon.co.uk/bmrs/api/v1/balancing/pricing/market-index?from=2025-07-01T00:00Z&to=2025-07-01T11:50Z&dataProviders=APXMIDP&format=json";

    time_t now = time(nullptr);

    client.setX509Time(now);

    // convert now time into beginning of today by resetting time to 0
    struct tm *beginning_of_day = gmtime(&now);
    beginning_of_day->tm_hour = 0;
    beginning_of_day->tm_min = 0;

    client.setSession(&tlsSession);

    strftime(date_from, sizeof(date_from), "%FT%TZ", beginning_of_day);
    strftime(date_to, sizeof(date_to), "%FT%TZ", gmtime(&now));
    sprintf(url, "https://data.elexon.co.uk/bmrs/api/v1/balancing/pricing/market-index?from=%s&to=%s&dataProviders=%s&format=json", date_from, date_to, provider);

    Serial.printf("URL=%s\n", url);

    HTTPClient httpClient;
    httpClient.begin(client, url);
    int respCode = httpClient.GET();
    if (respCode >= 400) {
        Serial.printf("HTTP error: %d\n", respCode);
    } else if (respCode > 0) {
        String result = httpClient.getString();
        //Serial.println(result);

        DeserializationError error = deserializeJson(doc, result);

        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        } else {
            // update chart with the historic prices from the start of the day
            display.clearDisplay();

            JsonArray data = doc["data"];

            // determine min/max Y range of the data set for chart scaling
            int min_y  = data[0]["price"] ;
            int max_y = data[0]["price"];
            for (JsonVariant item : data) {
                if (item["price"] > max_y)
                    max_y = (double) item["price"] + 5;
                if (item["price"] < min_y)
                    min_y = (double) item["price"] - 5;
            }
            Serial.printf("Y range %d %d\n", min_y, max_y);
            // update chart Y range
            display.setYLimits(min_y, max_y);
            Serial.print("Prices since midnight: ");

            // draw the actual chart with fetched data
            for (JsonVariant item : data) {
                double price = item["price"];
                Serial.printf("%.2f ", price);
                if (!display.updateChart(price)) {
                    display.clearDisplay();
                    display.drawChart();
                }
            }
            Serial.println();

            // check the latest price trend
            double price_now = doc["data"][0]["price"];
            double price_previous = doc["data"][1]["price"];

            Serial.printf("Last price change: %f -> %f\n", price_previous, price_now);

            // display latest price in top right corner
            display.setCursor(96, 0);
            display.printf("%.2f", price_now);

            // display trend indicator arrow on the right
            if (price_now > price_previous) {
                // arrow facing up
                display.fillTriangle(100, 60, 120, 60, 110, 30, WHITE);
            } else
                {
                // arrow facing down
                display.fillTriangle(100, 30, 120, 30, 110, 60, WHITE);
            }

            display.display();


        }
    } else {
        Serial.printf("Other error: %d\n", respCode);
    }
    httpClient.end();

    delay(15*60*1000); // 15 minutes

}