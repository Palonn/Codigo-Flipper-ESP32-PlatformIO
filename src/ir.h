#pragma once
#include <Arduino.h>

// Inicializa el receptor y el emisor IR.
void irInit();

// Intenta decodificar una señal IR (no bloquea). Devuelve una descripción
// en texto o "" si no hay señal.
String irReceive();

// Reenvía la última señal NEC recibida. Devuelve true si envió.
bool irReplay();
