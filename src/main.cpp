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
#include "cline.h"

#define I2C_SDA 10
#define I2C_SCL 11

#define RXD1 18
#define TXD1 17

#define TIMEZONE "MSK-3"                      // local time zone definition (Moscow)
#define TRACE(...) Serial.printf(__VA_ARGS__) // Serial output simplified
#define TLNET(...) telnet.printf(__VA_ARGS__) // Telnet output simplified

#define pinBtn0 35
#define pinBtn1 36
#define pinBtn2 37
#define pinBtn3 38
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
String client_ip;
const int serial_speed = 115200;
int keyArray[4]{};


bool initWiFi(const char *, const char *, int, int);
bool isConnected();

void setupSerial(int);

void setupTelnet();
void errorMsg(String, bool);
void onTelnetConnect(String ip);
void onTelnetDisconnect(String ip);
void onTelnetReconnect(String ip);
void onTelnetConnectionAttempt(String ip);
void onTelnetInput(String str);

void screenDraw(String, String);
String getTimeStr(int);

int readButton(bool, bool, bool, bool);
void testButton(int);
void execButton();

void indWiFiRx(int, int);
void commSerial();
void infoWiFi(int);
void infoChip(int);
void scanWiFi();

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
  digitalWrite(LED_BUILTIN, LOW);
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
  digitalWrite(LED_BUILTIN, HIGH);
  infoWiFi(5);
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


