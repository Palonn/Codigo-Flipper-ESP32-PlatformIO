#include "nfc.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_PN532.h>

// I2C (Wire) sin pines IRQ/RESET (255 -> -1).
Adafruit_PN532 pn532(255, 255);

bool nfcInit() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  pn532.begin();
  uint32_t version = pn532.getFirmwareVersion();
  if (!version) {
    return false;              // no se detectó el módulo
  }
  pn532.SAMConfig();           // configura el modo seguro para leer tarjetas
  return true;
}

bool nfcReadCard(NfcCard& card) {
  uint8_t uid[7] = {0};
  uint8_t uidLen = 0;

  // Espera 200 ms por una tarjeta ISO14443A (Mifare Classic, NTAG, etc.)
  if (!pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 200)) {
    return false;
  }

  memcpy(card.uid, uid, sizeof(uid));
  card.uidLen = uidLen;
  return true;
}

String nfcUidToString(const NfcCard& card) {
  String s = "";
  char buf[4];
  for (uint8_t i = 0; i < card.uidLen; i++) {
    sprintf(buf, "%02X", card.uid[i]);
    s += buf;
    if (i + 1 < card.uidLen) s += ":";
  }
  return s;
}

bool nfcEmulateStart() {
  // AsTarget: el PN532 deja de ser lector y pasa a actuar
  // como si fuera una tarjeta/etiqueta NFC.
  return (pn532.AsTarget() != 0);
}

void nfcEmulateStop() {
  // Volvemos a configurarlo como lector.
  pn532.SAMConfig();
}

void nfcResume() {
  pn532.SAMConfig();
}

// Prefijos de URI NDEF (registro tipo 'U')
static String ndefUriPrefix(uint8_t p) {
  switch (p) {
    case 0x01: return "http://www.";
    case 0x02: return "https://www.";
    case 0x03: return "http://";
    case 0x04: return "https://";
    case 0x05: return "tel:";
    case 0x06: return "mailto:";
    default:   return "";
  }
}

// Parser común de TLV NDEF: buf[0]=0x03, buf[1]=len, buf[2..]=mensaje.
static bool parseNdefTlv(const uint8_t* buf, int total, String& out) {
  if (total < 3 || buf[0] != 0x03) return false;
  int len = buf[1];
  if (len < 1 || len == 0xFF) return false;
  if (2 + len > total) return false;

  const uint8_t* msg = buf + 2;
  uint8_t header = msg[0];
  int typeLen = msg[1];
  int payloadLen, off;
  if (header & 0x10) {                          // short record
    payloadLen = msg[2];
    off = 3;
  } else {
    if (len < 6) return false;
    payloadLen = (msg[2] << 24) | (msg[3] << 16) | (msg[4] << 8) | msg[5];
    off = 6;
  }
  if (off + typeLen + payloadLen > len) return false;

  uint8_t type0 = msg[off];
  const uint8_t* payload = msg + off + typeLen;

  out = "";
  if (typeLen == 1 && type0 == 'T') {           // registro de texto
    uint8_t status = payload[0];
    int langLen = status & 0x3F;
    int textLen = payloadLen - 1 - langLen;
    if (textLen < 0) return false;
    for (int i = 0; i < textLen; i++) out += (char)payload[1 + langLen + i];
    return out.length() > 0;
  } else if (typeLen == 1 && type0 == 'U') {    // URL
    out = ndefUriPrefix(payload[0]);
    for (int i = 1; i < payloadLen; i++) out += (char)payload[i];
    return out.length() > 0;
  } else if (typeLen == 10 && memcmp(msg + off, "text/plain", 10) == 0) {
    // MIME "text/plain": el payload es el texto crudo
    for (int i = 0; i < payloadLen; i++) out += (char)payload[i];
    return out.length() > 0;
  }
  return false;
}

