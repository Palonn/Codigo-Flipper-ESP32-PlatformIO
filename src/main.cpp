#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <SD.h>
#include "config.h"
#include "display.h"
#include "nfc.h"
#include "radio.h"
#include "storage.h"
#include "badusb.h"
#include "ir.h"
#include "wifi_mod.h"
#include "bt_mod.h"
#include <Lua.h>
#include "lua/lua.hpp"

// ================= Joystick =================
// Devuelve 1=ARRIBA 2=ABAJO 3=OK (pulsar) 4=ATRÁS (izquierda)
// Espera a que el joystick vuelva al centro para no repetir la orden.
int waitButton() {
  const int CENTER = 2048;
  const int DEAD   = 900;
  static bool lastUp = false, lastDown = false, lastSw = true, lastLeft = false;

  while (true) {
    int y = analogRead(PIN_JOY_Y);
    int x = analogRead(PIN_JOY_X);
    bool sw = (digitalRead(PIN_JOY_SW) == LOW);

    bool up   = (y < CENTER - DEAD);
    bool down = (y > CENTER + DEAD);
    bool left = (x < CENTER - DEAD);

    int r = 0;
    if (up   && !lastUp)   r = 1;
    else if (down && !lastDown) r = 2;
    else if (sw   && !lastSw)   r = 3;
    else if (left && !lastLeft) r = 4;

    lastUp = up; lastDown = down; lastSw = sw; lastLeft = left;

    if (r != 0) {
      // Espera a que regrese al centro (para no repetir al mantenerlo)
      while (true) {
        delay(10);
        int y2 = analogRead(PIN_JOY_Y);
        int x2 = analogRead(PIN_JOY_X);
        bool sw2 = (digitalRead(PIN_JOY_SW) == LOW);
        bool centered = (y2 > CENTER - DEAD) && (y2 < CENTER + DEAD) &&
                        (x2 > CENTER - DEAD) && (x2 < CENTER + DEAD) && !sw2;
        if (centered) break;
      }
      delay(60);   // pequeño descanso anti-rebote
      lastUp = false; lastDown = false; lastLeft = false; lastSw = true;
      return r;
    }
    delay(10);
  }
}

