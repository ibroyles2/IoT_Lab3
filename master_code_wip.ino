 #include <SPI.h>
    
//Radio Head Library:
#include <RH_RF95.h>
#include <TemperatureZero.h>

# define CPU_HZ 48000000
# define TIMER_PRESCALER_DIV 512

void startTimer(int frequencyHz);
void setTimerFrequency(int frequencyHz);
void TC3_Handler();

TemperatureZero TempZero = TemperatureZero();
RH_RF95 rf95(12, 6);

float frequency = 14;
int packetCounter = 0;
int timerCounter = 0; // increments up to 5 by interupt handler, then reset
float temperatureArray[5];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  TempZero.init();
  while(!SerialUSB)
  pinMode(PIN_LED_13, OUTPUT);
  startTimer(1);

  if (rf95.init() == false) {
    SerialUSB.println("Radio Init Failed - Freezing");
    while (1);
  } else {
    SerialUSB.println("Transmitter up!");
    digitalWrite(PIN_LED_13, HIGH);
    delay(500);
    digitalWrite(PIN_LED_13, LOW);
    delay(500);
  }

  rf95.setFrequency(frequency);
    
  // Transmitter power can range from 14-20dbm.
  // Since we're close together, we can use the lowest value (14)
  rf95.setTxPower(14, false);
}

void loop() {
  // put your main code here, to run repeatedly:

  // wait_for_permission_packet()
  // send_temperature_packet()
  // wait_for_confirmation_packet()

  delay(1000);
  digitalWrite(PIN_LED_13, HIGH);
  float temperature = TempZero.readInternalTemperature();
  // SerialUSB.print("Internal Temperature:");
  // SerialUSB.println(temperature);
  packetCounter = packetCounter + 1;
  char* toSend;
  sprintf(toSend, "I1,%d,%ld,%f", packetCounter, millis(), temperature);
  // itoa(packetCounter, toSend + strlen(toSend), 10);
  // itoa(timeSinceLastPacket, toSend + strlen(toSend), 10)

  //Concatenate the packet counter value to the message
  SerialUSB.println(toSend);
  SerialUSB.println(packetCounter);
  digitalWrite(PIN_LED_13, LOW);

}

void setTimerFrequency(int frequencyHz) {
  int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * frequencyHz)) - 1;
  TcCount16* TC = (TcCount16*) TC3;
  // Make sure the count is in a proportional position to where it was
  // to prevent any jitter or disconnect when changing the compare value.
  // map(value, fromLow, fromHigh, toLow, toHigh) takes a value in a range of [fromLow,fromHigh] to a value in a range of [toLow,toHigh]
  TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[1].reg, 0, compareValue);
  TC->CC[1].reg = compareValue;
  while (TC->STATUS.bit.SYNCBUSY == 1);

  SerialUSB.print("COUNT: ");
  SerialUSB.println(TC->COUNT.reg);
  while (TC->STATUS.bit.SYNCBUSY == 1);

  SerialUSB.print("CC[1]: ");
  SerialUSB.println(TC->CC[1].reg);
}

void startTimer (int frequencyHz) {
  REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | 
                                 GCLK_CLKCTRL_GEN_GCLK0 |
                                 GCLK_CLKCTRL_ID_TC4_TC5) ;
  while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync

  TcCount16* TC = (TcCount16*) TC4;
  
  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE; //Disable timer
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
  
  // Use the 16-bit timer
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use match mode so that the timer counter resets when the count matches the compare register
  TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Set prescaler to 1024
  TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  setTimerFrequency(frequencyHz);
  
  // Enable the compare interrupt
  TC->INTENSET.reg = 0;
  TC->INTENSET.bit.OVF = 1;
  
  NVIC_EnableIRQ(TC4_IRQn);
  
  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
}


void TC3_handler() {
  // every 5 seconds send permissions
  // error codes: WDT, REC_FAIL ( reception failure ),
  // do interrupt stuff:

  // sending permission message :
  float temperature = TempZero.readInternalTemperature();
  temperatureArray[timerCounter] = temperature;
  SerialUSB.print("Current temperature");
  SerialUSB.println(temperature);

  if (timerCounter > 4) {
    packetCounter ++;
    char* permissionPacket;
    sprintf(permissionPacket, "p%d,I1,%ld,%2.2f", packetCounter, millis(), temperature);
    // rf95.send((uint8_t *) &permissionPacket,sizeof(permissionPacket));
    // rf95.waitPacketSent();
    SerialUSB.println(permissionPacket);

    // checks if packet was recieved:
    if( 1 == 1 ) {
      SerialUSB.println("Permission packet recieved by slave");
      SerialUSB.println("Temperature packet recieved from slave");
    }
    timerCounter = 0;
  } 
  timerCounter++;
  

}

float averageTemp(float* tempArray) {
  float sumTemp = 0;
  int size = sizeof(tempArray) / sizeof(float);
  for (int i = 0; i < size; i++ ) {
    sumTemp += tempArray[i];
  }
  return sumTemp / size;
}

// void slave_node {
//   // calc_temp()
//   // check for confirmation signal
//   // if no signal comes
// }
