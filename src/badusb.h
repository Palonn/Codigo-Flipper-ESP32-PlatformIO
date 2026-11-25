#pragma once
#include <Arduino.h>
#include "config.h"

#if ENABLE_BADUSB
void badusbInit();                       // activa el teclado HID
void badusbType(const char* text);       // escribe texto
void badusbPressRelease(uint8_t key);    // presiona y suelta una tecla
void badusbCombo(uint8_t modifier, char key); // combina modificador + tecla
bool badusbRunScript(const char* path);  // ejecuta un script desde la SD
#else
// stubs para que el resto del código compile sin HID
inline void badusbInit() {}
inline void badusbType(const char*) {}
inline void badusbPressRelease(uint8_t) {}
inline void badusbCombo(uint8_t, char) {}
inline bool badusbRunScript(const char*) { return false; }
#endif
