#include "badusb.h"
#include "config.h"
#include <SD.h>

#if ENABLE_BADUSB
#include <BleKeyboard.h>

// Nombre con el que aparecerá el ESP32 al emparejarlo por Bluetooth.
BleKeyboard bleKeyboard("Flipper-ESP32", "progra", 100);

void badusbInit() {
  bleKeyboard.begin();
  // begin() NO bloquea: deja el ESP32 visible para emparejar.
  // Empareja "Flipper-ESP32" en la PC o el móvil antes de ejecutar un script.
}

void badusbType(const char* text) {
  if (!bleKeyboard.isConnected()) return;
  bleKeyboard.print(text);
}

void badusbPressRelease(uint8_t key) {
  if (!bleKeyboard.isConnected()) return;
  bleKeyboard.press(key);
  delay(30);
  bleKeyboard.release(key);
}

void badusbCombo(uint8_t modifier, char key) {
  if (!bleKeyboard.isConnected()) return;
  bleKeyboard.press(modifier);
  delay(30);
  bleKeyboard.write(key);
  delay(30);
  bleKeyboard.release(modifier);
}

// Intérprete muy simple de scripts:
//   STRING <texto>          -> escribe texto
//   ENTER / TAB / ESC / SPACE / UP / DOWN / LEFT / RIGHT
//   GUI r / CTRL c / ALT f4 / SHIFT a  -> combinación
//   DELAY <milisegundos>    -> espera
//   # comentario
bool badusbRunScript(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (line.startsWith("#") || line.startsWith("REM")) continue;

    int sp = line.indexOf(' ');
    String cmd = (sp == -1) ? line : line.substring(0, sp);
    String arg = (sp == -1) ? "" : line.substring(sp + 1);
    cmd.toUpperCase();
    arg.trim();

    if (cmd == "STRING") {
      badusbType(arg.c_str());
    } else if (cmd == "ENTER")  { badusbPressRelease(KEY_RETURN); }
    else if (cmd == "TAB")      { badusbPressRelease(KEY_TAB); }
    else if (cmd == "ESC")      { badusbPressRelease(KEY_ESC); }
    else if (cmd == "SPACE")    { badusbType(" "); }
    else if (cmd == "UP")       { badusbPressRelease(KEY_UP_ARROW); }
    else if (cmd == "DOWN")     { badusbPressRelease(KEY_DOWN_ARROW); }
    else if (cmd == "LEFT")     { badusbPressRelease(KEY_LEFT_ARROW); }
    else if (cmd == "RIGHT")    { badusbPressRelease(KEY_RIGHT_ARROW); }
    else if (cmd == "GUI"   && arg.length() > 0) { badusbCombo(KEY_LEFT_GUI,   arg[0]); }
    else if (cmd == "CTRL"  && arg.length() > 0) { badusbCombo(KEY_LEFT_CTRL,  arg[0]); }
    else if (cmd == "ALT"   && arg.length() > 0) { badusbCombo(KEY_LEFT_ALT,   arg[0]); }
    else if (cmd == "SHIFT" && arg.length() > 0) { badusbCombo(KEY_LEFT_SHIFT, arg[0]); }
    else if (cmd == "DELAY" && arg.length() > 0) { delay(arg.toInt()); }
  }
  f.close();
  return true;
}
#endif
