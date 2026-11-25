#include "storage.h"
#include "config.h"
#include <SD.h>

bool storageInit() {
  return SD.begin(PIN_SD_CS);
}

bool storageAppend(const char* path, const String& line) {
  File f = SD.open(path, FILE_APPEND);
  if (!f) return false;
  f.println(line);
  f.close();
  return true;
}

String storageRead(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return "";
  String out = "";
  while (f.available()) {
    out += (char)f.read();
  }
  f.close();
  return out;
}

String storageList(const char* dir) {
  File root = SD.open(dir);
  if (!root) return "";
  String out = "";
  File entry = root.openNextFile();
  while (entry) {
    out += String(entry.isDirectory() ? "[D] " : "    ") + entry.name() + "\n";
    entry = root.openNextFile();
  }
  root.close();
  return out;
}

bool storageWrite(const char* path, const String& content) {
  SD.remove(path);
  File f = SD.open(path, FILE_WRITE);
  if (!f) return false;
  f.print(content);
  f.close();
  return true;
}
