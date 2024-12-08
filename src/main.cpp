/* Version 1.6

ESP32-S3-WROOM Clock with LCD1602

I2C device found at address 0x27 // LCD

*/


#include <Wire.h>
#include <WIFi.h>
#include <SHT85.h>
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

#define PIN_BTN_0 35
#define PIN_BTN_1 36
#define PIN_BTN_2 37
#define PIN_BTN_3 38
#define PIN_BEEP 8
#define PIN_RGBLED 48
#define NUM_RGB_LEDS 1 // number of RGB LEDs (assuming 1 WS2812 LED)
#define SHT85_ADDRESS 0x44

const char *ntpHost0 = "0.ru.pool.ntp.org";
const char *ntpHost1 = "1.ru.pool.ntp.org";
const char *ntpHost2 = "2.ru.pool.ntp.org";

const char *mssid = "Xiaomi_065C";
const char *mpass = "43v3ry0nG";

const int serial_speed = 115200;
const uint16_t port = 23;
IPAddress ip;

String client_ip;

bool backLight = true;
String boot_date;
time_t boot_time;


bool initWiFi(const char *, const char *, int, int);
bool isWiFiOn();

void setupSerial(int);
void setupTelnet();

void onTelnetConnect(String ip);
void onTelnetDisconnect(String ip);
void onTelnetReconnect(String ip);
void onTelnetConnectionAttempt(String ip);
void onTelnetInput(String str);
void errorMsg(String, bool);

void readSerial();
String commHandler(String);

int* buttonsRead();
void buttonsExec(int*);

void screenDraw(String, String);
void rssiWiFi(int, int);
void lcdBackLight(bool);
void signalBeep(int);
void signalBuzz(int, int, int);
void readSensor(int);

String getTimeStr(int);
String getSensVal(char);
String uptimeCount();
String testSensor();
String infoChip();
String infoWiFi();
String scanWiFi();

ESPTelnet telnet;
SHT85 sht(SHT85_ADDRESS);
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD i2c
Adafruit_NeoPixel rgb_led = Adafruit_NeoPixel(NUM_RGB_LEDS, PIN_RGBLED, NEO_GRB + NEO_KHZ800);


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
  } while (!isWiFiOn() && i < max_tries);
  WiFi.setSleep(WIFI_PS_NONE);
  TRACE("\nConnected!\n");
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  return isWiFiOn();
}

bool isWiFiOn()
{
  return (WiFi.status() == WL_CONNECTED);
}

String infoWiFi() {
  String wfinf("");
  if (isWiFiOn()) {
    wfinf += "WiFi SSID: ";
    wfinf += WiFi.SSID();
    wfinf += " (ch ";
    wfinf += WiFi.channel();
    wfinf += ")\n";
    wfinf += "WiFi RSSI: ";
    wfinf += WiFi.RSSI();
    wfinf += "dBm\n";
    wfinf += "Local ip: ";
    wfinf += WiFi.localIP().toString();
    wfinf += '\n';
  } else {
    wfinf += "WiFi not connected\n";
  }
  return wfinf;
}

