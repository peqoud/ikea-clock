/****************************************************************************************************************************
  Async_AutoConnect.ino
  For ESP8266 / ESP32 boards

  IKEA OBERGRANSÃ„D/Frequenz KIT
  (C)2023 DR. ARMIN ZINK
  MIT LICENSE
  parts borrowed from
  By /u/frumperino
  goodwires.org
 *****************************************************************************************************************************/

#if !( defined(ESP8266) ||  defined(ESP32) )
  #error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

#include <AsyncPrinter.h>
#include <DebugPrintMacros.h>
#include <SyncClient.h>
#include <async_config.h>
#include <tcp_axtls.h>

#define CLOCK_VERSION "0.9.1"

// Ports for the WebServices
#define WIFI_SETUP_PORT  (80)
#define OTA_PORT_PORT    (8080)
#define WEBSERIAL_PORT   (8081)

// UNCOMMENT FOR FREQUENZ
#define H_FREKVENS
// UNCOMMENT FOR OBEGRÃ„NSAD
// #define H_OBEGRANSAD

#define MAX_X  (16)
#define MAX_Y  (16)


#include <Arduino.h>

#include <pgmspace.h>

// WebServer Stuff
#include <ESP8266WiFi.h>
#include <ESPAsyncDNSServer.h>
#include <ESPAsyncWebServer.h>

// WifiManager
#define USE_EADNS   // use ESPAsyncDNSServer
#include <ESPAsyncWiFiManager.h>       //  https://github.com/alanswx/ESPAsyncWiFiManager/blob/master/examples/AutoConnect/AutoConnect.ino

const byte DNS_PORT =  53;
AsyncDNSServer    dnsServer;
AsyncWebServer    wifi_server(WIFI_SETUP_PORT);
AsyncWiFiManager  wifiManager(&wifi_server,&dnsServer);

// End of WifiManager

// For the OTA Update
#include <ElegantOTA.h> //  https://docs.elegantota.pro/getting-started/

const char* host = "ikea-clock-update-webupdate";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";

AsyncWebServer  OtaServer(OTA_PORT_PORT);

// Webserial
 #include <WebSerialLite.h>

 AsyncWebServer webserialserver(WEBSERIAL_PORT);
 // Webserial End


// TAKEN FROM PUBSUB MQTT EXAMPLE
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);

// SPI of D1 Mini Pro
// M-CLK => D5
// MISO => D6
// MOSI => D7
// --------------------

//                  OBEGRÃ„NSAD ARIKEL       FREKVENS ARTIKEL
#define P_EN  D5   // ORANGE                  ROSA
#define P_DI  D6   // GELB                    GELB
#define P_CLK D7   // BLAU                    LILA
#define P_CLA D8   // LILA                    BLUE

#define P_KEY_YELLOW D2         // white wire - yellow button
#define P_KEY_RED    D1         // black wire - RED BUTTON


bool mqttenable = false;

// SET YOUR TIMEZONE HERE
#define MY_NTP_SERVER "pool.ntp.org"            // set the best fitting NTP server (pool) for your location
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

long    mil;
int     brightness = 220;
uint8_t sec;
uint8_t minute;
uint8_t hour;
time_t  now;  // this is the epoch
tm 		  tm;


// Waiting Time for Shift Cycke, can go downto 1
// Text Speed
#define SCROLL_SPEED 35


//
#define MQTTSERVER "01.01.100.100"
#define MQTTPORT "1883"
#define MQTTTOPIC1 "dust_meter/pm25"
#define MQTTTOPIC2 "dust_meter/pm10"
String _DisplayLine1 = "";
String _DisplayLine2 = "";
uint8_t mqttretry = 0;