// ================= Batería =================
int readBatteryPct() {
  analogSetPinAttenuation(PIN_BAT, ADC_11db);   // hasta ~3.3V (x2 por divisor)
  int raw = 0;
  for (int i = 0; i < 8; i++) { raw += analogRead(PIN_BAT); delay(2); }
  raw /= 8;
  float v = (raw / 4095.0) * 3.3 * 2.0;          // divisor 10k/10k
  int pct = map((int)(v * 100), 300, 420, 0, 100); // 3.0V=0%, 4.2V=100%
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// ================= Dormir (light sleep) =================
void doSleep() {
  displayTitle("Dormir");
  displayLine(0, "Pulsa el joystick");
  displayLine(1, "para despertar");
  delay(1200);
  displayClear();                                  // apaga pantalla
  gpio_wakeup_enable((gpio_num_t)PIN_JOY_SW, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();                         // Zzz...
  displayInit();                                   // al despertar
  displayTitle("Despierto!");
  delay(800);
}

// ================= Config (/config.txt) =================
String cfgRead(const char* key) {
  File f = SD.open("/config.txt", FILE_READ);
  if (!f) return "";
  String v = "";
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    int eq = line.indexOf('=');
    if (eq > 0) {
      String k = line.substring(0, eq); k.trim();
      if (k == key) { v = line.substring(eq + 1); v.trim(); break; }
    }
  }
  f.close();
  return v;
}

void cfgWrite(const char* key, const char* value) {
  String all = "";
  File f = SD.open("/config.txt", FILE_READ);
  if (f) { while (f.available()) all += (char)f.read(); f.close(); }
  String out = "";
  int start = 0;
  while (start < (int)all.length()) {
    int nl = all.indexOf('\n', start);
    String line = (nl == -1) ? all.substring(start) : all.substring(start, nl);
    line.trim();
    if (line.length() > 0 && !line.startsWith(String(key) + "=")) out += line + "\n";
    if (nl == -1) break;
    start = nl + 1;
  }
  out += String(key) + "=" + String(value) + "\n";
  SD.remove("/config.txt");
  File w = SD.open("/config.txt", FILE_WRITE);
  if (w) { w.print(out); w.close(); }
}

// ================= Menús =================
const char* OS_MENU[] = {"Configuracion", "Info", "Programas", "Dormir"};
const int OS_MENU_N = 4;

const char* NFC_MENU[] = {"Leer tarjeta", "Clonar", "Emular", "Volver"};
const int NFC_MENU_N = 4;

const char* IR_MENU[] = {"Recibir", "Reenviar", "Volver"};
const int IR_MENU_N = 3;

const char* RF_MENU[] = {"Escanear canales", "Test loopback", "Volver"};
const int RF_MENU_N = 3;

const char* USB_MENU[] = {"Ejecutar demo", "Volver"};
const int USB_MENU_N = 2;

const char* SD_MENU[] = {"Listar raiz", "Ver /nfc.log", "Volver"};
const int SD_MENU_N = 3;

const char* WIFI_MENU[] = {"Escanear", "Conectar (SD)", "Punto AP", "Volver"};
const int WIFI_MENU_N = 4;

// Muestra un menú y devuelve el índice elegido.
// BACK siempre equivale a la última opción ("Volver").
int runMenu(const char* title, const char* const items[], int n) {
  int sel = 0;
  while (true) {
    displayMenu(title, items, n, sel);
    int b = waitButton();
    if (b == 1)      sel = (sel - 1 + n) % n;
    else if (b == 2) sel = (sel + 1) % n;
    else if (b == 3) return sel;
    else if (b == 4) return n - 1;
  }
}

// Muestra un texto multilínea en pantalla (hasta 4 líneas).
void showText(const char* title, const String& text) {
  displayTitle(title);
  int line = 0, start = 0;
  while (line < 4) {
    int nl = text.indexOf('\n', start);
    String chunk = (nl == -1) ? text.substring(start) : text.substring(start, nl);
    displayLine(line, chunk.c_str());
    line++;
    if (nl == -1) break;
    start = nl + 1;
  }
  displayLine(4, "OK para volver");
  while (waitButton() != 3) {}
}

// ================= Acciones =================
void nfcRead() {
  displayTitle("Acerca una tarjeta");
  displayLine(0, "Escaneando...");
  displayLine(2, "hasta 10 seg");
  nfcResume();                       // asegura modo lector

  NfcCard card;
  bool found = false;
  unsigned long t0 = millis();
  while (millis() - t0 < 10000) {    // escanea hasta 10 segundos
    if (nfcReadCard(card)) { found = true; break; }
    delay(50);
  }

  if (found) {
    String uid = nfcUidToString(card);
    String texto;
    bool hasText = nfcReadNdefText(card, texto);   // leer con la tarjeta aún activa

    displayTitle("Tarjeta leida");
    displayLine(0, uid.c_str());
    if (hasText) {
      int i = 0, line = 1;
      while (i < (int)texto.length() && line < 4) {
        String chunk = texto.substring(i, i + 20);
        displayLine(line, chunk.c_str());
        i += 20;
        line++;
      }
      displayLine(4, (i < (int)texto.length()) ? "..." : "OK para volver");
    } else {
      // ---- Diagnóstico: re-escanear y mostrar datos crudos ----
      NfcCard c2;
      bool ok2 = false;
      for (int i = 0; i < 100; i++) { if (nfcReadCard(c2)) { ok2 = true; break; } delay(20); }
      if (ok2) {
        String diag = nfcDiagnose(c2);
        int line = 1, start = 0;
        while (line < 4) {
          int nl = diag.indexOf('\n', start);
          String chunk = (nl == -1) ? diag.substring(start) : diag.substring(start, nl);
          displayLine(line, chunk.c_str());
          line++;
          if (nl == -1) break;
          start = nl + 1;
        }
        // Guardar diagnóstico detallado en la SD
        storageWrite("/nfc_diag.txt", nfcDiagLog(c2));
      } else {
        displayLine(1, "(re-acerca la tarjeta)");
      }
      displayLine(4, "OK para volver");
    }
  } else {
    displayTitle("Sin tarjeta");
    displayLine(0, "No se detecto nada");
    displayLine(4, "OK para volver");
  }
  while (waitButton() != 3) {}
}

void nfcCloneAction() {
  displayTitle("Acerca la tarjeta");
  displayLine(0, "para clonar...");
  NfcCard card;
  bool found = false;
  unsigned long t0 = millis();
  while (millis() - t0 < 10000) {
    if (nfcReadCard(card)) { found = true; break; }
    delay(50);
  }
  if (!found) {
    displayTitle("Sin tarjeta");
    displayLine(0, "No se detecto");
    displayLine(4, "OK para volver");
    while (waitButton() != 3) {}
    return;
  }
  String uid = nfcUidToString(card);
  displayTitle("Leyendo...");
  displayLine(0, uid.c_str());
  String dump = nfcCloneDump(card);

  String fname = "/clone_" + uid + ".txt";
  fname.replace(":", "");
  bool ok = storageWrite(fname.c_str(), dump);

  displayTitle("Clonado!");
  displayLine(0, uid.c_str());
  displayLine(2, ok ? fname.c_str() : "SD no disponible");
  displayLine(4, "OK para volver");
  while (waitButton() != 3) {}
}

void nfcEmulate() {
  displayTitle("Emular (NDEF)");
  displayLine(0, "El PN532 actua");
  displayLine(1, "como tarjeta NDEF");
  displayLine(2, "generica (UID fijo)");
  nfcEmulateStart();
  displayLine(4, "BACK para salir");
  while (waitButton() != 4) {}
  nfcEmulateStop();
}

void rfScan() {
  displayTitle("Escaneando 2.4GHz");
  uint8_t chans[126];
  int n = radioScanChannels(chans, 126);
  displayTitle("Canales activos");
  displayLine(0, (String(n) + " con senal").c_str());
  String list = "";
  for (int i = 0; i < n && i < 8; i++) list += String(chans[i]) + " ";
  displayLine(1, list.c_str());
  displayLine(3, "OK para volver");
  while (waitButton() != 3) {}
}

void rfLoopback() {
  displayTitle("Test NRF24 1->2");
  bool ok = radioLoopbackTest();
  displayLine(0, ok ? "RECIBIDO OK!" : "FALLO");
  displayLine(2, "OK para volver");
  while (waitButton() != 3) {}
}

static bool g_badusbReady = false;
void ensureBadusb() {
  if (!g_badusbReady) { badusbInit(); g_badusbReady = true; delay(300); }
}

void usbDemo() {
  ensureBadusb();
  displayTitle("BadUSB");
  bool ok = badusbRunScript(BADUSB_SCRIPT);
  displayLine(0, ok ? "Script ejecutado" : "No hay script");
  displayLine(1, "(pon demo.txt en SD)");
  displayLine(3, "OK para volver");
  while (waitButton() != 3) {}
}

void irReceiveAction() {
  displayTitle("Apunta un control");
  displayLine(0, "y presiona un");
  displayLine(1, "boton...");
  String result = "";
  unsigned long t0 = millis();
  while (millis() - t0 < 10000) {
    result = irReceive();
    if (result.length() > 0) break;
    delay(10);
  }
  if (result.length() == 0) result = "Sin senal";
  displayTitle("IR recibido");
  int line = 0, start = 0;
  while (line < 3) {
    int nl = result.indexOf('\n', start);
    String chunk = (nl == -1) ? result.substring(start) : result.substring(start, nl);
    displayLine(line, chunk.c_str());
    line++;
    if (nl == -1) break;
    start = nl + 1;
  }
  displayLine(4, "OK para volver");
  while (waitButton() != 3) {}
}

void irReplayAction() {
  displayTitle("Reenviando IR");
  bool ok = irReplay();
  displayLine(0, ok ? "Enviado!" : "No hay codigo");
  displayLine(1, "(recibe uno antes)");
  displayLine(3, "OK para volver");
  while (waitButton() != 3) {}
}

// ================= WiFi / BT =================
void wifiScanAction() {
  displayTitle("Escaneando WiFi");
  displayLine(0, "espera...");
  String list = wifiScan();
  storageWrite("/wifi_scan.txt", list);
  showText("Redes WiFi", list);
}

void wifiConnectAction() {
  String cfg = storageRead("/wifi.txt");
  if (cfg.length() == 0) {
    showText("WiFi", "Falta /wifi.txt\nLinea 1: SSID\nLinea 2: clave");
    return;
  }
  int nl = cfg.indexOf('\n');
  String ssid = (nl == -1) ? cfg : cfg.substring(0, nl);
  String pass = (nl == -1) ? "" : cfg.substring(nl + 1);
  ssid.trim();
  pass.trim();
  displayTitle("Conectando...");
  displayLine(0, ssid.c_str());
  bool ok = wifiConnect(ssid.c_str(), pass.c_str());
  displayTitle(ok ? "Conectado!" : "Fallo");
  if (ok) displayLine(0, wifiIP().c_str());
  displayLine(4, "OK para volver");
  while (waitButton() != 3) {}
}

void wifiAPAction() {
  displayTitle("Punto de acceso");
  displayLine(0, "iniciando...");
  wifiAP("FLIPPER-ESP32", "");
  displayTitle("AP activo");
  displayLine(0, "SSID: FLIPPER-ESP32");
  displayLine(1, "IP: 192.168.4.1");
  displayLine(3, "OK para apagar");
  while (waitButton() != 3) {}
  wifiOff();
}

void btScanAction() {
  displayTitle("Escaneando BLE");
  displayLine(0, "4 seg...");
  String list = btScan();
  storageWrite("/bt_scan.txt", list);
  showText("Dispositivos BLE", list);
}

// ================= Lua (Fase 2) =================
// Redirige print() de Lua a la pantalla
class DisplayPrint : public Print {
public:
  String buf;
  int line = 0;
  size_t write(uint8_t c) override {
    if (c == '\n') {
      if (buf.length()) { displayLine(line, buf.c_str()); line = (line + 1) % 5; buf = ""; }
    } else if (c != '\r') {
      buf += (char)c;
    }
    return 1;
  }
};

static Lua* g_lua = nullptr;
static DisplayPrint* g_dp = nullptr;

static int os_delay(lua_State* L) { delay((int)luaL_checkinteger(L, 1)); return 0; }
static int os_title(lua_State* L) { displayTitle(luaL_checkstring(L, 1)); if (g_dp) g_dp->line = 0; return 0; }
static int os_clear(lua_State* L) { displayClear(); if (g_dp) g_dp->line = 0; return 0; }
static int os_wait(lua_State* L) { while (waitButton() != 3) {} return 0; }
static int os_joy_wait(lua_State* L) { lua_pushinteger(L, waitButton()); return 1; }

static int os_nfc_read(lua_State* L) {
  NfcCard card; bool ok = false; unsigned long t0 = millis();
  while (millis() - t0 < 8000) { if (nfcReadCard(card)) { ok = true; break; } delay(50); }
  if (!ok) { lua_pushnil(L); return 1; }
  String uid = nfcUidToString(card);
  lua_pushstring(L, uid.c_str());
  return 1;
}
static int os_nfc_reader(lua_State* L) { nfcRead(); return 0; }
static int os_nfc_clone(lua_State* L)  { nfcCloneAction(); return 0; }
static int os_nfc_emulate(lua_State* L){ nfcEmulate(); return 0; }
static int os_ir_recv(lua_State* L)    { irReceiveAction(); return 0; }
static int os_ir_replay(lua_State* L)  { irReplayAction(); return 0; }
static int os_rf_scan(lua_State* L)    { rfScan(); return 0; }
static int os_rf_loopback(lua_State* L){ rfLoopback(); return 0; }
static int os_wifi_scan(lua_State* L)  { String s = wifiScan(); lua_pushstring(L, s.c_str()); return 1; }
static int os_wifi_ap(lua_State* L)    { wifiAPAction(); return 0; }
static int os_wifi_connect(lua_State* L){ wifiConnectAction(); return 0; }
static int os_bt_scan(lua_State* L)    { String s = btScan(); lua_pushstring(L, s.c_str()); return 1; }
static int os_badusb(lua_State* L)     { ensureBadusb(); badusbRunScript(luaL_checkstring(L, 1)); return 0; }

static const luaL_Reg os_lib[] = {
  {"delay", os_delay}, {"title", os_title}, {"clear", os_clear}, {"wait", os_wait},
  {"joy_wait", os_joy_wait},
  {"nfc_read", os_nfc_read}, {"nfc_reader", os_nfc_reader}, {"nfc_clone", os_nfc_clone}, {"nfc_emulate", os_nfc_emulate},
  {"ir_recv", os_ir_recv}, {"ir_replay", os_ir_replay},
  {"rf_scan", os_rf_scan}, {"rf_loopback", os_rf_loopback},
  {"wifi_scan", os_wifi_scan}, {"wifi_ap", os_wifi_ap}, {"wifi_connect", os_wifi_connect},
  {"bt_scan", os_bt_scan}, {"badusb", os_badusb},
  {nullptr, nullptr}
};
static int luaopen_flipper(lua_State* L) { luaL_newlib(L, os_lib); return 1; }

void runLuaProgram(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) { showText("Lua", "No se pudo abrir"); return; }
  String script = "";
  while (f.available()) script += (char)f.read();
  f.close();

  if (!g_lua) { g_lua = new Lua(); g_lua->addModule("os", luaopen_flipper); }

  displayTitle("Lua");
  DisplayPrint dp;
  g_dp = &dp;
  g_lua->run(script.c_str(), &dp);
  g_dp = nullptr;
  if (dp.buf.length()) displayLine(dp.line, dp.buf.c_str());
  displayTitle("Fin Lua");
  displayLine(0, "OK para volver");
  while (waitButton() != 3) {}
}

