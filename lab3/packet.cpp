#include "USB/USBAPI.h"
#include "packet.h"

String packetSetup(Packet p) {

  String type = p.type;
  String packet_id = p.packetId;
  String node_id = p.nodeId;
  String payload = p.payload;
  String time_stamp = String(p.timeStamp);
  
  String message = type + "," + packet_id + "," + node_id + "," + payload + "," + time_stamp;

  return message;
}

Packet packetParser(String str){
  int first_comma = str.indexOf(",");
  int second_comma = str.indexOf(",", first_comma + 1);
  int third_comma = str.indexOf(",", second_comma + 1);
  int fourth_comma = str.indexOf(",", third_comma + 1 );
  int fifth_comma = str.indexOf(",", fourth_comma + 1 );

  String type = str.substring(0, first_comma);
  String packet_id = str.substring(first_comma + 1, second_comma);
  
  String sender_id = str.substring(second_comma + 1, third_comma);
  String node_id = str.substring(third_comma + 1, fourth_comma);
  String payload = str.substring(fourth_comma + 1, fifth_comma);
  long time_stamp = atol(str.substring(fifth_comma + 1, str.length()).c_str());

  Packet p = {type, packet_id, sender_id, node_id, payload, time_stamp};

  return p;
}

void serialPrintPacket(Packet p) {
  SerialUSB.println("type = " + p.type);
  SerialUSB.println("packet_id = " + p.packetId);
  SerialUSB.println("sender_id = " + p.senderId);
  SerialUSB.println("node id = " + p.nodeId);
  SerialUSB.println("payload = " + p.payload);
  SerialUSB.print("timeStamp = ");
  SerialUSB.println(p.timeStamp);
  SerialUSB.println("--------------");
}