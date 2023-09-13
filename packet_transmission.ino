#include <SPI.h>
#include <RH_RF95.h>
#include <Arduino.h>
#include <TemperatureZero.h>

TemperatureZero TempZero = TemperatureZero();

RH_RF95 rf95(12, 6);
int LED = 13;
int packetCounter = 0;
long timeSinceLastPacket = 0;
float frequency = 914;

void setup() {
  pinMode(LED, OUTPUT);
  SerialUSB.begin(9600);
  
  if (rf95.init() == false) {
    SerialUSB.println("Radio Init Failed");
    while (1);
  }
  
  rf95.setFrequency(frequency);
  rf95.setTxPower(20, false);
  TempZero.init();

}

unsigned long lastReadingTime = 0; 
unsigned long lastSendingTime = 0; 
float tempSum = 0;  
int tempCount = 0;  
int PackageCounter = 1;
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastReadingTime >= 1000) {
    lastReadingTime = currentMillis;

    float currentTemp = TempZero.readInternalTemperature();
    //SerialUSB.print("currentTemp: ");
    //SerialUSB.println(currentTemp);

    tempSum += currentTemp;
    tempCount++;
  }

  if (currentMillis - lastSendingTime >= 5000) {
    lastSendingTime = currentMillis;

    if (tempCount > 0) {
      float avgTemp = tempSum / tempCount;
      //SerialUSB.print("avgTemp: ");
      //SerialUSB.println(avgTemp);

      unsigned long timeStamp = millis();
      
      char toSend[100];
      snprintf(toSend, sizeof(toSend), "From Karnav :%ds,a,%.2f,%lu", PackageCounter, avgTemp, timeStamp);

      SerialUSB.println(toSend);
      rf95.send((uint8_t *)toSend, strlen(toSend));
      
      tempSum = 0;
      tempCount = 0;
      PackageCounter++;
    }
  }
}