static const uint8_t System6x7[] PROGMEM = {

  // Fixed width; char width table not used !!!!
  // FIRST 32 Characters omitted (non printable)

  // font data
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // (space)
  0x00, 0x00, 0x4f, 0x4f, 0x00, 0x00,
  0x00, 0x00, 0x03, 0x00, 0x03, 0x00,
  0x00, 0x0a, 0x3f, 0x0a, 0x3f, 0x0a,
  0x24, 0x2a, 0x7f, 0x2a, 0x7f, 0x12,
  0x23, 0x33, 0x18, 0x0c, 0x66, 0x62,
  0x3e, 0x3f, 0x6d, 0x6a, 0x30, 0x48,
  0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
  0x00, 0x1c, 0x3e, 0x63, 0x63, 0x00,
  0x00, 0x00, 0x63, 0x63, 0x3e, 0x1c,
  0x2a, 0x1c, 0x7f, 0x7f, 0x1c, 0x2a,
  0x08, 0x08, 0x3e, 0x3e, 0x08, 0x08,
  0x00, 0x00, 0xc0, 0xe0, 0x60, 0x00,
  0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
  0x00, 0x00, 0x60, 0x60, 0x00, 0x00,
  0x20, 0x30, 0x18, 0x0c, 0x06, 0x02,
  0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x3e,  // 0
  0x00, 0x02, 0x7f, 0x7f, 0x00, 0x00,  // 1
  0x62, 0x73, 0x7b, 0x6b, 0x6f, 0x66,  // 2
  0x22, 0x63, 0x6b, 0x6b, 0x7f, 0x36,  // 3
  0x0f, 0x0f, 0x08, 0x08, 0x7f, 0x7f,  // 4
  0x2f, 0x6f, 0x6b, 0x6b, 0x7b, 0x3b,  // 5
  0x3e, 0x7f, 0x6b, 0x6b, 0x7b, 0x3a,  // 6
  0x03, 0x03, 0x7b, 0x7b, 0x0f, 0x07,  // 7
  0x36, 0x7f, 0x6b, 0x6b, 0x7f, 0x36,  // 8
  0x26, 0x6f, 0x6b, 0x6b, 0x7f, 0x3e,  // 9
  0x00, 0x00, 0x36, 0x36, 0x00, 0x00,
  0x00, 0x00, 0x76, 0x36, 0x00, 0x00,
  0x00, 0x18, 0x3c, 0x66, 0x42, 0x00,
  0x00, 0x00, 0x36, 0x36, 0x36, 0x36,
  0x00, 0x00, 0x42, 0x66, 0x3c, 0x18,
  0x00, 0x02, 0x5b, 0x5b, 0x0f, 0x06,
  0x3c, 0x42, 0x4a, 0x56, 0x5c, 0x00,
  0x7e, 0x7f, 0x0b, 0x0b, 0x7f, 0x7e,
  0x7f, 0x7f, 0x6b, 0x6b, 0x7f, 0x36,
  0x3e, 0x7f, 0x77, 0x63, 0x63, 0x22,
  0x7f, 0x7f, 0x63, 0x63, 0x7f, 0x3e,
  0x7f, 0x7f, 0x6b, 0x6b, 0x63, 0x63,
  0x7f, 0x7f, 0x0b, 0x0b, 0x03, 0x03,
  0x3e, 0x7f, 0x63, 0x6b, 0x7b, 0x32,
  0x7f, 0x7f, 0x08, 0x08, 0x7f, 0x7f,
  0x00, 0x00, 0x7f, 0x7f, 0x00, 0x00,
  0x00, 0x23, 0x63, 0x63, 0x7f, 0x3f,
  0x7f, 0x7f, 0x1c, 0x36, 0x63, 0x41,
  0x7f, 0x7f, 0x60, 0x60, 0x60, 0x60,
  0x7f, 0x7f, 0x03, 0x06, 0x03, 0x7f,
  0x7f, 0x7f, 0x0e, 0x18, 0x7f, 0x7f,
  0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x3e,
  0x7f, 0x7f, 0x0b, 0x0b, 0x0f, 0x06,
  0x3e, 0x7f, 0x63, 0x73, 0x3f, 0x5e,
  0x7f, 0x7f, 0x1b, 0x3b, 0x7f, 0x6e,
  0x26, 0x6f, 0x6b, 0x6b, 0x7b, 0x32,
  0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03,
  0x3f, 0x7f, 0x60, 0x60, 0x7f, 0x3f,
  0x0f, 0x7f, 0x78, 0x40, 0x7f, 0x0f,
  0x3f, 0x7f, 0x60, 0x78, 0x60, 0x3f,
  0x63, 0x77, 0x1c, 0x1c, 0x77, 0x63,
  0x07, 0x0f, 0x78, 0x78, 0x0f, 0x07,
  0x63, 0x73, 0x7b, 0x6f, 0x67, 0x63,
  0x7d, 0x7e, 0x0b, 0x0b, 0x7e, 0x7d,
  0x3d, 0x7e, 0x66, 0x66, 0x7e, 0x3d,
  0x3d, 0x7d, 0x60, 0x60, 0x7d, 0x3d,
  0x00, 0x04, 0x02, 0x01, 0x02, 0x04,
  0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
  0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
  0x7e, 0x7f, 0x0b, 0x0b, 0x7f, 0x7e,
  0x7f, 0x7f, 0x6b, 0x6b, 0x7f, 0x36,
  0x3e, 0x7f, 0x77, 0x63, 0x63, 0x22,
  0x7f, 0x7f, 0x63, 0x63, 0x7f, 0x3e,
  0x7f, 0x7f, 0x6b, 0x6b, 0x63, 0x63,
  0x7f, 0x7f, 0x0b, 0x0b, 0x03, 0x03,
  0x3e, 0x7f, 0x63, 0x6b, 0x7b, 0x32,
  0x7f, 0x7f, 0x08, 0x08, 0x7f, 0x7f,
  0x00, 0x00, 0x7f, 0x7f, 0x00, 0x00,
  0x00, 0x23, 0x63, 0x63, 0x7f, 0x3f,
  0x7f, 0x7f, 0x1c, 0x36, 0x63, 0x41,
  0x7f, 0x7f, 0x60, 0x60, 0x60, 0x60,
  0x7f, 0x7f, 0x03, 0x06, 0x03, 0x7f,
  0x7f, 0x7f, 0x0e, 0x18, 0x7f, 0x7f,
  0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x3e,
  0x7f, 0x7f, 0x0b, 0x0b, 0x0f, 0x06,
  0x3e, 0x7f, 0x63, 0x73, 0x3f, 0x5e,
  0x7f, 0x7f, 0x1b, 0x3b, 0x7f, 0x6e,
  0x66, 0x6f, 0x6b, 0x6b, 0x7b, 0x32,
  0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03,
  0x3f, 0x7f, 0x60, 0x60, 0x7f, 0x3f,
  0x0f, 0x7f, 0x78, 0x40, 0x7f, 0x0f,
  0x3f, 0x7f, 0x60, 0x78, 0x60, 0x3f,
  0x63, 0x77, 0x1c, 0x1c, 0x77, 0x63,
  0x07, 0x0f, 0x78, 0x78, 0x0f, 0x07,
  0x63, 0x73, 0x7b, 0x6f, 0x67, 0x63,
  0x7d, 0x7e, 0x0b, 0x0b, 0x7e, 0x7d,
  0x3d, 0x7e, 0x66, 0x66, 0x7e, 0x3d,
  0x3d, 0x7d, 0x60, 0x60, 0x7d, 0x3d,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00

};


