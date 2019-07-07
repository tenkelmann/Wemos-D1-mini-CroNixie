#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>
#include <Makerblog_TSL45315.h>
#include <Wire.h>
#include "credentials.h"

#define PROJECT_NAME "WEMOS-CRONIXIE2"
#define TIME_ZONE (+2)
#define DEBUG
#ifdef DEBUG
  #define DEBUG_LOG(x) Serial.print(x)
#else
  #define DEBUG_LOG(x)
#endif
#define PIN D3
#define NUMPIXELS 60

time_t getNtpTime();
//void digitalClockDisplay();
//void printDigits(int digits);
void sendNTPpacket(IPAddress &address);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Makerblog_TSL45315 luxsensor = Makerblog_TSL45315(TSL45315_TIME_M4);

WiFiUDP Udp;
unsigned int localPort = 2390;
static const char ntpServerName[] = "time.nist.gov";
int delayval = 1000; // delay for a second
int cronixie[60] = {5, 0, 6, 1, 7, 2, 8, 3, 9, 4, 15, 10, 16, 11, 17, 12, 18, 13, 19, 14, 25, 20, 26, 21, 27, 22, 28, 23, 29, 24, 35, 30, 36, 31, 37, 32, 38, 33, 39, 34, 45, 40, 46, 41, 47, 42, 48, 43, 49, 44, 55, 50, 56, 51, 57, 52, 58, 53, 59, 54};
int sec1 = 5;
int sec2 = 5;
int min1 = 8;
int min2 = 5;
int std1 = 3;
int std2 = 1;
int myMaxRed = 255;         //Maximale Helligkeit rot (0-255)
int myMaxGreen = 140;       //Maximale Helligkeit grün (0-255)
int myMaxBlue = 0;          //Maximale Helligkeit blau (0-255)
uint32_t myBrightness = 50; //Gewünschte Gesamthelligkeit (0-255)
int myRed = 0;
int myGreen = 0;
int myBlue = 0;

void setup()
{
  pixels.begin();
  // Enable our serial port.
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(PROJECT_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  DEBUG_LOG("\nWiFi connecting: ");
  while (WiFi.status() != WL_CONNECTED)
  {
    DEBUG_LOG(".");
    delay(500);
  }

  DEBUG_LOG("\nWiFi connected ");
  DEBUG_LOG(WiFi.localIP());
  DEBUG_LOG("\n");

  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(3600);

  luxsensor.begin();
}

void loop()
{
  myBrightness = luxsensor.readLux();
  DEBUG_LOG(myBrightness);
  DEBUG_LOG(":\t");

  myBrightness = constrain(map(myBrightness, 3, 22000, 2, 255), 0, 255);
  DEBUG_LOG(myBrightness);
  DEBUG_LOG("\n");

  myRed = map(myBrightness, 0, 255, 0, myMaxRed);
  myGreen = map(myBrightness, 0, 255, 0, myMaxGreen);
  myBlue = map(myBrightness, 0, 255, 0, myMaxBlue);

  pixels.setPixelColor(cronixie[sec1 + 50], pixels.Color(0, 0, 0));
  pixels.setPixelColor(cronixie[sec2 + 40], pixels.Color(0, 0, 0));
  pixels.setPixelColor(cronixie[min1 + 30], pixels.Color(0, 0, 0));
  pixels.setPixelColor(cronixie[min2 + 20], pixels.Color(0, 0, 0));
  pixels.setPixelColor(cronixie[std1 + 10], pixels.Color(0, 0, 0));
  pixels.setPixelColor(cronixie[std2], pixels.Color(0, 0, 0));
  //
  // Get the current hour/min
  //
  time_t nw = now();
  int cur_hour = hour(nw);
  int cur_min = minute(nw);
  int cur_sec = second(nw);

  sec2 = (cur_sec % 100) / 10;  // Zehner berechnen
  sec1 = cur_sec % 10;          // Einer berechnen
  min2 = (cur_min % 100) / 10;  // Zehner berechnen
  min1 = cur_min % 10;          // Einer berechnen
  std2 = (cur_hour % 100) / 10; // Zehner berechnen
  std1 = (cur_hour % 10);       // Einer berechnen

  DEBUG_LOG(std2);
  DEBUG_LOG(std1);
  DEBUG_LOG(":");
  DEBUG_LOG(min2);
  DEBUG_LOG(min1);
  DEBUG_LOG(":");
  DEBUG_LOG(sec2);
  DEBUG_LOG(sec1);
  DEBUG_LOG("\t\t(");
  DEBUG_LOG(myRed);
  DEBUG_LOG(", ");
  DEBUG_LOG(myGreen);
  DEBUG_LOG(", ");
  DEBUG_LOG(myBlue);
  DEBUG_LOG(")\n");

  pixels.setPixelColor(cronixie[sec1 + 50], pixels.Color(myRed, myGreen, myBlue));
  pixels.setPixelColor(cronixie[sec2 + 40], pixels.Color(myRed, myGreen, myBlue));
  pixels.setPixelColor(cronixie[min1 + 30], pixels.Color(myRed, myGreen, myBlue));
  pixels.setPixelColor(cronixie[min2 + 20], pixels.Color(myRed, myGreen, myBlue));
  pixels.setPixelColor(cronixie[std1 + 10], pixels.Color(myRed, myGreen, myBlue));
  pixels.setPixelColor(cronixie[std2], pixels.Color(myRed, myGreen, myBlue));
  pixels.show();
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48;     // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP;

  // discard any previously received packets
  while (Udp.parsePacket() > 0)
    ;

  DEBUG_LOG("Initiating NTP sync\n");

  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);

  DEBUG_LOG(ntpServerName);
  DEBUG_LOG(" -> ");
  DEBUG_LOG(ntpServerIP);
  DEBUG_LOG("\n");

  sendNTPpacket(ntpServerIP);

  delay(50);
  uint32_t beginWait = millis();

  while ((millis() - beginWait) < 5000)
  {
    DEBUG_LOG("#");
    int size = Udp.parsePacket();

    if (size >= NTP_PACKET_SIZE)
    {

      DEBUG_LOG("Received NTP Response\n");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);

      unsigned long secsSince1900;

      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];

      // Now convert to the real time.
      unsigned long now = secsSince1900 - 2208988800UL;

#ifdef TIME_ZONE
      DEBUG_LOG("Adjusting time : ");
      DEBUG_LOG(TIME_ZONE);
      DEBUG_LOG("\n");

      now += (TIME_ZONE * SECS_PER_HOUR);
#endif

      return (now);
    }

    delay(50);
  }

  DEBUG_LOG("NTP-sync failed\n");
  return 0;
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision

  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
