#ifndef PACKET_H
#define PACKET_H
#include <Arduino.h>

enum ErrorCode {
  NO_ERROR = 0,
  WDT_RESET = 1,
  RECEPTION_FAIL = 2
};

struct Packet {
  String type;
  String packetId;
  String senderId;
  String nodeId;
  String payload;
  long timeStamp;
};

struct ErrorPacket {
  String packetId;
  int nodeId;
  ErrorCode errCode;
  unsigned long timeStamp;
};

struct TemperaturePacket {
  String packetId;
  int nodeId;
  float temp;
  unsigned long timeStamp;
};
Packet packetParser(String str);

String packetSetup(Packet p);

TemperaturePacket tempPacketParser(String str);

void serialPrintPacket(Packet p);

#endif