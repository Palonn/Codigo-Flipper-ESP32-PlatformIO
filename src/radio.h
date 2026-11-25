#pragma once
#include <Arduino.h>

// Inicializa los dos módulos NRF24L01. Devuelve true si ambos responden.
bool radioInit();

// Recorre los 126 canales de 2.4 GHz y guarda los que tienen actividad.
// Devuelve la cantidad de canales con señal detectada.
int radioScanChannels(uint8_t* activeChannels, int maxChannels);

// Demo loopback: el módulo 1 transmite un mensaje y el 2 lo recibe.
// Devuelve true si el mensaje llegó correctamente.
bool radioLoopbackTest();