String scanWiFi() {
  String scanResult(' ');
  uint32_t scan_start = millis();
  int n = WiFi.scanNetworks();
  uint32_t scan_elapse = millis() - scan_start;
  if (n == 0) {
   scanResult += "No networks found";
  } else {
  scanResult += n;
  scanResult += " networks found for ";
  scanResult += scan_elapse;
  scanResult += "ms at ";
  scanResult += getTimeStr(0).c_str();
  scanResult += '\n';
  int lengthMax = 0;
  for (int l = 0; l < n; l++) {
    if (WiFi.SSID(l).length() >= lengthMax) {
      lengthMax = WiFi.SSID(l).length();
    }
  }
  scanResult += " Nr |";
  scanResult +=  " SSID";
  String spaceStr = "";
  for (int m = 0; m < lengthMax - 3; m++) {
    spaceStr += ' ';
  }
  scanResult += spaceStr;
  scanResult += "|RSSI | CH | Encrypt\n";
  for (int i = 0; i < n; i++) {
    scanResult += ' ';
    if (i < 9) {
      scanResult += '0';
    }
    scanResult += i + 1;
    scanResult += " | ";
    int lengthAdd = lengthMax - WiFi.SSID(i).length();
    String cmStr = "";
    for (int k = 0; k < lengthAdd; k++) {
      cmStr += ' ';
    }
    scanResult += WiFi.SSID(i).c_str();
    scanResult += cmStr;
    scanResult += " | ";
    scanResult += WiFi.RSSI(i);
    scanResult += " | ";
    scanResult += WiFi.channel(i);
    if (WiFi.channel(i) < 10) {
      scanResult += ' ';
    }
    scanResult += " | ";
    switch (WiFi.encryptionType(i)) {
      case WIFI_AUTH_OPEN:
      scanResult += "open";
      break;
      case WIFI_AUTH_WEP:
      scanResult += "WEP";
      break;
      case WIFI_AUTH_WPA_PSK:
      scanResult += "WPA";
      break;
      case WIFI_AUTH_WPA2_PSK:
      scanResult += "WPA2";
      break;
      case WIFI_AUTH_WPA_WPA2_PSK:
      scanResult += "WPA+WPA2";
      break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
      scanResult += "WPA2-EAP";
      break;
      case WIFI_AUTH_WPA3_PSK:
      scanResult += "WPA3";
      break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
      scanResult += "WPA2+WPA3";
      break;
      case WIFI_AUTH_WAPI_PSK:
      scanResult += "WAPI";
      break;
      default:
      scanResult += "unk";
    }
    scanResult += '\n';
    delay(10);
  }
 }
 return scanResult;
}

void rssiWiFi(int col, int row)
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

String uptimeCount() {

  time_t curr_time = time(nullptr);
  time_t up_time = curr_time - boot_time;
  int uptime_dd = up_time / 86400;
  int uptime_hh = (up_time / 3600) % 24;
  int uptime_mm = (up_time / 60 ) % 60;
  int uptime_ss = up_time % 60;

  String uptimeStr(String(uptime_dd) + " days " + String(uptime_hh) + " hours " + String(uptime_mm) + " minutes " + String(uptime_ss) + " seconds");
  return uptimeStr;
}

String infoChip() {
  uint32_t total_internal_memory = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  uint32_t free_internal_memory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  String infoChipStr[8];

  infoChipStr[0] += "Chip:";
  infoChipStr[0] += ESP.getChipModel();
  infoChipStr[0] += " R";
  infoChipStr[0] += ESP.getChipRevision();
  
  infoChipStr[1] += "CPU :";
  infoChipStr[1] += ESP.getChipCores();
  infoChipStr[1] += " cores @ ";
  infoChipStr[1] += ESP.getCpuFreqMHz();
  infoChipStr[1] += "MHz";
  
  infoChipStr[2] += "Flash frequency ";
  infoChipStr[2] += ESP.getFlashChipSpeed() / 1000000;
  infoChipStr[2] += "MHz";

  infoChipStr[3] += "Flash size: ";
  infoChipStr[3] += ESP.getFlashChipSize() * 0.001;
  infoChipStr[3] += "kb";

  infoChipStr[4] += "Flash size: ";
  infoChipStr[4] += ESP.getFlashChipSize() / 1024;
  infoChipStr[4] += " kib";

  infoChipStr[5] += "Free  heap: ";
  infoChipStr[5] += esp_get_free_heap_size() * 0.001;
  infoChipStr[5] += " kb";

  infoChipStr[6] += "Free  DRAM: ";
  infoChipStr[6] += free_internal_memory * 0.001;
  infoChipStr[6] += " kb";

  infoChipStr[7] += "Total DRAM: ";
  infoChipStr[7] += total_internal_memory * 0.001;
  infoChipStr[7] += " kb";

  String infoChipAll = "";
  for (int i = 0; i < 8; i++) {
    infoChipAll += i + 1;
    infoChipAll += ". ";
    infoChipAll += infoChipStr[i];
    infoChipAll += '\n';
  }
  return infoChipAll;
}

void lcdBackLight(bool backLight) {
  if (backLight) {
    lcd.setBacklight(1);
    return;
  } else {
    lcd.setBacklight(0);
    return;
  }
}

