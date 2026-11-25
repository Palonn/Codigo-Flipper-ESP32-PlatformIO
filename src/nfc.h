#pragma once
#include <Arduino.h>

// Datos básicos de una tarjeta NFC leída.
struct NfcCard {
  uint8_t uid[7];
  uint8_t uidLen;
};

// Inicializa el PN532 por I2C. Devuelve true si detecta el módulo.
bool nfcInit();

// Lee el UID de una tarjeta ISO14443A cercana (Mifare/NTAG...).
// Devuelve true si leyó algo.
bool nfcReadCard(NfcCard& card);

// Convierte el UID a texto hexadecimal legible (ej: "04:3A:1B:2C").
String nfcUidToString(const NfcCard& card);

// Pone el PN532 en modo "tarjeta" (emulación) para que otro lector lo vea.
bool nfcEmulateStart();

// Vuelve a modo lector (sin reiniciar el bus I2C).
void nfcEmulateStop();

// Reconfigura el PN532 como lector (por si otro módulo lo distrajo).
void nfcResume();

// Lee el texto/URL NDEF de la tarjeta activa (NTAG o Mifare Classic).
// Debe llamarse justo después de nfcReadCard(), con la tarjeta aún activa.
bool nfcReadNdefText(const NfcCard& card, String& out);

// Diagnóstico: devuelve texto con datos crudos (tipo de tarjeta, auth, bloque 4).
String nfcDiagnose(const NfcCard& card);

// Diagnóstico detallado (todos los sectores/llaves) para guardar en la SD.
String nfcDiagLog(const NfcCard& card);

// Genera un dump completo (UID + sectores) de la tarjeta para "clonar".
String nfcCloneDump(const NfcCard& card);
