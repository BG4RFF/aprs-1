#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "brains.h"
#include "APRS.h"
#include "printConsole.h"
#include "WebServer.h"

#define DEBUG

SoftwareSerial ss_gps(D6, D7, false, 256); // RX, TX
SoftwareSerial ss_radio(D2, D3, false, 256); //RX, TX

IPAddress apIP(192, 168, 1, 1);
ESP8266WebServer webServer(80);
DNSServer dnsServer;

TinyGPSPlus gps;
TinyGPSCustom RawLat(gps, "GPGGA", 2);
TinyGPSCustom RawLatDir(gps, "GPGGA", 3);
TinyGPSCustom RawLong(gps, "GPGGA", 4);
TinyGPSCustom RawLongDir(gps, "GPGGA", 5);

HardwareSerial& console = Serial;

const int RadioRst = D1;

Ticker tPrintConsole;
Ticker tPrintAPRS;
Ticker tGPSUpdate;

volatile bool ppsTriggered = false;

bool newRadioData = false;

void ppsHandler(void)
{
  ppsTriggered = true;
}

void gpsHardwareReset()
{
  // Empty input buffer
  while (ss_gps.available())
    console.write(ss_gps.read());
}

void setup() {
  // Open serial communications and wait for port to open:
//  Serial.setDebugOutput(true);
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(RadioRst, OUTPUT);
  console.println("Resetting Radio ...");
  digitalWrite(RadioRst, LOW);
  delay (100);
  digitalWrite(RadioRst, HIGH);
  console.println("... done");
  // set the data rate for the SoftwareSerial port
  ss_gps.begin(9600);
  ss_radio.begin(9600);

  console.println("Resetting GPS module ...");
  gpsHardwareReset();
  console.println("... done");

  console.println("Starting Wifi/Web ...");
  setupWiFi();
  SetupWebServer();
  SetupDNSServer();

  console.println("... done");

  // Trigger a GPS update every Second
  tGPSUpdate.attach(1, ppsHandler);

  // Print to the Console every 5 seconds
//  tPrintConsole.attach(5, printConsole);
  tPrintAPRS.attach(30, printAPRS);

  // TODO: Print to web clients every X seconds
}

void loop(void)
{
  char rc;
  char endMarker = '\n';

  dnsServer.processNextRequest();
  webServer.handleClient();
  
  if (ppsTriggered) {
    ppsTriggered = false;
    digitalWrite(LED_BUILTIN, gps.location.isValid());
  }

  while (ss_gps.available()) {
#ifdef DEBUG
    rc = ss_gps.read();
    console.write(rc);
    gps.encode(rc);
#else
    gps.encode(ss_gps.read());
#endif
  }

  if (ss_radio.available()) {
    newRadioData = false;
    console.print("RADIO: ");
    while (newRadioData == false) {
      if (ss_radio.available()) {
        rc = ss_radio.read();
        console.write(rc);
        if (rc == endMarker) {
          newRadioData = true;
        }
      }
    }
  }
}