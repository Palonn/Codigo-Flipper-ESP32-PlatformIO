#include "ir.h"
#include "config.h"
#include <IRremote.hpp>

static bool hasCode = false;
static uint16_t lastAddress = 0;
static uint16_t lastCommand = 0;

void irInit() {
  IrReceiver.begin(PIN_IR_RX, false);
  IrSender.begin(PIN_IR_TX, false);
}

String irReceive() {
  if (!IrReceiver.decode()) return "";
  String s;
  if (IrReceiver.decodedIRData.protocol == NEC) {
    lastAddress = (uint16_t)IrReceiver.decodedIRData.address;
    lastCommand = (uint16_t)IrReceiver.decodedIRData.command;
    hasCode = true;
    s = "NEC\nAddr 0x" + String(lastAddress, HEX);
    s += "\nCmd 0x" + String(lastCommand, HEX);
  } else if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
    s = "No soportado\n(raw)";
  } else {
    s = "Proto " + String((int)IrReceiver.decodedIRData.protocol);
  }
  IrReceiver.resume();
  return s;
}

bool irReplay() {
  if (!hasCode) return false;
  IrSender.sendNEC(lastAddress, (uint8_t)lastCommand, 0);
  return true;
}