void signalBeep(int beep_ms) {
  uint32_t beep_start = millis();
  while (millis() <= beep_start + beep_ms) {
    digitalWrite(PIN_BEEP, HIGH);
  }
  digitalWrite(PIN_BEEP, LOW);
}

void signalBuzz(int beep_count, int beep_length, int beep_pause)
{
  for (int i = 0; i < beep_count; i++)
  {
    uint32_t beep_start = millis();
    uint32_t beep_stop = beep_start + beep_length;
    while (millis() <= beep_stop)
    {
      digitalWrite(PIN_BEEP, HIGH);
    }
    while (millis() < beep_stop + beep_pause)
    {
      digitalWrite(PIN_BEEP, LOW);
    }
  }
  digitalWrite(PIN_BEEP, LOW);
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
  return;
}

void onTelnetConnect(String ip)
{
  signalBeep(50);
  TRACE("- Telnet: %s connected\n", ip.c_str());
  TLNET("\nWelcome %s\n(Use ^] +q to disconnect)\n", telnet.getIP().c_str());
  client_ip = ip.c_str();
}

void onTelnetDisconnect(String ip)
{
  signalBeep(100);
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

void onTelnetInput(const String comm_telnet)
{
  TLNET("[%s] > %s\n", getTimeStr(0), comm_telnet);
  TLNET("%s", commHandler(comm_telnet).c_str());
}

void readSerial() {
  if (Serial.available())
  {
    String comm_serial = Serial.readStringUntil('\n');
    TRACE("[%s] > %s :\n", getTimeStr(0), comm_serial);
    TRACE("%s", commHandler(comm_serial).c_str());
  }
}

void screenDraw(String lcdRow0, String lcdRow1) {
  lcd.setCursor(0, 0);
  lcd.printf("%s", lcdRow0.c_str());
  lcd.setCursor(0, 1);
  lcd.printf("%s", lcdRow1.c_str());

  rssiWiFi(15, 1);
  if (telnet.isConnected()) {
    lcd.setCursor(15, 0);
    lcd.print('T');
  } else {
    lcd.setCursor(15, 0);
    lcd.print(' ');
  }
}

int* buttonsRead() {
  
  static int keysArr[4];
  static bool btnFlag[4];
  bool btnArr[4];

  btnArr[0] = !digitalRead(PIN_BTN_0);
  btnArr[1] = !digitalRead(PIN_BTN_1);
  btnArr[2] = !digitalRead(PIN_BTN_2);
  btnArr[3] = !digitalRead(PIN_BTN_3);

  for (int i = 0; i < 4; i++) {
    if (btnArr[i] && !btnFlag[i]) {
      lcd.clear();
      keysArr[i]++;
      btnFlag[i] = true;
      if (keysArr[i] > 5) {
        keysArr[i] = 0;
      }
    } else if (!btnArr[i] && btnFlag[i]) {
      btnFlag[i] = false;
    }
  }
  return keysArr;
} 

void buttonsExec(int* btn_curr)
{
  static uint32_t key0timer;
  uint32_t period = 200;
  if (millis() >= key0timer + period)
  {
    switch (btn_curr[0])
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
      String lcdRowStr0(getTimeStr(0) + " " + getTimeStr(8));
      String lcdRowStr1("  " + getSensVal('t') + 'C' + " " + getSensVal('h') + '%');
      screenDraw(lcdRowStr0, lcdRowStr1);
    }
      break;
    case 3: {
      String lcdRowStr0(getTimeStr(1) + " " + getTimeStr(7));
      String lcdRowStr1("  " + getSensVal('t') + 'C' + " " + getSensVal('h') + '%');
      screenDraw(lcdRowStr0, lcdRowStr1);
    }
      break;
    case 4:
      btn_curr[0] = 0;
      break;
    }
    key0timer = millis();
  }

  switch (btn_curr[1])
  {
    case 0:

    break;
  case 1:
    {
      digitalWrite(PIN_BEEP, HIGH);
      break;
    }
  case 2:
    {
      digitalWrite(PIN_BEEP, LOW);
      break;
    }
  case 3:

    break;
  case 4:
    btn_curr[1] = 0;
    break;
  }

  switch (btn_curr[2])
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
    btn_curr[2] = 0;
  }

  switch (btn_curr[3])
  {
  case 0:
    lcdBackLight(backLight);
    break;
  case 1:
    lcdBackLight(!backLight);
    break;
  case 2:
    btn_curr[3] = 0;
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
    strftime(timeStrRet, 32, "%d %h", timeinfo); // 23 Aug
    break;
  case 9:
    strftime(timeStrRet, 32, "%c", timeinfo); // Thu Aug 23 14:55:02 2001
    break;
  }

  return timeStrRet;
}