// LUT For OBEGRÃ„NSAD X,Y POSITIONS
int lut[MAX_X][MAX_Y] = {
  { 23  , 22  , 21  , 20  , 19  , 18  , 17  , 16  , 7   , 6   , 5   , 4   , 3   , 2   , 1   , 0 },
  { 24  , 25  , 26  , 27  , 28  , 29  , 30  , 31  , 8   , 9   , 10  , 11  , 12  , 13  , 14  , 15 },
  { 39  , 38  , 37  , 36  , 35  , 34  , 33  , 32  , 55  , 54  , 53  , 52  , 51  , 50  , 49  , 48 },
  { 40  , 41  , 42  , 43  , 44  , 45  , 46  , 47  , 56  , 57  , 58  , 59  , 60  , 61  , 62  , 63 },
  { 87  , 86  , 85  , 84  , 83  , 82  , 81  , 80  , 71  , 70  , 69  , 68  , 67  , 66  , 65  , 64 },
  { 88  , 89  , 90  , 91  , 92  , 93  , 94  , 95  , 72  , 73  , 74  , 75  , 76  , 77  , 78  , 79 },
  { 103 , 102 , 101 , 100 , 99  , 98  , 97  , 96  , 119 , 118 , 117, 116  , 115, 114  , 113 , 112 },
  { 104 , 105 , 106 , 107 , 108 , 109 , 110 , 111 , 120 , 121 , 122, 123  , 124, 125  , 126 , 127 },
  { 151 , 150 , 149 , 148 , 147 , 146 , 145 , 144 , 135 , 134 , 133, 132  , 131, 130  , 129 , 128 },
  { 152 , 153 , 154 , 155 , 156 , 157 , 158 , 159 , 136 , 137 , 138, 139  , 140, 141  , 142 , 143 },
  { 167 , 166 , 165 , 164 , 163 , 162 , 161 , 160 , 183 , 182 , 181, 180  , 179, 178  , 177 , 176 },
  { 168 , 169 , 170 , 171 , 172 , 173 , 174 , 175 , 184 , 185 , 186, 187  , 188, 189  , 190 , 191 },
  { 215 , 214 , 213 , 212 , 211 , 210 , 209 , 208 , 199 , 198 , 197, 196  , 195, 194  , 193 , 192 },
  { 216 , 217 , 218 , 219 , 220 , 221 , 222 , 223 , 200 , 201 , 202, 203  , 204, 205  , 206 , 207 },
  { 231 , 230 , 229 , 228 , 227 , 226 , 225 , 224 , 247 , 246 , 245, 244  , 243, 242  , 241 , 240 },
  { 232 , 233 , 234 , 235 , 236 , 237 , 238 , 239 , 248 , 249 , 250, 251  , 252, 253  , 254 , 255 }
};