// ================= Intérprete de programas (SD) =================
void runProgram(const char* path) {
  String p = path;
  p.toLowerCase();
  if (p.endsWith(".lua")) { runLuaProgram(path); return; }

  File f = SD.open(path, FILE_READ);
  if (!f) { showText("Error", "No se pudo abrir"); return; }
  displayTitle("Ejecutando");
  displayLine(0, path);
  delay(500);

  int line = 0;
  while (f.available()) {
    String s = f.readStringUntil('\n');
    s.trim();
    if (s.length() == 0 || s.startsWith("#")) continue;
    int sp = s.indexOf(' ');
    String cmd = (sp == -1) ? s : s.substring(0, sp);
    String arg = (sp == -1) ? "" : s.substring(sp + 1);
    cmd.toUpperCase();

    if (cmd == "TITLE")       { displayTitle(arg.c_str()); line = 0; }
    else if (cmd == "PRINT")  { displayLine(line, arg.c_str()); line = (line + 1) % 5; }
    else if (cmd == "CLEAR")  { displayClear(); line = 0; }
    else if (cmd == "DELAY")  { delay(arg.toInt()); }
    else if (cmd == "WAIT")   { while (waitButton() != 3) {} }
    else if (cmd == "NFC_READ")  { nfcRead(); line = 0; }
    else if (cmd == "NFC_CLONE") { nfcCloneAction(); line = 0; }
    else if (cmd == "IR_RECV")   { irReceiveAction(); line = 0; }
    else if (cmd == "IR_REPLAY") { irReplayAction(); line = 0; }
    else if (cmd == "WIFI_SCAN") { wifiScanAction(); line = 0; }
    else if (cmd == "WIFI_AP")   { wifiAPAction(); line = 0; }
    else if (cmd == "BT_SCAN")   { btScanAction(); line = 0; }
    else if (cmd == "RF_SCAN")   { rfScan(); line = 0; }
    else if (cmd == "BADUSB")    { if (arg.length()) badusbRunScript(arg.c_str()); line = 0; }
    else { displayLine(line, ("? " + cmd).c_str()); line = (line + 1) % 5; }
  }
  f.close();
  displayTitle("Programa terminado");
  displayLine(0, "OK para volver");
  while (waitButton() != 3) {}
}

