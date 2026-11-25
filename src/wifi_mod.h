#pragma once
#include <Arduino.h>

void wifiInit();
String wifiScan();                         // lista "SSID (RSSI)" por red
bool wifiConnect(const char* ssid, const char* pass);
void wifiAP(const char* ssid, const char* pass);
void wifiOff();
String wifiIP();