unsigned short _pLatch;
unsigned short _pClock;
unsigned short _pData;
uint8_t p_buf[MAX_X * MAX_Y];


// -----------------------------------------------------------
void p_init(int p_latch, int p_clock, int p_data) {
  _pLatch = p_latch;
  _pClock = p_clock;
  _pData  = p_data;
  pinMode(_pLatch, OUTPUT);
  pinMode(_pClock, OUTPUT);
  pinMode(_pData, OUTPUT);
}

// -----------------------------------------------------------

// Clear the Panel Buffer
void p_clear() {
  for (int i = 0; i < 256; i++) {
    p_buf[i] = 0;
  }
}

// -----------------------------------------------------------
// SCAN DISPLAY, output Bytes to Serial
void p_scan(uint8_t cmask) {

  // TODO: Convert this to SPI if the LATCH is actually Chip-Select
 

  // updated 2023-12-23
  uint8_t w = 0;
  for (int i = (MAX_X * MAX_Y); i > 0; i--) {
    digitalWrite(_pData, cmask & p_buf[w++]);   // Output HIGH or LOW to the DI input of the Display
    // 1 Clock Tick
    digitalWrite(_pClock, HIGH);
    digitalWrite(_pClock, LOW);
  }

  
  // After all Bits are set - latch the values with 1 tick
  digitalWrite(_pLatch, HIGH);
  digitalWrite(_pLatch, LOW);
}


// -----------------------------------------------------------
void p_drawPixel(int8_t x, int8_t y, uint8_t color) 
{

  if ((x < MAX_X) && (y < MAX_Y))
  {
#ifdef H_OBEGRANSAD
    p_buf[lut[y][x]] = color;
#endif

#ifdef H_FREKVENS
    if (x > 7) {
      y += 0x10;
      x &= 0x07;
    }
    p_buf[(y * 8 + x)] = color;
#endif
  }
}

// -----------------------------------------------------------
void p_fillScreen(uint8_t col) {
  for (uint8_t x = 0; x < 16; x++)
    for (uint8_t y = 0; y < 16; y++)
      p_drawPixel(x, y, col);
}

// -----------------------------------------------------------
void test_display() {

  for (int i = 0; i < 2; i++) {
    p_fillScreen(0xff);
    p_scan(1);
    analogWrite(P_EN, 250);
    delay(300);
    p_fillScreen(0x00);
    p_scan(1);
    analogWrite(P_EN, 250);
    delay(300);
  }
}

