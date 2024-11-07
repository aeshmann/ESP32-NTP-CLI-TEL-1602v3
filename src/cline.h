#include <cstdint>

const char* build_date = __DATE__ " " __TIME__;
const char* term_clear = "\033[2J\033[H";
const int comm_qty = 24;
const String comm_array[][2]{
    {"", "error"},                           // 0
    {"abt", "information about chip"},       // 1
    {"cls", "clear terminal screen"},        // 2
    {"helo", "greets you, showing ip"},      // 3
    {"ping", "echoes reply \"pong\""},       // 4
    {"help", "prints command help"},         // 5
    {"wfinf", "prints wifi info"},           // 6
    {"hwinf", "prints chip info"},           // 7
    {"bye", "disconnects your session"},     // 8
    {"time", "prints current time"},         // 9
    {"date", "prints current date"},         // 10
    {"wfcon", "connects to wifi"},           // 11
    {"wfoff", "turns off wifi"},             // 12
    {"wfscn", "scans for wifi networks"},    // 13
    {"rssi", "prints wifi RSSI"},            // 14
    {"uname", "prints chip name"},           // 15
    {"rboot", "reboots MCU"},                // 16
    {"reset", "soft reset MCU"},             // 17
    {"utime", "prints uptime"},              // 18
    {"lcdsw", "switch LCD on/off"},          // 19
    {"comon", "launch setup Serial"},        // 20
    {"testb", "seial show btn arr output"},  // 21
    {"tests", "test SHT30 sensor"},          // 22
    {"sens", "show sensor values"}           // 23
};

const char* comm_about =
"\nHi! I'm ESP32 WROOM Board\n"
"Sketch of NTP clock, telnet\n"
"and some CLI commands\n"
"compiled: ";
const char* comm_err = "Command not understood: ";
const char* comm_helo = "Hi, ";
const char* tnet_discon = "Disconnecting you!";