// ================= Menú Programas =================
void programasMenu() {
  File dir = SD.open("/programas");
  if (!dir) { showText("Programas", "No existe /programas"); return; }
  static String names[21];
  static const char* items[22];
  int n = 0;
  File e = dir.openNextFile();
  while (e && n < 20) {
    if (!e.isDirectory()) { names[n] = e.name(); n++; }
    e = dir.openNextFile();
  }
  dir.close();
  if (n == 0) { showText("Programas", "Carpeta vacia"); return; }
  for (int i = 0; i < n; i++) items[i] = names[i].c_str();
  items[n] = "Volver";
  int sel = runMenu("Programas", items, n + 1);
  if (sel < 0 || sel >= n) return;
  String path = "/programas/" + names[sel];
  runProgram(path.c_str());
}

// ================= Configuración (intro) =================
void configuracionMenu() {
  File dir = SD.open("/intro");
  if (!dir) { showText("Configuracion", "No existe /intro"); return; }
  static String names[21];
  static const char* items[22];
  int n = 0;
  File e = dir.openNextFile();
  while (e && n < 20) {
    if (!e.isDirectory()) { names[n] = e.name(); n++; }
    e = dir.openNextFile();
  }
  dir.close();
  if (n == 0) { showText("Configuracion", "Sin intros en /intro"); return; }
  for (int i = 0; i < n; i++) items[i] = names[i].c_str();
  items[n] = "Volver";
  int sel = runMenu("Intro de inicio", items, n + 1);
  if (sel < 0 || sel >= n) return;

  cfgWrite("intro", names[sel].c_str());
  displayTitle("Guardado!");
  displayLine(0, names[sel].c_str());
  displayLine(2, "Se usara al reiniciar");
  displayLine(4, "OK para volver");
  while (waitButton() != 3) {}
}