String getSensVal(char valSelact) {
  float sens_temperature;
  float sens_relhumidity;
  String errorStr("err");
  sens_temperature = sht.getTemperature();
  sens_relhumidity = sht.getHumidity();
  String sensTempStr = String(sens_temperature, 1);
  String sensHumidStr = String(sens_relhumidity, 1);
  if (valSelact == 't') {
    return sensTempStr;
  } else if (valSelact == 'h') {
    return sensHumidStr;
  } else {
    return errorStr;
  }
}

String testSensor()
{
  static uint32_t start, stop;
  static int sht_timer;
  int sht_period = 2000;
  String testResult("");
  if (millis() >= (sht_timer + sht_period))
  {
    sht_timer = millis();
    start = micros();
    sht.read(); //  default = true/fast       slow = false
    stop = micros();
    float lasted = (stop - start) * 0.001;

    testResult += "SHT30:\nRead time: " + String(lasted) + "ms\n";
    testResult += "Tempeatue: " + String(sht.getTemperature()) + "C\n";
    testResult += "Humidity : " + String(sht.getHumidity()) + "%\n";
    testResult += "Wire clk : " + String(Wire.getClock()) + '\n';
  }
  return testResult;
}

void readSensor(int readPeriod) {
  static int readTimer;
  if (millis() >= readTimer + readPeriod) {
    sht.read();
    readTimer = millis();
  }
}

