#include "display.h"
#include "config.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <SD.h>

#if DISPLAY_MODEL == SSD1306
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#elif DISPLAY_MODEL == SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#else
#error "DISPLAY_MODEL no valido: usa SSD1306 o SH1106 en include/config.h"
#endif

void displayInit() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setContrast(255);
  displayClear();
}

void displayClear() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void displayFlush() {
  u8g2.sendBuffer();
}

void displayTitle(const char* title) {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, title);
  u8g2.drawHLine(0, 13, 128);
  u8g2.sendBuffer();
}

void displayLine(int line, const char* text) {
  // Cada línea ~10 px de alto, hasta 5 líneas (0..4)
  u8g2.drawStr(0, 22 + line * 10, text);
  u8g2.sendBuffer();
}

void displayMenu(const char* title, const char* const items[], int count, int selected) {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, title);
  u8g2.drawHLine(0, 13, 128);

  // Ventana de 5 opciones visibles con scroll
  const int VISIBLE = 5;
  int start = 0;
  if (selected >= VISIBLE) start = selected - VISIBLE + 1;
  if (start > count - VISIBLE) start = count - VISIBLE;
  if (start < 0) start = 0;

  for (int i = 0; i < VISIBLE && (start + i) < count; i++) {
    int idx = start + i;
    int y = 22 + i * 10;
    if (idx == selected) {
      u8g2.drawStr(0, y, ">");
      u8g2.drawStr(10, y, items[idx]);
    } else {
      u8g2.drawStr(10, y, items[idx]);
    }
  }
  u8g2.sendBuffer();
}

// ---------- Auto-test ----------
void displaySelfTest(bool nfc, bool sd, bool rf, int joyX, int joyY) {
  bool joyOk = (joyX > 200 && joyX < 3900 && joyY > 200 && joyY < 3900);
  char buf[40];

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 9, "AUTO-TEST");
  u8g2.drawHLine(0, 12, 128);

  snprintf(buf, sizeof(buf), "Pantalla  OK");
  u8g2.drawStr(0, 23, buf);
  snprintf(buf, sizeof(buf), "NFC       %s", nfc ? "OK" : "FALLA");
  u8g2.drawStr(0, 33, buf);
  snprintf(buf, sizeof(buf), "SD        %s", sd ? "OK" : "sin tarjeta");
  u8g2.drawStr(0, 43, buf);
  snprintf(buf, sizeof(buf), "RF24      %s", rf ? "OK" : "no conectado");
  u8g2.drawStr(0, 53, buf);
  snprintf(buf, sizeof(buf), "Joystick  %s", joyOk ? "OK" : "?");
  u8g2.drawStr(0, 63, buf);
  u8g2.sendBuffer();
}

// ---------- Animación de inicio ----------
static void drawHeart(int x, int y, int r) {
  u8g2.drawDisc(x - r, y, r);
  u8g2.drawDisc(x + r, y, r);
  u8g2.drawTriangle(x - 2 * r, y, x + 2 * r, y, x, y + 2 * r);
}

static void drawSparkle(int x, int y, int s) {
  u8g2.drawLine(x - s, y, x + s, y);
  u8g2.drawLine(x, y - s, x, y + s);
}

static void drawChibi(int cx, int cy, bool blink, int heartY, bool sparkleOn) {
  // orejas de gato
  u8g2.drawTriangle(cx - 17, cy - 13, cx - 9, cy - 30, cx - 2, cy - 15);
  u8g2.drawTriangle(cx + 17, cy - 13, cx + 9, cy - 30, cx + 2, cy - 15);
  // cabeza
  u8g2.drawCircle(cx, cy, 20);
  u8g2.drawCircle(cx, cy, 19);
  // ojos
  if (blink) {
    u8g2.drawLine(cx - 13, cy - 2, cx - 5, cy - 2);
    u8g2.drawLine(cx + 5, cy - 2, cx + 13, cy - 2);
  } else {
    u8g2.drawCircle(cx - 9, cy - 2, 4);
    u8g2.drawCircle(cx + 9, cy - 2, 4);
    u8g2.drawDisc(cx - 10, cy - 3, 1);   // brillo del ojo
    u8g2.drawDisc(cx + 8, cy - 3, 1);
  }
  // boca (w)
  u8g2.drawLine(cx - 4, cy + 8, cx - 2, cy + 10);
  u8g2.drawLine(cx - 2, cy + 10, cx, cy + 8);
  u8g2.drawLine(cx, cy + 8, cx + 2, cy + 10);
  u8g2.drawLine(cx + 2, cy + 10, cx + 4, cy + 8);
  // rubor
  u8g2.drawLine(cx - 16, cy + 4, cx - 12, cy + 4);
  u8g2.drawLine(cx + 12, cy + 4, cx + 16, cy + 4);
  // corazón flotando
  if (heartY > 2 && heartY < 64) drawHeart(cx + 32, heartY, 3);
  // destellos
  if (sparkleOn) {
    drawSparkle(18, 18, 3);
    drawSparkle(112, 50, 3);
  }
}

void displayBootAnimation() {
  const int cx = 64, cy = 36;

  // 1) aparecen destellos y la carita
  for (int f = 0; f < 10; f++) {
    if (displayJoySkip()) return;
    u8g2.clearBuffer();
    drawSparkle(18, 18, 3);
    drawSparkle(112, 50, 3);
    if (f > 3) drawChibi(cx, cy, false, -1, true);
    u8g2.sendBuffer();
    delay(70);
  }

  // 2) animación con parpadeos y corazón subiendo
  for (int f = 0; f < 26; f++) {
    if (displayJoySkip()) return;
    u8g2.clearBuffer();
    bool blink = (f >= 6 && f <= 8) || (f >= 19 && f <= 21);
    int heartY = 58 - f * 2;              // el corazón sube
    drawChibi(cx, cy, blink, heartY, (f % 4) < 2);
    u8g2.sendBuffer();
    delay(80);
  }

  // 3) texto final "nya~"
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(20, 28, "FLIPPER-ESP32");
  u8g2.drawStr(40, 44, "nya~ <3");
  drawSparkle(14, 20, 3);
  drawSparkle(114, 46, 3);
  u8g2.sendBuffer();
  delay(1300);
}

// ---------- Intro desde la SD (/intro.bin) ----------
// Devuelve true si encontro y reprodujo el archivo.
// Se puede saltar moviendo/pulsando el joystick.
bool displayPlayIntroSD(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  uint8_t buf[1024];
  unsigned long next = millis();
  while (f.available() >= 1024) {
    if (displayJoySkip()) break;       // saltar con el joystick
    f.read(buf, 1024);
    memcpy(u8g2.getBufferPtr(), buf, 1024);
    u8g2.sendBuffer();
    next += 83;                       // ~12 fps
    long wait = (long)(next - millis());
    if (wait > 0) delay(wait);
    else next = millis();             // resincroniza si va atrasado
  }
  f.close();
  return true;
}

// true si el joystick esta pulsado o movido fuera del centro
bool displayJoySkip() {
  if (digitalRead(PIN_JOY_SW) == LOW) return true;
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  if (x < 900 || x > 3200 || y < 900 || y > 3200) return true;
  return false;
}
