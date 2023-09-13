#ifndef PACKET_H
#define PACKET_H
#include <Arduino.h>

enum ErrorCode {
  NO_ERROR = 0,
  WDT_RESET = 1,
  RECEPTION_FAIL = 2
};

struct ErrorPacket {
  String packetId;
  String nodeId;
  ErrorCode errCode;
  unsigned long timeStamp;
};

struct TemperaturePacket {
  String packetId;
  String nodeId;
  float temp;
  unsigned long timeStamp;
};

String packetSetup(TemperaturePacket p);

TemperaturePacket packetParser(String str);

void serialPrintPacket(TemperaturePacket p);

#endif