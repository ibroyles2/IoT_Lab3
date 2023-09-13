#include <TemperatureZero.h>
#include "packet.h"
#include <SPI.h>
#include <RH_RF95.h>

RH_RF95 rf95(12, 6);

float frequency = 914;

String NODE_ID = "h";
String SUCCESSFULL_CODE = "s";
String ERROR_CODE = "e";

TemperatureZero TempZero = TemperatureZero();

int packet_counter;

void setup() {
  SerialUSB.begin(9600);

  TempZero.init();

  while (!SerialUSB)
    ;

  //Initialize the Radio.
  if (rf95.init() == false) {
    SerialUSB.println("Radio Init Failed - Freezing");
    while (1)
      ;
  } else {
    // rf95.setModemConfig(Bw125Cr48Sf4096); // slow and reliable?
    SerialUSB.println("Transmitter up!");
  }
  // Set frequency
  rf95.setFrequency(frequency);
  // Transmitter power can range from 14-20dbm.
  rf95.setTxPower(20, false);
}

void loop() {

  packet_counter++;
  float temperature = TempZero.readInternalTemperature();
  SerialUSB.print(temperature);
  String packet_id = SUCCESSFULL_CODE + String(packet_counter);
  int time_stamp = millis();

  TemperaturePacket p = { packet_id, NODE_ID, temperature, time_stamp };

  String message = packetSetup(p);

  sendMessage(message);
}

void sendMessage(String str) {
  uint8_t data_array[str.length()];
  str.getBytes(data_array, sizeof(data_array));
  rf95.send(data_array, sizeof(data_array));
}

String receivedMessage() {
  if (rf95.available()) {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      SerialUSB.print((char*)buf);
    } else
      SerialUSB.println("Recieve failed");
  }
}