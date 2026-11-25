#include "radio.h"
#include "config.h"
#include <string.h>
#include <SPI.h>
#include <RF24.h>

// Dos módulos NRF24L01 en el MISMO bus SPI (CE/CSN distintos).
RF24 radio1(PIN_NRF1_CE, PIN_NRF1_CSN);
RF24 radio2(PIN_NRF2_CE, PIN_NRF2_CSN);

// Dirección (pipe) de 5 bytes usada por los dos radios.
const uint8_t PIPE_ADDR[5] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01};

bool radioInit() {
  // El bus SPI usa los pines por defecto del ESP32 (VSPI: 18/19/23),
  // así que no hace falta llamar a SPI.begin() explícitamente.
  if (!radio1.begin()) return false;
  if (!radio2.begin()) return false;

  radio1.setChannel(76);
  radio1.setPALevel(RF24_PA_MAX);
  radio1.setDataRate(RF24_1MBPS);
  radio1.setAutoAck(false);

  radio2.setChannel(76);
  radio2.setPALevel(RF24_PA_MAX);
  radio2.setDataRate(RF24_1MBPS);
  radio2.setAutoAck(false);

  return true;
}

int radioScanChannels(uint8_t* activeChannels, int maxChannels) {
  int found = 0;
  for (uint8_t ch = 0; ch <= 125; ch++) {
    radio1.setChannel(ch);
    radio1.startListening();
    delayMicroseconds(200);            // deja que el radio "escuche"
    if (radio1.testCarrier()) {        // detecta portadora en ese canal
      if (found < maxChannels) activeChannels[found] = ch;
      found++;
    }
    radio1.stopListening();
  }
  return found;
}

bool radioLoopbackTest() {
  const char* msg = "HOLA DESDE NRF24";

  radio1.openWritingPipe(PIPE_ADDR);
  radio2.openReadingPipe(1, PIPE_ADDR);

  radio1.stopListening();              // radio1 = transmisor
  radio2.startListening();             // radio2 = receptor

  bool sent = radio1.write(msg, strlen(msg) + 1);

  delay(50);

  char buf[32] = {0};
  if (radio2.available()) {
    radio2.read(buf, sizeof(buf));
  }

  return sent && (strcmp(buf, msg) == 0);
}