// -----------------------------------------------------------
void p_printChar(uint8_t xs, uint8_t ys, char ch) {
  uint8_t d;

  for (uint8_t x = 0; x < 6; x++) {
    d = pgm_read_byte_near((ch - 32) * 6 +  // Buchstabennummer (ASCII ) minus 32 da die ersten 32 Zeichen nicht im Font sind
                           x +              // jede Spalte
                           System6x7);      // Adrress of Font

    if ((d & 1) == 1) p_drawPixel(x + xs, 0 + ys, 0xFF);
    else p_drawPixel(x + xs, 0 + ys, 0);
    if ((d & 2) == 2) p_drawPixel(x + xs, 1 + ys, 0xFF);
    else p_drawPixel(x + xs, 1 + ys, 0);
    if ((d & 4) == 4) p_drawPixel(x + xs, 2 + ys, 0xFF);
    else p_drawPixel(x + xs, 2 + ys, 0);
    if ((d & 8) == 8) p_drawPixel(x + xs, 3 + ys, 0xFF);
    else p_drawPixel(x + xs, 3 + ys, 0);
    if ((d & 16) == 16) p_drawPixel(x + xs, 4 + ys, 0xFF);
    else p_drawPixel(x + xs, 4 + ys, 0);
    if ((d & 32) == 32) p_drawPixel(x + xs, 5 + ys, 0xFF);
    else p_drawPixel(x + xs, 5 + ys, 0);
    if ((d & 64) == 64) p_drawPixel(x + xs, 6 + ys, 0xFF);
    else p_drawPixel(x + xs, 6 + ys, 0);
  }
}
// -----------------------------------------------------------

uint8_t p_getPixel(int8_t x, int8_t y) {

#ifdef H_OBERANSAD
  return p_buf[lut[y][x]];
#endif
#ifdef H_FREKVENS
  if (x > 7) {
    y += 0x10;
    x &= 0x07;
  }
  return p_buf[(y * 8 + x)];
#endif
}

// -----------------------------------------------------------

void p_scroll()
{
  for (uint8_t x = 1; x < MAX_X; x++)
    for (uint8_t y = 0; y < MAX_Y; y++)
      p_drawPixel(x - 1, y, p_getPixel(x, y));
  for (uint8_t y = 0; y < MAX_Y; y++)
    p_drawPixel(15, y, 0);
}
// -----------------------------------------------------------

void p_scrollChar(int8_t xs, uint8_t ys, char ch) {
  uint8_t d;

  p_scroll();

  p_scan(1);  // manual scan here !
  delay(SCROLL_SPEED);
  for (uint8_t x = 0; x < 6; x++) {
    p_scroll();  // wait to scan here
    d = pgm_read_byte_near(System6x7 + (ch - 32) * 6 + x);

    if ((d & 1) == 1) p_drawPixel(xs, 0 + ys, 0xFF);
    else p_drawPixel(xs, 0 + ys, 0x00);
    if ((d & 2) == 2) p_drawPixel(xs, 1 + ys, 0xFF);
    else p_drawPixel(xs, 1 + ys, 0x00);
    if ((d & 4) == 4) p_drawPixel(xs, 2 + ys, 0xFF);
    else p_drawPixel(xs, 2 + ys, 0x00);
    if ((d & 8) == 8) p_drawPixel(xs, 3 + ys, 0xFF);
    else p_drawPixel(xs, 3 + ys, 0x00);
    if ((d & 16) == 16) p_drawPixel(xs, 4 + ys, 0xFF);
    else p_drawPixel(xs, 4 + ys, 0x00);
    if ((d & 32) == 32) p_drawPixel(xs, 5 + ys, 0xFF);
    else p_drawPixel(xs, 5 + ys, 0x00);
    if ((d & 64) == 64) p_drawPixel(xs, 6 + ys, 0xFF);
    else p_drawPixel(xs, 6 + ys, 0x00);
    p_scan(1);
    delay(SCROLL_SPEED);
  }
  p_scroll();
  p_scan(1);  // manual scan here !
  delay(SCROLL_SPEED);
}

// -----------------------------------------------------------

void p_scrollText(int8_t xs, uint8_t ys, String s) {
  Serial.print("Scrolltext: ");
  Serial.println(s);
  for (int i = 0; i < s.length(); i++) {
    p_scrollChar(xs, ys, s[i]);
  }
}


// -----------------------------------------------------------
const char* getTimeString(void) {
  static char acTimeString[32];
  time_t now = time(nullptr);
  ctime_r(&now, acTimeString);
  size_t stLength;
  while (((stLength = strlen(acTimeString))) && ('\n' == acTimeString[stLength - 1])) {
    acTimeString[stLength - 1] = 0;  // Remove trailing line break...
  }
  return acTimeString;
}