// Lee NDEF de una tarjeta Type 2 (NTAG/Ultralight) leyendo páginas desde la 4.
static bool readType2(String& out) {
  uint8_t page[4];
  if (!pn532.mifareultralight_ReadPage(4, page)) return false;
  if (page[0] != 0x03) return false;           // no hay TLV NDEF
  int len = page[1];
  if (len < 1 || len == 0xFF) return false;    // longitud extendida: no soportada

  uint8_t buf[258];
  buf[0] = 0x03;
  buf[1] = (uint8_t)len;
  int total = 2 + len;
  buf[2] = page[2];
  buf[3] = page[3];
  int got = 4;
  int pg = 5;
  while (got < total && pg < 232) {
    if (!pn532.mifareultralight_ReadPage(pg, page)) return false;
    for (int i = 0; i < 4 && got < total; i++) buf[got++] = page[i];
    pg++;
  }
  if (got < total) return false;
  return parseNdefTlv(buf, total, out);
}

// Lee NDEF de una tarjeta Mifare Classic.
static bool readMifareClassic(const NfcCard& card, String& out) {
  // 1) Camino directo: llave NDEF (D3F7..) en sector 1 (bloque 4) = lo mas comun
  static const uint8_t NDEF_KEY[6] = {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7};
  if (pn532.mifareclassic_AuthenticateBlock((uint8_t*)card.uid, 4, 4, 0, (uint8_t*)NDEF_KEY)) {
    uint8_t buf[48], block[16];
    bool ok = true;
    for (int b = 0; b < 3; b++) {
      if (!pn532.mifareclassic_ReadDataBlock(4 + b, block)) { ok = false; break; }
      memcpy(buf + b * 16, block, 16);
    }
    if (ok) {
      for (int start = 0; start + 2 < 48; start++) {
        if (buf[start] == 0x03 && buf[start + 1] > 0 && buf[start + 1] != 0xFF) {
          int len = buf[start + 1];
          if (start + 2 + len <= 48 && parseNdefTlv(buf + start, 2 + len, out)) return true;
        }
      }
    }
  }

  // 2) Respaldo: probar sectores 1..15 con llaves comunes (D3F7 primero)
  static const uint8_t KEYS[][6] = {
    {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7},   // NDEF
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},   // default
    {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},   // MAD
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},
    {0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD},
    {0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A},
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
  };
  const int NKEYS = sizeof(KEYS) / sizeof(KEYS[0]);

  // Sector 0 = MAD; sectores 1..15 = datos (1K)
  for (int sector = 1; sector <= 15; sector++) {
    int firstBlock = sector * 4;
    for (int k = 0; k < NKEYS; k++) {
      for (int kt = 0; kt < 2; kt++) {          // 0 = KeyA, 1 = KeyB
        if (!pn532.mifareclassic_AuthenticateBlock((uint8_t*)card.uid, 4,
                                                   firstBlock, kt, (uint8_t*)KEYS[k]))
          continue;

        uint8_t buf[48];
        uint8_t block[16];
        bool okRead = true;
        for (int b = 0; b < 3; b++) {
          if (!pn532.mifareclassic_ReadDataBlock(firstBlock + b, block)) { okRead = false; break; }
          memcpy(buf + b * 16, block, 16);
        }
        if (!okRead) continue;

        // Buscar el TLV NDEF (0x03) en el sector
        for (int start = 0; start + 2 < 48; start++) {
          if (buf[start] == 0x03 && buf[start + 1] > 0 && buf[start + 1] != 0xFF) {
            int len = buf[start + 1];
            if (start + 2 + len <= 48) {
              if (parseNdefTlv(buf + start, 2 + len, out)) return true;
            }
          }
        }
      }
    }
  }
  return false;
}

// Intenta leer el texto NDEF probando Mifare Classic primero y luego Type 2.
bool nfcReadNdefText(const NfcCard& card, String& out) {
  if (readMifareClassic(card, out)) return true;
  if (readType2(out)) return true;
  return false;
}

