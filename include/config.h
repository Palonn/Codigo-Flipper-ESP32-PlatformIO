#pragma once

// =============================================================
//  FLIPPER-ESP32 — Configuración de pines y opciones
//  ESP32 clásico (ESP32-WROOM-32 / DevKit de 30 o 38 pines)
//  AJUSTA AQUÍ LOS PINES SEGÚN TU PLACA.
// =============================================================

// ---------- Pantalla OLED ----------
// Elige el controlador de tu pantalla (I2C):
//   SSD1306 -> OLED 0.96" 128x64 (el más común)
//   SH1106  -> OLED 1.3"  128x64
//#define DISPLAY_MODEL SSD1306
#define DISPLAY_MODEL SH1106

// Direcciones I2C habituales: OLED = 0x3C (a veces 0x3D), PN532 = 0x24

// ---------- Bus SPI (compartido: microSD + 2x NRF24L01) ----------
// Pines por defecto del ESP32 clásico (VSPI).
#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23
#define PIN_SD_CS 5

// ---------- NRF24L01 (dos módulos) ----------
#define PIN_NRF1_CE  17
#define PIN_NRF1_CSN 16
#define PIN_NRF2_CE  26
#define PIN_NRF2_CSN 27

// ---------- I2C (PN532 NFC + pantalla OLED) ----------
// Pines por defecto del ESP32 clásico.
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// ---------- Joystick (KY-023: 2 ejes analógicos + botón) ----------
// VRx -> eje X  (izquierda = ATRÁS)
// VRy -> eje Y  (arriba/abajo = navegar menú)
// SW  -> botón central (OK)
#define PIN_JOY_X   34   // VRx (analógico)
#define PIN_JOY_Y   35   // VRy (analógico)
#define PIN_JOY_SW  25   // SW (digital, pull-up)

// ---------- Infrarrojo ----------
// Receptor TSOP (OUT) y LED emisor (ánodo vía resistencia 100-220Ω)
#define PIN_IR_RX  4
#define PIN_IR_TX  13

// ---------- LED de estado (GPIO2 = LED integrado de muchos DevKit) ----------
#define PIN_LED 2

// ---------- Batería (ADC) ----------
// Necesita divisor de voltaje 10k/10k entre BAT+ y GND, con el punto medio al GPIO33
#define PIN_BAT 33

// ---------- BadUSB por Bluetooth (teclado BLE) ----------
// 1 = activa el teclado BLE (emparejas el ESP32 como teclado en la PC/móvil).
// 0 = lo desactiva.
#define ENABLE_BADUSB 1

// ---------- Script de demostración para BadUSB (en la microSD) ----------
#define BADUSB_SCRIPT "/badusb/demo.txt"
