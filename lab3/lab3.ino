#include "packet.h"
#include <TemperatureZero.h>
#include <SPI.h>
#include <RH_RF95.h>
#include "timer.h"


#define MY_NODE "1";

void startTimer(int frequencyHz);
void setTimerFrequency(int frequencyHz);
void TC4_Handler();



RH_RF95 rf95(12, 6);

float frequency = 914;
float sumTemp = 0.0;
int tempCounter = 0;
bool isSuccesful = true;

const int windowSize = 5;
float data[windowSize];
int dataIndex = 0;
float average_temperature = 0.0;

TemperatureZero TempZero = TemperatureZero();

int packet_counter = 0;

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
  rf95.setTxPower(20, true);

  // rf95.setTxPower(14, false);
  startTimer(1);
}

void loop() {
  if (rf95.available()) {
    String received_message = receivedMessage();
    SerialUSB.println(received_message);
    Packet received_packet = packetParser(received_message);
    serialPrintPacket(received_packet);
    if (received_packet.type == "p" && received_packet.nodeId == "1"){
      String type = "s";
      String packet_id = String(packet_counter++);
      String sender_id = "None";
      String node_id = "1";
      String payload = "temp:" + String(average_temperature);
      long time_stamp = millis();

      Packet p = {type, packet_id, sender_id, node_id, payload, time_stamp};

      String message_ready = packetSetup(p);

      sendMessage(message_ready);
    }
  }
}

void sendMessage(String str) {
  SerialUSB.println("sent reply");
  uint8_t data_array[str.length()];
  str.getBytes(data_array, sizeof(data_array));
  rf95.send(data_array, sizeof(data_array));
}

String receivedMessage() {

  // Should be a message for us now
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  if (rf95.recv(buf, &len)) {
    String message = ((char*)buf);
    return message;
  } else
    SerialUSB.println("Recieve failed");
}

void TC4_Handler() {
  TcCount16* TC = (TcCount16*)TC4;
  // If this interrupt is due to the compare register matching the timer count
  // we toggle the LED.
  if (TC->INTFLAG.bit.MC0 == 1) {
    TC->INTFLAG.bit.MC0 = 1;
    // TODO: calulate the average temp
    float t = TempZero.readInternalTemperature();
    data[dataIndex] = t;
    dataIndex = (dataIndex + 1) % windowSize;
    float sum = 0;
    for (int i = 0; i < windowSize; i++){
      sum += data[i];
    }
    average_temperature = sum / windowSize;
    SerialUSB.print("average: = ");
    SerialUSB.println(average_temperature);
    
    }
  }