String nfcDiagnose(const NfcCard& card) {
  String out = "";
  char d[40];

  uint8_t p4[4] = {0};
  bool t2 = pn532.mifareultralight_ReadPage(4, p4);
  if (t2) {
    snprintf(d, sizeof(d), "P4:%02X%02X%02X%02X", p4[0], p4[1], p4[2], p4[3]);
  } else {
    snprintf(d, sizeof(d), "P4: no (no T2)");
  }
  out += d;
  out += "\n";

  uint8_t k1[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  bool a1 = pn532.mifareclassic_AuthenticateBlock((uint8_t*)card.uid, 4, 4, 0, k1);
  out += a1 ? "Auth FF: OK\n" : "Auth FF: FAIL\n";

  if (a1) {
    uint8_t b4[16] = {0};
    if (pn532.mifareclassic_ReadDataBlock(4, b4)) {
      snprintf(d, sizeof(d), "%02X%02X%02X%02X%02X%02X%02X%02X",
               b4[0], b4[1], b4[2], b4[3], b4[4], b4[5], b4[6], b4[7]);
      out += d;
    }
  }
  return out;
}

// Diagnóstico detallado para guardar en la SD: prueba todos los sectores y llaves.
String nfcDiagLog(const NfcCard& card) {
  String log = "";
  char d[96];
  snprintf(d, sizeof(d), "UIDlen=%d UID=", card.uidLen);
  log += d;
  for (int i = 0; i < card.uidLen; i++) { snprintf(d, sizeof(d), "%02X", card.uid[i]); log += d; }
  log += "\n";

  static const uint8_t KEYS[][6] = {
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
    {0xD3,0xF7,0xD3,0xF7,0xD3,0xF7},
    {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5},
    {0x00,0x00,0x00,0x00,0x00,0x00},
    {0xB0,0xB1,0xB2,0xB3,0xB4,0xB5},
    {0x4D,0x3A,0x99,0xC3,0x51,0xDD},
    {0x1A,0x98,0x2C,0x7E,0x45,0x9A},
    {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF},
  };
  const int NKEYS = sizeof(KEYS) / sizeof(KEYS[0]);
  bool anyAuth = false;

  for (int sector = 1; sector <= 15; sector++) {
    int fb = sector * 4;
    bool got = false;
    for (int k = 0; k < NKEYS && !got; k++) {
      for (int kt = 0; kt < 2 && !got; kt++) {
        if (pn532.mifareclassic_AuthenticateBlock((uint8_t*)card.uid, 4, fb, kt, (uint8_t*)KEYS[k])) {
          anyAuth = true; got = true;
          snprintf(d, sizeof(d), "AUTH S%d K%d%c data=", sector, k, kt ? 'B' : 'A');
          log += d;
          uint8_t b[16];
          for (int bb = 0; bb < 3; bb++) {
            if (pn532.mifareclassic_ReadDataBlock(fb + bb, b)) {
              for (int i = 0; i < 16; i++) { snprintf(d, sizeof(d), "%02X", b[i]); log += d; }
            } else {
              log += "RF";
            }
          }
          log += "\n";
        }
      }
    }
  }
  if (!anyAuth) log += "NINGUNA LLAVE AUTENTICO\n";
  return log;
}

// Dump completo de la tarjeta (UID + 16 sectores) para clonar.
String nfcCloneDump(const NfcCard& card) {
  String d = "";
  char buf[48];
  d += "UID=";
  for (int i = 0; i < card.uidLen; i++) { snprintf(buf, sizeof(buf), "%02X", card.uid[i]); d += buf; }
  d += "\n";

  static const uint8_t KEYS[][6] = {
    {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},
  };
  const int NKEYS = sizeof(KEYS) / sizeof(KEYS[0]);

  for (int sector = 0; sector <= 15; sector++) {
    int fb = sector * 4;
    int usedKey = -1, usedType = 0;
    for (int k = 0; k < NKEYS && usedKey < 0; k++) {
      for (int kt = 0; kt < 2 && usedKey < 0; kt++) {
        if (pn532.mifareclassic_AuthenticateBlock((uint8_t*)card.uid, 4, fb, kt, (uint8_t*)KEYS[k])) {
          usedKey = k; usedType = kt;
        }
      }
    }
    snprintf(buf, sizeof(buf), "S%02d ", sector);
    d += buf;
    uint8_t b[16];
    for (int bb = 0; bb < 4; bb++) {
      if (usedKey >= 0 && pn532.mifareclassic_ReadDataBlock(fb + bb, b)) {
        for (int i = 0; i < 16; i++) { snprintf(buf, sizeof(buf), "%02X", b[i]); d += buf; }
      } else {
        d += "--------------------------------";
      }
      d += " ";
    }
    d += "\n";
  }
  return d;
}