// -----------------------------------------------------------
void get_ntp_time() {
  int retry_count = 0;
  configTime(MY_TZ, MY_NTP_SERVER);  // --> Here is the IMPORTANT ONE LINER needed in your sketch!
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);   // Secs since 01.01.1970 (when uninitialized starts with (8 * 3600 = 28800)
  while ((now < 8 * 3600 * 2) && (retry_count < 40))
  {  // Wait for realistic value
    delay(25);
    Serial.printf(".");  // EEB: Keep the serial print to serve the watchdog unt NTP time is aquired
    now = time(nullptr);
    retry_count++;
  }

  // NTP connection timeout
  if (retry_count == 100)
  {
    Serial.printf("NTP connection to Server %s timeout.", MY_NTP_SERVER);    
  }

  Serial.printf("\n\nCurrent time: %s\n\n", getTimeString());

}

// -----------------------------------------------------------
void set_clock_from_tm() {
  time(&now);              // read the current time
  localtime_r(&now, &tm);  // update the structure tm with the current time

  // update time from struct
  minute  = tm.tm_min;
  hour    = tm.tm_hour;
  sec     = tm.tm_sec;
}

// -----------------------------------------------------------
void print_time() {
  p_clear();
  p_printChar(2, 0, (tm.tm_hour / 10) + 48);
  p_printChar(9, 0, (tm.tm_hour % 10) + 48);
  p_printChar(2, 9, (tm.tm_min / 10) + 48);
  p_printChar(9, 9, (tm.tm_min % 10) + 48);
  p_scan(1);  // refreshes display
}

// -----------------------------------------------------------


void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}




void onOTAProgress(size_t current, size_t final) {
  static unsigned long ota_progress_millis = 0;
  uint32 progress_per;

  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();

    progress_per = (current* 100) / final;

    Serial.printf("OTA Progress Current: %u %%  (%u / %u bytes)\n", progress_per, current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}


void setup() {
  
  Serial.begin(115200);  // Native Baud Rate of ESP  115200
  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level
  Serial.println();
  Serial.printf("Booting Clock Version V%s\n", CLOCK_VERSION);
  // Local IP for later usage
  IPAddress ip_addr;      // the IP address 

  pinMode(P_KEY_RED,    INPUT_PULLUP);   // RED KEY
  pinMode(P_KEY_YELLOW, INPUT_PULLUP);   // YELLOW KEY
  pinMode(P_EN, OUTPUT);          // Pseudo Analog out for FM Brightess
  analogWrite(P_EN, brightness);  // adjust brightness

  p_init(P_CLA, P_CLK, P_DI);  // init Display

  test_display();

  p_clear();
  p_printChar(2, 0, 'A');  // Print "AP" while waiting for WiFi Manager
  p_printChar(9, 0, 'P');
  p_scan(1);
  delay(1000);


  // WifiManager Setup
  wifiManager.setMinimumSignalQuality(50);
  bool res;
#ifdef H_FREKVENS
  res = wifiManager.autoConnect("Y-CLOCK");
#endif
#ifdef H_OBEGRANSAD
  res = wifiManager.autoConnect("X-CLOCK");
#endif

  /*
// enable fo use without wifi manager
// start network
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
*/
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Local IP for later usage
  ip_addr = WiFi.localIP();      // the IP address 

  p_printChar(2, 9, 'O');
  p_printChar(9, 9, 'K');
  p_scan(1);
  delay(500);

  //if you get here you have connected to the WiFi
  Serial.println("Wifi connected!");
  // Sync clock
  get_ntp_time();
  set_clock_from_tm();
  mqtt_setup();
  print_time();

  // Setup OTA
  ElegantOTA.begin(&OtaServer);  // Start ElegantOTA
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  OtaServer.begin();
  Serial.println("HTTP server started");
  Serial.printf("HTTPUpdateServer ready! Open http://%s:%s/update in your browser\n",ip_addr.toString().c_str(), String(OTA_PORT_PORT));

  // WebSerial is accessible at "<IP Address>/webserial" in browser
  Serial.println();
  Serial.printf("WebSerial is accessible at http://%s:%s/webserial\n", ip_addr.toString().c_str(), String(WEBSERIAL_PORT));
  
 

  WebSerial.begin(&webserialserver);
  WebSerial.onConnect(webserialconnect);  // Connect
  /* Attach Message Callback */
  WebSerial.onMessage(recvMsg);
  webserialserver.begin();
  
  // Start DNS
  dnsServer.stop();  // Stop any previous dnsServer from the WifiManager
  dnsServer.start(DNS_PORT, "yclock.local", ip_addr);


  byte mac[6];                     // the MAC address of your Wifi shield
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
}

// -----------------------------------------------------------


/* Message callback of WebSerial */
void webserialconnect(AsyncWebSocketClient *client)
{
  WebSerial.print("Clock Version: ");
  WebSerial.println(CLOCK_VERSION);
  WebSerial.print("Current time: ");
  WebSerial.println(getTimeString());
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);

  if (d.equals("reset"))
  {
      //reset settings - for testing
      wifiManager.resetSettings();
      WebSerial.println("Reset WIFI Settings.");
  }
}


