#pragma once
#include <Arduino.h>

// Inicializa la pantalla (y el bus I2C).
void displayInit();

// Limpia la pantalla.
void displayClear();

// Dibuja un título (primera línea) con una raya separadora.
void displayTitle(const char* title);

// Escribe una línea de texto (line: 0..4).
void displayLine(int line, const char* text);

// Dibuja un menú con su título y resalta la opción seleccionada.
void displayMenu(const char* title, const char* const items[], int count, int selected);

// Fuerza el envío del buffer a la pantalla.
void displayFlush();

// Auto-test: muestra el estado de cada módulo.
void displaySelfTest(bool nfc, bool sd, bool rf, int joyX, int joyY);

// Animación de inicio (chibi otaku).
void displayBootAnimation();

// Reproduce una intro pregrabada desde la SD.
// Devuelve true si la encontró y reprodujo.
bool displayPlayIntroSD(const char* path);

// Devuelve true si el usuario pide saltar (pulsa o mueve el joystick).
bool displayJoySkip();