// ================= Info =================
void infoAction() {
  int n = 0;
  File dir = SD.open("/programas");
  if (dir) {
    File e = dir.openNextFile();
    while (e) { if (!e.isDirectory()) n++; e = dir.openNextFile(); }
    dir.close();
  }
  String info = "Flipper-ESP32 OS\n";
  info += "NFC+IR+RF+WiFi+BT\n";
  info += "Bat: " + String(readBatteryPct()) + "%\n";
  info += "Progs: " + String(n) + " en /programas";
  showText("Info", info);
}

// ================= main =================
void setup() {
  Serial.begin(115200);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  // PIN_JOY_X y PIN_JOY_Y son analógicos: analogRead() no necesita pinMode
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  displayInit();

  bool okSd = storageInit();        // SD primero (para la intro)

  // ---- Intro configurable desde /config.txt ----
  String introName = cfgRead("intro");
  if (introName.length() == 0) introName = "intro.bin";
  String introPath = "/intro/" + introName;
  if (!displayPlayIntroSD(introPath.c_str())) {
    displayBootAnimation();
  }

  // ---- Inicializar módulos ----
  bool okNfc   = nfcInit();
  bool okRadio = radioInit();
  irInit();

  int joyX = analogRead(PIN_JOY_X);
  int joyY = analogRead(PIN_JOY_Y);

  // ---- Auto-test: reporte por serial ----
  Serial.println("=== AUTO-TEST ===");
  Serial.println("Pantalla: OK");
  Serial.printf("NFC:      %s\n", okNfc ? "OK" : "FALLA");
  Serial.printf("SD:       %s\n", okSd ? "OK" : "sin tarjeta");
  Serial.printf("RF24:     %s\n", okRadio ? "OK" : "no conectado");
  Serial.printf("Joystick: X=%d Y=%d\n", joyX, joyY);
  Serial.println("=================");

  displaySelfTest(okNfc, okSd, okRadio, joyX, joyY);
  // espera 3 s, pero se puede saltar con el joystick
  unsigned long tSelf = millis();
  while (millis() - tSelf < 3000) {
    if (displayJoySkip()) break;
    delay(20);
  }
}

void loop() {
  int sel = runMenu("FLIPPER-ESP32", OS_MENU, OS_MENU_N);
  switch (sel) {
    case 0: configuracionMenu(); break;
    case 1: infoAction();        break;
    case 2: programasMenu();     break;
    case 3: doSleep();           break;
  }
}
