#include "USB/USBAPI.h"
#include <cstdlib>
#include <sys/_types.h>
#include "WString.h"
#include "packet.h"

String packetSetup(TemperaturePacket p) {
  String time_stamp = String(p.timeStamp);
  String temp = String(p.temp);
  String message = p.packetId + "," + p.nodeId + "," + temp + "," + time_stamp;

  return message;
}

TemperaturePacket packetParser(String str) {
  int first_comma = str.indexOf(",");
  int second_comma = str.indexOf(",", first_comma + 1);
  int third_comma = str.indexOf(",", second_comma + 1);

  String packet_id = str.substring(0, first_comma);
  String node_id = str.substring(first_comma + 1, second_comma);
  String temp_str = str.substring(second_comma + 1, third_comma);
  float temp = atof(temp_str.c_str());
  String time_stamp_str = str.substring(third_comma + 1, str.length());

  unsigned long time_stamp = strtoul(time_stamp_str.c_str(), NULL, 10);

  TemperaturePacket p = { packet_id, node_id, temp, time_stamp };
  return p;
}

void serialPrintPacket(TemperaturePacket p) {
  SerialUSB.println("packet_id = " + p.packetId);
  SerialUSB.println("node id = " + p.nodeId);
  SerialUSB.print("temp = ");
  SerialUSB.println(p.temp);
  SerialUSB.print("timeStamp = ");
  SerialUSB.println(p.timeStamp);
  SerialUSB.println("--------------");
}