/* Version 1.6

ESP32-S3-WROOM Clock with LCD1602

I2C device found at address 0x27 // LCD

*/


#include <Wire.h>
#include <WIFi.h>
#include <Arduino.h>
#include <ESPTelnet.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include "chars.h"

#define I2C_SDA 10
#define I2C_SCL 11

#define TIMEZONE "MSK-3"                      // local time zone definition (Moscow)
#define TRACE(...) Serial.printf(__VA_ARGS__) // Serial output simplified
#define TLNET(...) telnet.printf(__VA_ARGS__) // Telnet output simplified

#define pinBtn0 17
#define pinBtn1 18
#define pinBtn2 19
#define pinBtn3 20
#define ledBuiltIn 2
#define RGB_LED_PIN 48
#define NUM_RGB_LEDS 1 // number of RGB LEDs (assuming 1 WS2812 LED)

const char *ntpHost0 = "0.ru.pool.ntp.org";
const char *ntpHost1 = "1.ru.pool.ntp.org";
const char *ntpHost2 = "2.ru.pool.ntp.org";

const char *mssid = "Xiaomi_065C";
const char *mpass = "43v3ry0nG";

IPAddress ip;
uint16_t port = 23;

int key0, key1, key2, key3;
int keyArray[4]{key0, key1, key2, key3};
int readButton(bool, bool, bool, bool);

bool initWiFi(const char *, const char *, int, int);
bool isConnected();
void setupTelnet();
void errorMsg(String, bool);
void onTelnetConnect(String ip);
void onTelnetDisconnect(String ip);
void onTelnetReconnect(String ip);
void onTelnetConnectionAttempt(String ip);
void onTelnetInput(String str);

String getTimeStr(int);
void customChar();
void testButton();
void execButton();
void screenClockV0();
void screenClockV1();

void traceHelp();
void wfrxChar(int, int);
void wfInfo();
void wfScan();
void hwInfo();

void setup();
void loop();

ESPTelnet telnet;
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD i2c
Adafruit_NeoPixel rgb_led = Adafruit_NeoPixel(NUM_RGB_LEDS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

bool initWiFi(const char *mssid, const char *mpass,
              int max_tries = 20, int pause = 500)
{
  int i = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  TRACE("Connecting to Wifi:\n");
  WiFi.begin(mssid, mpass);
  do
  {
    delay(pause);
    TRACE(".");
    i++;
  } while (!isConnected() && i < max_tries);
  TRACE("\nConnected!\n");
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  return isConnected();
  wfInfo();
}

bool isConnected()
{
  return (WiFi.status() == WL_CONNECTED);
}

void errorMsg(String error, bool restart = true)
{
  TRACE("%s\n", error);
  if (restart)
  {
    TRACE("Rebooting now...\n");
    delay(2000);
    ESP.restart();
    delay(2000);
  }
}

void setupTelnet()
{
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onInputReceived(onTelnetInput);

  //TRACE("- Telnet: ");
  if (telnet.begin(port))
  {
    TRACE("- Telnet running\n");
  }
  else
  {
    TRACE("- Telnet error.");
    errorMsg("Will reboot...");
  }
}

void onTelnetConnect(String ip)
{
  TRACE("- Telnet: %s connected\n", ip.c_str());
  TLNET("\nWelcome %s\n(Use ^] +q to disconnect)\n", telnet.getIP().c_str());
}

void onTelnetDisconnect(String ip)
{
  TRACE("- Telnet: %s disconnected\n", ip.c_str());
}

void onTelnetReconnect(String ip)
{
  TRACE("- Telnet: %s reconnected\n", ip.c_str());
}

void onTelnetConnectionAttempt(String ip)
{
  TRACE("- Telnet: %s tried to connect\n", ip.c_str());
}

void onTelnetInput(String str)
{
  TLNET("[%s] > %s\n", getTimeStr(0), str);
  // checks for a certain command
  if (str == "ping")
  {
    telnet.println("> pong");
    Serial.println("- Telnet: pong");
  }
  else if (str == "helo")
  {
    telnet.printf("\nHi! I'm ESP32 WROOM Board\n");
    telnet.printf("Sketch of NTP clock, telnet\nand some CLI commands\n");
    telnet.printf(""
                  "Compiled at %s %s\n",
                   __DATE__,  __TIME__);
  }
  else if (str == "wfinfo")
  {
    telnet.printf(R"(
  Connection Details:
  ------------------
  SSID       : %s
  Hostname   : %s
  IP-Address : %s
  Gateway    : %s
  DNS server : %s
  MAC-Address: %s
  RSSI       : %d
  )",
                  WiFi.SSID().c_str(),
                  // WiFi.hostname().c_str(),  // ESP8266
                  WiFi.getHostname(), // ESP32
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str(),
                  WiFi.dnsIP().toString().c_str(),
                  WiFi.macAddress().c_str(),
                  WiFi.RSSI());
    telnet.printf("\n");
  }
  else if (str == "cls")
  {
    telnet.printf("\033[2J\033[H");
  }
  else if (str == "bye")
  {
    telnet.println("> disconnecting you...");
    telnet.disconnectClient();
  }
  else if (str == "help")
  {
    telnet.printf(R"(
    Possible commands are:
    ping - answers "pong"
    helo - greets you
    help - print this help
    wfinfo - WiFi info
    bye - disconnects from server
    cls - clear terminal)");
    telnet.printf("\n");
  }
  else
  {
    TLNET("Command \"%s\" not understood\n", str);
  }
}

