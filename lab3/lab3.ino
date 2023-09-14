#include <TemperatureZero.h>
#include "packet.h"
#include <SPI.h>
#include <RH_RF95.h>

#define CPU_HZ 48000000

#define TIMER_PRESCALER_DIV 1024

#define NODE_ID "h"

void startTimer(int frequencyHz);
void setTimerFrequency(int frequencyHz);
void TC4_Handler();

RH_RF95 rf95(12, 6);

float frequency = 914;
float sumTemp = 0.0;
int tempCounter = 0;
bool isSuccesful = true;


String SUCCESSFULL_CODE = "s";
String ERROR_CODE = "e";

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
  }
  
}

void sendMessage(String str) {
  uint8_t data_array[str.length()];
  str.getBytes(data_array, sizeof(data_array));
  rf95.send(data_array, sizeof(data_array));
}

String getPacketId(int packetNum) {
  if (isSuccesful) {
    return SUCCESSFULL_CODE + String(packetNum);
  } else {
    return ERROR_CODE + String(packetNum);
  }
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

// TODO: calculate average temp
// float getAverageTemperature() {
//   float sumTemp = 0.0;
//   float t = ge;
//   sumTemp = sumTemp + t;
//   return sumTemp;
// }

void setTimerFrequency(int frequencyHz) {
  int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * frequencyHz)) - 1;
  TcCount16* TC = (TcCount16*)TC4;
  TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
  TC->CC[0].reg = compareValue;
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ;
  SerialUSB.print("COUNT: ");
  SerialUSB.println(TC->COUNT.reg);
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ;
  SerialUSB.print("CC[0]: ");
  SerialUSB.println(TC->CC[0].reg);
}

void startTimer(int frequencyHz) {
  REG_GCLK_CLKCTRL = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TC4_TC5);
  while (GCLK->STATUS.bit.SYNCBUSY == 1)
    ;  // wait for sync

  TcCount16* TC = (TcCount16*)TC4;

  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;  //Disable timer
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ;  // wait for sync

  // Use the 16-bit timer
  // Use match mode so that the timer counter resets when the count matches the compare register
  // Set prescaler to 1024
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1024;
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ;  // wait for sync

  setTimerFrequency(frequencyHz);

  // Enable the compare interrupt
  TC->INTENSET.reg = 0;
  TC->INTENSET.bit.MC0 = 1;

  NVIC_EnableIRQ(TC4_IRQn);

  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ;  // wait for sync
}

void TC4_Handler() {
  TcCount16* TC = (TcCount16*)TC4;
  // If this interrupt is due to the compare register matching the timer count
  // we toggle the LED.
  if (TC->INTFLAG.bit.MC0 == 1) {
    TC->INTFLAG.bit.MC0 = 1;
    float t = TempZero.readInternalTemperature();
    sumTemp = sumTemp + t;
    tempCounter++;
    SerialUSB.println(String(tempCounter) + " temperature = " + String(t));
    if (tempCounter == 5) {
      rf95.setTxPower(20, false);
      float averageTemp = sumTemp / 5.0;

      packet_counter++;
      float temperature = TempZero.readInternalTemperature();
      String packet_id = getPacketId(packet_counter);
      unsigned long time_stamp = millis();

      TemperaturePacket p = { packet_id, NODE_ID, temperature, time_stamp };

      String message = packetSetup(p);

      sendMessage(message);

      SerialUSB.println(message);


      SerialUSB.println("succesfull");
      sumTemp = 0.0;
      tempCounter = 0;
    }
  }
}
