#include "wifi_mod.h"
#include <WiFi.h>
#include <BLEDevice.h>

void wifiInit() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

String wifiScan() {
  BLEDevice::deinit(true);                      // libera el radio del BLE (si estaba activo)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(150);                                   // deja estabilizar el radio
  int n = WiFi.scanNetworks(false, false);      // síncrono: bloquea hasta terminar
  String out = "";
  if (n <= 0) {
    out = "Sin redes\n";
  } else {
    for (int i = 0; i < n && i < 20; i++) {
      out += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + ")\n";
    }
  }
  WiFi.scanDelete();
  return out;
}

bool wifiConnect(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 100) { delay(100); t++; }
  return WiFi.status() == WL_CONNECTED;
}

void wifiAP(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
}

void wifiOff() {
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

String wifiIP() {
  return WiFi.localIP().toString();
}