void wfInfo()
{
  if (isConnected())
  {
    ip = WiFi.localIP();
    TRACE("\n");
    WiFi.printDiag(Serial);
    TRACE("\n");
    TRACE("WiFi SSID: \t%s\n", mssid);
    TRACE("WiFi RSSI: \t%d dBm\n", WiFi.RSSI());
    TRACE("Local ip: \t%s\n", ip.toString().c_str());
    TRACE("Gateway ip: \t%s\n", WiFi.gatewayIP().toString().c_str());
    TRACE("DNS ip: \t%s\n", WiFi.dnsIP().toString().c_str());
    TRACE("Subnet mask: \t%s\n", WiFi.subnetMask().toString().c_str());
    TRACE("MAC address: \t%s\n", WiFi.macAddress().c_str());
    TRACE("Hostname: \t%s\n", WiFi.getHostname());
    TRACE("\n");
  }
  else
  {
    TRACE("WiFi not connected\n");
  }
}

void wfScan()
{
  TRACE("Scan start\n");
  uint32_t scan_startime = millis();
  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks();
  uint32_t scan_duration = millis() - scan_startime;
  float scan_seconds = scan_duration/1000;
  TRACE("Scan done in %.2f seconds\n", scan_seconds);
  if (n == 0)
  {
    TRACE("no networks found\n");
  }
  else
  {
    TRACE("%d networks found\n", n);
    TRACE("Nr | SSID                             | RSSI | CH | Encryption\n");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      TRACE("%2d", i + 1);
      TRACE(" | ");
      TRACE("%-32.32s", WiFi.SSID(i).c_str());
      TRACE(" | ");
      TRACE("%4ld", WiFi.RSSI(i));
      TRACE(" | ");
      TRACE("%2ld", WiFi.channel(i));
      TRACE(" | ");
      switch (WiFi.encryptionType(i))
      {
      case WIFI_AUTH_OPEN:
        TRACE("open");
        break;
      case WIFI_AUTH_WEP:
        TRACE("WEP");
        break;
      case WIFI_AUTH_WPA_PSK:
        TRACE("WPA");
        break;
      case WIFI_AUTH_WPA2_PSK:
        TRACE("WPA2");
        break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        TRACE("WPA+WPA2");
        break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
        TRACE("WPA2-EAP");
        break;
      case WIFI_AUTH_WPA3_PSK:
        TRACE("WPA3");
        break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
        TRACE("WPA2+WPA3");
        break;
      case WIFI_AUTH_WAPI_PSK:
        TRACE("WAPI");
        break;
      default:
        TRACE("unknown");
      }
      TRACE("\n");
      delay(10);
    }
  }
  TRACE("\n");

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
}

