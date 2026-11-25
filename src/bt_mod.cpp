#include "bt_mod.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// BLEDevice::init es idempotente (si BleKeyboard ya lo inicio, no hace nada)
String btScan() {
  BLEDevice::init("FLIPPER-ESP32");

  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(99);

  BLEScanResults res = scan->start(4, false);   // 4 segundos de escaneo
  String out = "";
  int n = res.getCount();
  for (int i = 0; i < n && i < 20; i++) {
    BLEAdvertisedDevice d = res.getDevice(i);
    String name = d.haveName() ? String(d.getName().c_str()) : String("(sin nombre)");
    out += name + " (" + String(d.getRSSI()) + ")\n";
  }
  if (n == 0) out = "Ninguno\n";
  scan->clearResults();
  return out;
}