void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message arrived [");
  Serial.println(topic);
  Serial.print("] ");

  if (strcmp(topic, MQTTTOPIC1) == 0) {
    _DisplayLine1 = "  ";
    for (int i = 0; i < length; i++) { _DisplayLine1 += (char)payload[i]; }
    p_scrollText(15, 0, _DisplayLine1 + "   ");
  }
  if (strcmp(topic, MQTTTOPIC2) == 0) {
    _DisplayLine2 = "  ";
    for (int i = 0; i < length; i++) { _DisplayLine2 += (char)payload[i]; }
    p_scrollText(15, 8, _DisplayLine2 + "   ");
  }
  print_time();
}

// -----------------------------------------------------------

void mqtt_setup() {
  if (mqttenable) {
    p_scrollText(15, 0, "  MQTT ");
    p_scrollText(15, 0, MQTTSERVER);
    p_scrollText(15, 0, "/");
    p_scrollText(15, 0, MQTTTOPIC1);
    p_scrollText(15, 0, "   ");

    Serial.println("MQTT SETUP");
    Serial.println(MQTTSERVER);
    Serial.println(atoi(MQTTPORT));

    client.setServer(MQTTSERVER, atoi(MQTTPORT));
    client.setCallback(mqtt_callback);
  }
}

// -----------------------------------------------------------

void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!client.connected() && (mqttenable)) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      if (strcmp(MQTTTOPIC1, "")) client.subscribe(MQTTTOPIC1);
      if (strcmp(MQTTTOPIC2, "")) client.subscribe(MQTTTOPIC2);


    } else {
      mqttretry++;
      if (mqttretry > 2) {
        mqttenable = false;
        p_scrollText(15, 0, " MQTT ERROR, MQTT DISABLED!   ");
      }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// -----------------------------------------------------------
// -----------------------------------------------------------

// -----------------------------------------------------------
/// MAIN LOOP

void loop() {

    static uint8 led_blink = 1;

  // JEDE SEKUNDE
  if (millis() > mil + 1000) {
    mil = millis();
    set_clock_from_tm();
    print_time();      // Update Display Seconds

    if (led_blink == 1)
    {
      Serial.printf("LED OFF");
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
      led_blink = 0;
    }
    else
    {
      Serial.printf("LED ON");
      digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level  
      led_blink = 1;
    }

    // Jede Stunde
    if ((tm.tm_sec == 0) && (tm.tm_min == 0)) {
      get_ntp_time();  // Sync NTP time after 1 hour
      Serial.printf("Current time: %s\n", getTimeString());
    }
  }

  // Yellow Button pressed -   Helligkeit einstellen. Max = 0, Min = 254
  if (digitalRead(P_KEY_YELLOW) == 0) {
    if (brightness > 205) brightness = 104;  // 104,154,204,254
    brightness += 25;

    analogWrite(P_EN, brightness);
    print_time();
    delay(500);

    Serial.print("Brightness:");
    Serial.println(brightness);

    WebSerial.print("Brightness: ");
    WebSerial.println(brightness);

  // Button Debounce
    delay(100);
  }

  // Red Button pressed
  if (digitalRead(P_KEY_RED) == 0) {

    Serial.println("Red Button Pressed");
    WebSerial.println("Red Button Pressed");
    // Button Debounce
    delay(100);
  }
    

  if (mqttenable) {
    if (!client.connected()) {
      mqtt_reconnect();
      mqttretry++;
      if (mqttretry > 2) {
        mqttenable = false;
        p_scrollText(15, 0, " MQTT ERROR, MQTT DISABLED!  ");
      }
    }
    client.loop();
  }

  // For the OTA Update
  ElegantOTA.loop();
}