void hwInfo()
{

  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  TRACE("Chip ID: %d\n", chipId);
  TRACE("Chip model: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  TRACE("CPU has %d cores at %dMHz\n", ESP.getChipCores(), ESP.getCpuFreqMHz());
  TRACE("Flash frequency is %dMHz\n", ESP.getFlashChipSpeed() / 1000000);
  TRACE("Flash chip size: %d bytes\n", ESP.getFlashChipSize());

  uint32_t total_internal_memory = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  uint32_t free_internal_memory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  uint32_t largest_contig_internal_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

  TRACE("Free  heap (internal memory) : %d bytes\n", esp_get_free_heap_size());
  TRACE("Total DRAM (internal memory) : %d bytes\n", total_internal_memory);
  TRACE("Free  DRAM (internal memory) : %d bytes\n", free_internal_memory);
  TRACE("Largest free cont. DRAM block: %d bytes\n", largest_contig_internal_block);

  // ESP_LOGI("Memory Info", "Total DRAM (internal memory): %" PRIu32 " bytes", total_internal_memory);
  // ESP_LOGI("Memory Info", "Free DRAM (internal memory): %" PRIu32 " bytes", free_internal_memory);
  // ESP_LOGI("Memory Info", "Largest free contiguous DRAM block: %" PRIu32 " bytes", largest_contig_internal_block);
}

void testButton()
{
  static bool testFlag;
  int testTimer = millis() / 200 % 2;
  if (testTimer == 0 && !testFlag)
  {
    testFlag = true;
    TRACE("BTN: %d, %d, %d, %d \n", keyArray[0], keyArray[1], keyArray[2], keyArray[3]);
  }
  else if (testTimer > 0)
  {
    testFlag = false;
  }
}

void screenClockV0()
{
  lcd.setCursor(0, 0);
  lcd.printf("%s %s", getTimeStr(0), getTimeStr(8));
  lcd.setCursor(0, 1);
  lcd.printf("%s", ip.toString().c_str());
  wfrxChar(15, 1);
  if (telnet.isConnected()) {
    lcd.setCursor(15, 0);
    lcd.print('T');
  } else {
    lcd.setCursor(15, 0);
    lcd.print(' ');
  }
}

void screenClockV1()
{
  lcd.setCursor(0, 0);
  lcd.printf("%s %s", getTimeStr(1), getTimeStr(7));
  lcd.setCursor(0, 1);
  lcd.printf("%s", ip.toString().c_str());
  wfrxChar(15, 1);
  if (telnet.isConnected()) {
    lcd.setCursor(15, 0);
    lcd.print('T');
  } else {
    lcd.setCursor(15, 0);
    lcd.print(' ');
  }
}

int readButton(bool btn0 = !digitalRead(pinBtn0),
               bool btn1 = !digitalRead(pinBtn1),
               bool btn2 = !digitalRead(pinBtn2),
               bool btn3 = !digitalRead(pinBtn3))
{
  static bool flagBtn0, flagBtn1, flagBtn2, flagBtn3;
  static bool flagArray[]{flagBtn0, flagBtn1, flagBtn2, flagBtn3};
  bool btnArray[]{btn0, btn1, btn2, btn3};
  for (int i = 0; i < 4; i++)
  {
    if (btnArray[i] && !flagArray[i])
    {
      flagArray[i] = true;
      keyArray[i]++;
      lcd.clear(); // clearing LCD on every button press
      if (keyArray[i] > 16)
      { // max button interation = 16
        keyArray[i] = 0;
      }
    }
    else if (!btnArray[i] && flagArray[i])
    {
      flagArray[i] = false;
    }
  }
  return keyArray[4];
}

void customChar()
{
  lcd.createChar(0, cChar0);
  lcd.createChar(1, cChar1);
  lcd.createChar(2, cChar2);
  lcd.createChar(3, cChar3);
  lcd.createChar(4, cChar4);
  lcd.createChar(5, cChar5);
}

void wfrxChar(int col, int row)
{
  int rx = 0;
  if (WiFi.RSSI() > -60 && WiFi.RSSI() < 0)
    rx = 4;
  else if (WiFi.RSSI() <= -60)
    rx = 3;
  else if (WiFi.RSSI() <= -80)
    rx = 2;
  else if (WiFi.RSSI() <= -90)
    rx = 1;
  else if (WiFi.RSSI() == 0)
    rx = 5;
  else
    rx = 0;
  lcd.setCursor(col, row);
  lcd.write(rx);
}

void execButton()
{
  static uint32_t key0timer;
  uint32_t period;
  if (millis() >= key0timer + 200)
  {
    switch (keyArray[0])
    {
    case 0:
      screenClockV0();
      break;
    case 1:
      screenClockV1();
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.printf("Btn0.case2");
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.printf("Btn0.case3");
      break;
    case 4:
      keyArray[0] = 0;
      break;
    }
    key0timer = millis();
  }

  switch (keyArray[1])
  {
  case 0:
    lcd.setBacklight(1);
    break;
  case 1:
    lcd.setBacklight(0);
    break;
  case 2:
    keyArray[1] = 0;
    break;
  }

  switch (keyArray[2])
  {
  case 0:
    break;
  case 1:
    rgb_led.setPixelColor(0, rgb_led.Color(255, 0, 0)); // Red
    rgb_led.show();
    break;
  case 2:
    rgb_led.setPixelColor(0, rgb_led.Color(0, 255, 0)); // Green
    rgb_led.show();
    break;
  case 3:
    rgb_led.setPixelColor(0, rgb_led.Color(0, 0, 255)); // Blue
    rgb_led.show();
    break;
  case 4:
    rgb_led.setPixelColor(0, rgb_led.Color(0, 0, 0)); // Off
    rgb_led.show();
    break;
  case 5:
    rgb_led.setPixelColor(0, rgb_led.Color(128, 128, 128));
    rgb_led.show();
  case 6:
    keyArray[2] = 0;
  }

  switch (keyArray[3])
  {
  case 0:
    break;
  case 1:

    break;
  case 2:

    break;
  case 3:

    break;
  case 4:
    keyArray[3] = 0;
    break;
  }
}

void traceHelp()
{
  TRACE("\n");
  TRACE("Avaliable commands are:\n\n");
  TRACE(R"( 
  cls - clears screen
  help - print this help
  time - print current time
  date - print current date
  temp - print temperature
  prss - print pressure
  rssi - print WiFi RSSI
  uname - print chip name
  reboot - restarting MCU
  reset - soft reset MCU
  hwinfo - print chip info
  wfstop - turn off wifi
  wfinit - turn on wifi
  wfinfo - print wifi connectipn detalis
  wfscan - scan wifi networks)");
  TRACE("\n\n");
}

String getTimeStr(int numStr)
{
  time_t tnow = time(nullptr);
  struct tm *timeinfo;
  time(&tnow);
  timeinfo = localtime(&tnow);
  char timeStrRet[32];
  switch (numStr)
  {
  case 0:
    strftime(timeStrRet, 32, "%T", timeinfo); // ISO8601 (HH:MM:SS)
    break;
  case 1:
    strftime(timeStrRet, 32, "%R", timeinfo); // HH:MM
    break;
  case 3:
    strftime(timeStrRet, 32, "%H", timeinfo); // HH
    break;
  case 4:
    strftime(timeStrRet, 32, "%M", timeinfo); // MM
    break;
  case 5:
    strftime(timeStrRet, 32, "%S", timeinfo); // SS
    break;
  case 6:
    strftime(timeStrRet, 32, "%a %d %h", timeinfo); // Thu 23 Aug
    break;
  case 7:
    strftime(timeStrRet, 32, "%d.%m.%C", timeinfo); // 23.08.24
    break;
  case 8:
    strftime(timeStrRet, 32, "%d.%m", timeinfo); // 23.08
    break;
  case 9:
    strftime(timeStrRet, 32, "%c", timeinfo); // Thu Aug 23 14:55:02 2001
    break;
  }

  return timeStrRet;
}


void setup()
{
  pinMode(pinBtn0, INPUT);
  pinMode(pinBtn1, INPUT);
  pinMode(pinBtn2, INPUT);
  pinMode(pinBtn3, INPUT);
  pinMode(ledBuiltIn, OUTPUT);
  digitalWrite(ledBuiltIn, HIGH);

  Serial.begin(115200);
  while (!Serial)
  {
    ;
  };
  delay(200);
  //TRACE("\033[2J\033[H");
  lcd.init(I2C_SDA, I2C_SCL); // initialize LCD(I2C pins)
  lcd.backlight();
  initWiFi(mssid, mpass, 20, 500);
  wfInfo();

  if (isConnected())
  {
    ip = WiFi.localIP();
    TRACE("\n- Telnet: %s:%u\n", ip.toString().c_str(), port);
    setupTelnet();
  }
  else
  {
    TRACE("\n");
    errorMsg("Error connecting to WiFi");
  }

  TRACE("Setting up NTP time...");
  configTzTime(TIMEZONE, ntpHost0, ntpHost1, ntpHost2);
  TRACE("Done.\n");

  for (int i = 0; i < 40; i++)
  {
    TRACE("%c", '*');
    delay(25);
  }
  TRACE("\n");
  String greet = "     Welcome to ESP32-WROOM board!      ";
  for (char letter : greet)
  {
    TRACE("%c", letter);
    delay(25);
  }
  TRACE("\n");
  TRACE("Program built at %s %s\n", __DATE__ , __TIME__);

  for (int i = 0; i < 40; i++)
  {
    TRACE("%c", '*');
    delay(25);
  }
  TRACE("\n");

  customChar();

  for (int i = 0; i < 5; i++)
  {
    lcd.setCursor(15, 0);
    lcd.write(i);
    delay(100);
    lcd.write(char(16));
  }
  lcd.clear();
  TRACE("\nGuru meditates...\nESP32 Ready.\n");
}

void loop(void)
{
  if (isConnected() /* && millis()%2000 < 10 */)
  {
    digitalWrite(ledBuiltIn, HIGH);
  }
  else
  {
    digitalWrite(ledBuiltIn, LOW);
  }

  telnet.loop();
  readButton();
  execButton();
  //testButton();

  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    TRACE("[%s] > %s :\n", getTimeStr(0), cmd);
    if (cmd == "cls")
    {
      TRACE("\033[2J\033[H");
    }
    else if (cmd == "reboot")
    {
      TRACE("ESP restarting\n");
      delay(1000);
      ESP.restart();
    }
    else if (cmd == "reset")
    {
      TRACE("Running Setup\n");
      delay(1000);
      setup();
    }
    else if (cmd == "uname")
    {
      TRACE("SoC is %s\n", ESP.getChipModel());
    }
    else if (cmd == "help")
    {
      traceHelp();
    }
    else if (cmd == "time")
    {
      TRACE("%s\n", getTimeStr(9));
    }
    else if (cmd == "date")
    {
      TRACE("%s\n", getTimeStr(7));
    }
    else if (cmd == "wfstop")
    {
      TRACE("WiFi turned OFF\n");
      WiFi.disconnect();
    }
    else if (cmd == "wfinit")
    {
      TRACE("WiFi turned ON\n");
      initWiFi(mssid, mpass, 20, 500);
      wfInfo();
    }
    else if (cmd == "wfinfo")
    {
      wfInfo();
    }
    else if (cmd == "wfscan")
    {
      wfScan();
    }
    else if (cmd == "rssi")
    {
      TRACE("WiFi RSSI: %d dBm\n", WiFi.RSSI());
      if (WiFi.RSSI() == 0)
      {
        TRACE("[WiFi disconnected]\n");
      }
    }
    else if (cmd == "hwinfo")
    {
      hwInfo();
    }
    else
    {
      TRACE("Command \"%s\" not understood\n", cmd);
    }
  }
}