String commHandler(const String comm_input) {
  int exec_case = 0;
  String comm_output("");
  int comm_qty = sizeof(comm_array) / sizeof(comm_array[0]);
  for (int i = 1; i < comm_qty; i++) {
    if (comm_array[i][0] == comm_input) {
      exec_case = i;
      break;
    } else {
      exec_case = 0;
    }
  }

  switch (exec_case)
  {
    case 0:
    {
      comm_output += "Command \"" + comm_input + "\" not understood\n";
      break;
    }
    case 1:
    {
      comm_output += "ESP32-S3 Board CLI. Built: " + String(__DATE__) + ' ' + String(__TIME__) + '\n';
      break;
    }
    case 2:
    {
      comm_output += term_clear;
      break;
    }
    case 3:
    {
      comm_output += "Hi, " + client_ip + '\n';
      break;
    }
    case 4:
    {
      comm_output += "Pong!\n";
      break;
    }
    case 5:
    {
      for (int k = 1; k < comm_qty; k++) {
        if (k < 10) {
          comm_output += "[  " + String(k) + " ] " + comm_array[k][0] + "\t - " + comm_array[k][1] + '\n';
        } else {
          comm_output += "[ " + String(k) + " ] " + comm_array[k][0] + "\t - " + comm_array[k][1] + '\n';
        }
      }
      break;
    }
    case 6:
    {
      comm_output += infoWiFi();
      comm_output += '\n';
      break;
    }
    case 7:
    {
      comm_output += infoChip();
      comm_output += '\n';
      break;
    }
    case 8:
    {
      comm_output += "Client " + client_ip + " disconnected\n";
      telnet.disconnectClient();
      break;
    }
    case 9:
    {
      comm_output += getTimeStr(0) + '\n';
      break;
    }
    case 10:
    {
      comm_output += getTimeStr(7) + '\n';
      break;
    }
    case 11:
    {
      if (!isWiFiOn()) {
        comm_output += "Connecting WiFi to " + String(mssid) + '\n';
        initWiFi(mssid, mpass, 20, 500);
      } else {
        comm_output += "WiFi is already connected!\n";
      }
      break;
    }
    case 12:
    {
      if (isWiFiOn()) {
        WiFi.disconnect();
        comm_output += "WiFi disconnected\n";
      } else {
        comm_output += "WiFi is already disconnected\n";
      }
      break;
    }
    case 13:
    {
      comm_output += "WiFi scan starts at " + getTimeStr(0) + '\n';
      comm_output += scanWiFi();
      comm_output += "Scan complete\n";
      break;
    }
    case 14:
    {
      comm_output += "RSSI: " + String(WiFi.RSSI()) + "dBm\n";
      break;
    }
    case 15:
    {
      comm_output += String(ESP.getChipModel()) + '\n';
      break;
    }
    case 16:
    {
      comm_output += "ESP restating in 3 sec\n";
      TRACE("%s", comm_output.c_str());
      if (telnet.isConnected()) {
        TLNET("%s", comm_output.c_str());
        TLNET("Disconnecting you!\n");
        telnet.disconnectClient();
      }
      static int reboot_time = millis() + 3000;
      while (millis() < reboot_time)
      {
        TRACE(".");
        delay(200);
      }
      ESP.restart();
      break;
    }
    case 17:
    {
      comm_output+= "Running setup()\n";
      setup();
      break;
    }
    case 18:
    {
      comm_output += "System start : " + boot_date + '\n';
      comm_output += "System uptime: " + uptimeCount() + '\n';
      break;
    }
    case 19:
    {
      backLight = !backLight;
      String backlightState;
      if (backLight)
      {
        backlightState = "ON";
      }
      else
      {
        backlightState = "OFF";
      }
      comm_output += "LCD backlight switched to " + backlightState + '\n';
      backlightState.clear();
      break;
    }
    case 20:
    {
      if (Serial) {
        comm_output += "Serial is already up!\n";
      } else {
        setupSerial(0);
        comm_output += "Setting up Serial\n";
      }
      break;
    }
    case 21:
    {
      int* btn_curr = buttonsRead();
      comm_output += String(btn_curr[0]) + ' ';
      comm_output += String(btn_curr[1]) + ' ';
      comm_output += String(btn_curr[2]) + ' ';
      comm_output += String(btn_curr[3]) + ' ';
      comm_output += "<- BTN state V2\n";
      break;
    }
    case 22:
    {
      comm_output += testSensor();
      break;
    }
    case 23:
    {
      comm_output += getSensVal('t') + "C " + getSensVal('h') + "%\n";
      break;
    }
  }
  return comm_output;
}

void setup()
{
  pinMode(PIN_BTN_0, INPUT);
  pinMode(PIN_BTN_1, INPUT);
  pinMode(PIN_BTN_2, INPUT);
  pinMode(PIN_BTN_3, INPUT);
  pinMode(PIN_BEEP, OUTPUT);
  rgb_led.begin();
  rgb_led.setPixelColor(0, rgb_led.Color(64, 64, 0));
  rgb_led.show();

  setupSerial(0);
  setupSerial(1);

  lcd.init(I2C_SDA, I2C_SCL); // initialize LCD(I2C pins)
  lcd.backlight();
  initWiFi(mssid, mpass, 20, 500);

  TRACE("%s\n", infoWiFi().c_str());

  if (isWiFiOn())
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

  TRACE("Setting up SHT30 sensor\n");
  TRACE("SHT_LIB_VERSION: %s\t", SHT_LIB_VERSION);
  if (sht.begin()) {
    delay(200);
    TRACE("Sensor SHT30 set\n");
    TRACE("%s", testSensor().c_str());
  } else {
    TRACE("SHT30 sensor not started!\n");
  }
  
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

  boot_date = getTimeStr(0) + ' ' + getTimeStr(6);
  boot_time = time(nullptr);

  TRACE("\nGuru meditates...\nESP32 Ready.\n");
  rgb_led.setPixelColor(0, rgb_led.Color(0, 255, 0)); // Green
  rgb_led.show();
  signalBuzz(2, 50, 75);
  rgb_led.clear();
  rgb_led.show();
}

void loop(void)
{
  telnet.loop();
  readSerial();
  int readPeriod = 5000;
  readSensor(readPeriod);
  buttonsExec(buttonsRead());
}