void setupSerial(int uart_num)
{
  if (uart_num == 0)
  {
    Serial.begin(serial_speed);
    while (!Serial)
    {
      ;
    };
    delay(100);
    TRACE("Serial0 running\n");
    return;
  }
  else if (uart_num == 1)
  {
    Serial1.begin(serial_speed, SERIAL_8N1, RXD1, TXD1);
    while (!Serial1)
    {
      ;
    };
    delay(100);
    TRACE("Serial1 running\n");
    return;
  }
  else
  {
    return;
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
  client_ip = ip.c_str();
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

void onTelnetInput(String comm_telnet)
{
  TLNET("[%s] > %s\n", getTimeStr(0), comm_telnet);
  int comm_case_t = 0;
  for (int i = 1; i < comm_qty; i++)
  {
    if (comm_array[i][0] == comm_telnet)
    {
      comm_case_t = i;
      break;
    }
    else
    {
      comm_case_t = 0;
    }
  }

  switch (comm_case_t)
  {
  case 0:
    TLNET("Command \"%s\" not understood\n", comm_telnet);
    break;
  case 1:
    TLNET("%s %s %s\n",
          comm_about, __DATE__, __TIME__);
    break;
  case 2:
    TLNET("%s", term_clear);
    break;
  case 3:
    TLNET("Hi, %s!\n", client_ip.c_str());
    break;
  case 4:
  {
    TLNET(">pong\n");
    TRACE("- Telnet: >pong\n");
  }
  break;
  case 5:
    for (int k = 1; k < comm_qty; k++)
    {
      TLNET("[ %d ] %s\t- %s\n", k, comm_array[k][0], comm_array[k][1].c_str() );
    }
    break;
  case 6:
    infoWiFi(7);
  break;
  case 7:
    infoChip(7);
  break;
  case 8:
  {
    TLNET("Disconnecting you!\n");
    telnet.disconnectClient();
  }
  break;
  case 9:
    TLNET("%s\n", getTimeStr(0));
    break;
  case 10:
    TLNET("%s\n", getTimeStr(7));
    break;
  case 11:
    TRACE("Turning WiFi on/off unavaliable on telnet session\n");
    break;
  case 12:
    TRACE("Turning WiFi on/off unavaliable on telnet session\n");
    break;
  case 13:
    TRACE("WiFi scan fuction over telnet coming soon\n");
    break;
  case 14:
    TLNET("WiFi RSSI: %d dBm\n", WiFi.RSSI());
    break;
  case 15:
    TLNET("%s\n", ESP.getChipModel());
    break;
  case 16:
    TLNET("ESP restating, you'll be disconnected\n");
    TLNET("Bye, %s\n", client_ip.c_str());
    delay(1000);
    telnet.disconnectClient();
    delay(2000);
    ESP.restart();
    break;
  case 17:
    TLNET("Starting soft reset MCU,\ndon't know where this will lead :/\n");
    setup();
    break;
  case 18:
    TLNET("Uptime counter not implemented yet\n");
    break;
  case 19:
    TLNET("LCD backlight switch not implemented yet\n");
    break;
  case 20:
    if (!Serial) {
      TLNET("Starting setup Serial\n");
      setupSerial(0);
    } else {
      TLNET("Serial is already up!\n");
    }
    break;
  case 21:
    testButton(7);
    break;
  }
}

void infoWiFi(int wifiinfo_out)
{
  if (isConnected())
  {
    if (wifiinfo_out == 5)
    {
      TRACE("\n");
      WiFi.printDiag(Serial);
      TRACE("\n");
      TRACE("WiFi SSID: \t%s\n", mssid);
      TRACE("WiFi RSSI: \t%d dBm\n", WiFi.RSSI());
      TRACE("Local ip: \t%s\n", WiFi.localIP().toString().c_str());
      TRACE("Gateway ip: \t%s\n", WiFi.gatewayIP().toString().c_str());
      TRACE("DNS ip: \t%s\n", WiFi.dnsIP().toString().c_str());
      TRACE("Subnet mask: \t%s\n", WiFi.subnetMask().toString().c_str());
      TRACE("MAC address: \t%s\n", WiFi.macAddress().c_str());
      TRACE("Hostname: \t%s\n", WiFi.getHostname());
      TRACE("\n");
    }
    else if (wifiinfo_out == 7)
    {
      TLNET(R"(
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
            WiFi.getHostname(),
            WiFi.localIP().toString().c_str(),
            WiFi.gatewayIP().toString().c_str(),
            WiFi.dnsIP().toString().c_str(),
            WiFi.macAddress().c_str(),
            WiFi.RSSI());
      TLNET("\n");
    }
  }
  else
  {
    TRACE("WiFi not connected\n");
  }
  wifiinfo_out = 0;
}

void scanWiFi()
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

void infoChip(int chipinfo_out)
{
  uint32_t total_internal_memory = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  uint32_t free_internal_memory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  uint32_t largest_contig_internal_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

  if (chipinfo_out == 5) {
    TRACE("Chip model: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    TRACE("CPU has %d cores at %dMHz\n", ESP.getChipCores(), ESP.getCpuFreqMHz());
    TRACE("Flash frequency is %dMHz\n", ESP.getFlashChipSpeed() / 1000000);
    TRACE("Flash chip size: %d bytes\n", ESP.getFlashChipSize());
    TRACE("Free  heap (internal memory) : %d bytes\n", esp_get_free_heap_size());
    TRACE("Total DRAM (internal memory) : %d bytes\n", total_internal_memory);
    TRACE("Free  DRAM (internal memory) : %d bytes\n", free_internal_memory);
    TRACE("Largest free cont. DRAM block: %d bytes\n", largest_contig_internal_block);
  } else if (chipinfo_out == 7) {
    TLNET("Chip model: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    TLNET("CPU has %d cores at %dMHz\n", ESP.getChipCores(), ESP.getCpuFreqMHz());
    TLNET("Flash frequency is %dMHz\n", ESP.getFlashChipSpeed() / 1000000);
    TLNET("Flash chip size: %d bytes\n", ESP.getFlashChipSize());
    TLNET("Free  heap (internal memory) : %d bytes\n", esp_get_free_heap_size());
    TLNET("Total DRAM (internal memory) : %d bytes\n", total_internal_memory);
    TLNET("Free  DRAM (internal memory) : %d bytes\n", free_internal_memory);
    TLNET("Largest free cont. DRAM block: %d bytes\n", largest_contig_internal_block);
  } else {
    String err_output("No chip info output specified! Arg is: \n");
    TRACE("%s %d", err_output, chipinfo_out);
    TLNET("%s %d", err_output, chipinfo_out);
  }
  chipinfo_out = 0;
}

void testButton(int bttntest_out)
{
    if (bttntest_out == 5) {
      TRACE("BTN: %d, %d, %d, %d \n", keyArray[0], keyArray[1], keyArray[2], keyArray[3]);
    } else if (bttntest_out == 7) {
      TLNET("BTN: %d, %d, %d, %d \n", keyArray[0], keyArray[1], keyArray[2], keyArray[3]);
    } else {
      return;
    }
}

void screenDraw(String lcdRow0, String lcdRow1) {
  lcd.setCursor(0, 0);
  lcd.printf("%s", lcdRow0.c_str());
  lcd.setCursor(0, 1);
  lcd.printf("%s", lcdRow1.c_str());

  indWiFiRx(15, 1);
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

void indWiFiRx(int col, int row)
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
    case 0: {
      String lcdRowStr0(getTimeStr(0) + " " + getTimeStr(8));
      String lcdRowStr1(WiFi.localIP().toString().c_str());
      screenDraw(lcdRowStr0, lcdRowStr1);
    }
      break;
    case 1: {
      String lcdRowStr0(getTimeStr(1) + " " + getTimeStr(7));
      String lcdRowStr1(WiFi.localIP().toString().c_str());
      screenDraw(lcdRowStr0, lcdRowStr1);
    }
      break;
    case 2: {
      String lcdRowStr0("BTN.0 Case 2.R0");
      String lcdRowStr1("BTN.0 Case 2.R1");
      screenDraw(lcdRowStr0, lcdRowStr1);
    }
      break;
    case 3: {
      String lcdRowStr0("BTN.0 Case 3.R0");
      String lcdRowStr1("BTN.0 Case 3.R1");
      screenDraw(lcdRowStr0, lcdRowStr1);
    }
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
    rgb_led.setPixelColor(0, rgb_led.Color(128, 128, 128)); // Off
    rgb_led.show();
    break;
  case 5:
    rgb_led.setPixelColor(0, rgb_led.Color(0, 0, 0));
    rgb_led.show();
  case 6:
    keyArray[2] = 0;
  }

  switch (keyArray[3])
  {
  case 0:
    lcd.setBacklight(1);
    break;
  case 1:
    lcd.setBacklight(0);
    break;
  case 2:
    keyArray[3] = 0;
    break;
  }
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
    strftime(timeStrRet, 32, "%d.%m.%g", timeinfo); // 23.08.24
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

void commSerial() {
  if (Serial.available())
  {
    String comm_serial = Serial.readStringUntil('\n');
    TRACE("[%s] > %s :\n", getTimeStr(0), comm_serial);
    int comm_case_s = 0;
    for (int i = 1; i < comm_qty; i++) 
    { 
      if (comm_array[i][0] == comm_serial) 
      {
        comm_case_s = i;
        break;
      }
      else
      {
        comm_case_s = 0;
      }
    }

    switch (comm_case_s)
    {
      case 0:
        TRACE("%s \"%s\"\n", comm_err, comm_serial);
        break;
      case 1:
        TRACE("%s %s\n", comm_about, build_date);
        break;
      case 2:
        TRACE("%s\n", term_clear);
        break;
      case 3:
        TRACE("%s %s !\n", comm_helo, client_ip.c_str());
        break;
      case 4:
        TRACE("Pong!\n");
        break;
      case 5:
       for (int j = 1; j < comm_qty; j++) {
        TRACE("[ %d ] %s\t- %s\n", j, comm_array[j][0], comm_array[j][1].c_str()); }
        break;
      case 6:
        infoWiFi(5); // 2do: telnet || serial output according to args
        break;
      case 7:
        infoChip(5); // ag int: 5 - to Seial, 7 - to telnet
        break;
      case 8:
        {TRACE("This command will stop serial, proceed? yes/no\n");
        while (!Serial.available()) {
          ;
        }
        String serial_switch = Serial.readStringUntil('\n');
        if (serial_switch == "yes") {
          TRACE("Serial will shut down\n");
          delay(1000);
          Serial.end();
          serial_switch = "";
        } else {
          TRACE("Serial status unchanged\n");
        } 
        TRACE("(command under development)\n");}
        break;
      case 9:
        TRACE("%s\n", getTimeStr(0));
        break;
      case 10:
        TRACE("%s\n", getTimeStr(7));
        break;
      case 11:
        if (!isConnected()) {
          initWiFi(mssid, mpass, 20, 500);
        } else {
          TRACE("WiFi is already connected!\n");
          infoWiFi(5);
        }
        break;
      case 12:
        if (isConnected()) {
          WiFi.disconnect();
          digitalWrite(LED_BUILTIN, LOW);
          TRACE("WiFi disconnected\n");
        } else {
          TRACE("WiFi is already disconnected!\n");
        }
        break;
      case 13:
        scanWiFi();
        break;
      case 14:
        TRACE("WiFi RSSI: %d dBm\n", WiFi.RSSI());
        break;
      case 15:
        TRACE("%s\n", ESP.getChipModel());
        break;
      case 16:
        TRACE("ESP restarting in 3 sec\n");
        static int reboot_time = millis() + 3000;
        while (millis() < reboot_time) {
          TRACE(".");
          delay(200);
        }
        ESP.restart();
        break;
      case 17:
        TRACE("Starting soft reset MCU\n");
        setup();
        break;
      case 18:
        TRACE("Uptime counter not implemented yet\n");
        break;
      case 19:
        TRACE("LCD backlight switch not implemented yet\n");
        break;
      case 20:
        if (Serial) {
          TRACE("Serial is already up!\n");
        } else {
          setupSerial(0);
        }
        break;
      case 21:
        testButton(5);
        break;
    }
  }
}


void setup()
{
  pinMode(pinBtn0, INPUT);
  pinMode(pinBtn1, INPUT);
  pinMode(pinBtn2, INPUT);
  pinMode(pinBtn3, INPUT);
  pinMode(ledBuiltIn, OUTPUT);
  digitalWrite(ledBuiltIn, HIGH);

  setupSerial(0);
  setupSerial(1);

  lcd.init(I2C_SDA, I2C_SCL); // initialize LCD(I2C pins)
  lcd.backlight();
  initWiFi(mssid, mpass, 20, 500);
  infoWiFi(5);

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

  lcd.createChar(0, cChar0);
  lcd.createChar(1, cChar1);
  lcd.createChar(2, cChar2);
  lcd.createChar(3, cChar3);
  lcd.createChar(4, cChar4);
  lcd.createChar(5, cChar5);

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
  telnet.loop();
  readButton();
  execButton();
  commSerial();
}