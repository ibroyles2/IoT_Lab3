  #define CPU_HZ 48000000
  #define TIMER_PRESCALER_DIV 1024

  void startTimer4(int frequencyHz);
  void setTimer4Frequency(int frequencyHz);
  void TC4_Handler();

  #include <SPI.h>
  
  //Radio Head Library:
  #include <RH_RF95.h>
  
  // We need to provide the RFM95 module's chip select and interrupt pins to the
  // rf95 instance below.On the SparkFun ProRF those pins are 12 and 6 respectively.
  RH_RF95 rf95(12, 6);
  
  int LED = 13; //Status LED is on pin 13
  
  int packetCounter = 0; //Counts the number of packets sent
  long timeSinceLastPacket = 0; //Tracks the time stamp of last packet received
  
  // The broadcast frequency is set to 921.2, but the SADM21 ProRf operates
  // anywhere in the range of 902-928MHz in the Americas.
  // Europe operates in the frequencies 863-870, center frequency at 868MHz.
  // This works but it is unknown how well the radio configures to this frequency:
  float frequency = 914; //Broadcast frequency
  int sec = 0;

  void setup() { 
    // Setup General Clock Generator 2

    GCLK->GENDIV.bit.ID = 0x2;                // select clock generator 2
    while (GCLK->STATUS.bit.SYNCBUSY == 1);   
    GCLK->GENDIV.bit.DIV = 0x0010;            // set division factor of clock generator 2 to 16, using bits 8-12. 
    while (GCLK->STATUS.bit.SYNCBUSY == 1);
    
    GCLK->GENCTRL.bit.ID = 0x2;               // set clock to conifgure to clock generator 2
    while (GCLK->STATUS.bit.SYNCBUSY == 1);
    GCLK->GENCTRL.bit.GENEN = 0x1;            // enable clock generator
    while (GCLK->STATUS.bit.SYNCBUSY == 1);
    GCLK->GENCTRL.bit.SRC = 0x03;             // set clock source to (ULP Oscillator)
    while (GCLK->STATUS.bit.SYNCBUSY == 1);
    GCLK->GENCTRL.bit.DIVSEL = 0;             // set division style of clock to: clock freq / division factor
    while (GCLK->STATUS.bit.SYNCBUSY == 1);

    GCLK->CLKCTRL.bit.ID = 0x03;              // set clock to WDT's clock, so that the WDT will use it
    while (GCLK->STATUS.bit.SYNCBUSY == 1);
    GCLK->CLKCTRL.bit.GEN = 0x2;              // selected clock generator 2
    while (GCLK->STATUS.bit.SYNCBUSY == 1);
    GCLK->CLKCTRL.bit.CLKEN = 0x1;            // enabled clock
    while (GCLK->STATUS.bit.SYNCBUSY == 1);
    

    // Setup WDT with a period of 2 seconds
    // also setting up WDT Early Warning Interrupt
    WDT->CONFIG.bit.PER = 0x9;                // set timeout period of WDT to 4096 cycles (~2 seconds)
    while (WDT->STATUS.bit.SYNCBUSY == 1);

    WDT->EWCTRL.bit.EWOFFSET = 0x6;           // set WDT EW offset to 512 cycles (1/4 second)
    while (WDT->STATUS.bit.SYNCBUSY == 1);

    WDT->INTENSET.bit.EW = 0x1;               // enable WDT Early Warning interrupt
    
    while (WDT->STATUS.bit.SYNCBUSY == 1);
    WDT->CTRL.bit.ENABLE = 0x1;               // enabled WDT
    while (WDT->STATUS.bit.SYNCBUSY == 1);

    
    // put your setup code here, to run once:
    NVIC_EnableIRQ(WDT_IRQn);
    SerialUSB.begin(9600);
    while(!SerialUSB);
    startTimer4(1);


    //Initialize the Radio.
        if (rf95.init() == false) {
          SerialUSB.println("Radio Init Failed - Freezing");
          while (1);
        }
        else {
          //An LED inidicator to let us know radio initialization has completed.
          SerialUSB.println("Receiver up!");
        }
      
        // Set frequency
          rf95.setFrequency(frequency);
      
        // Transmitter power can range from 14-20dbm.
        rf95.setTxPower(14, true);
  }


  void loop() {
    if (rf95.available()){
        // Should be a message for us now
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
    
        if (rf95.recv(buf, &len)){
          digitalWrite(LED, HIGH); //Turn on status LED
          timeSinceLastPacket = millis(); //Timestamp this packet
    
          SerialUSB.print("Got message: ");
          SerialUSB.print((char*)buf);
          SerialUSB.print(" RSSI: ");
          SerialUSB.print(rf95.lastRssi(), DEC);
          SerialUSB.println();

         rf95.setTxPower(14, false);              // set this to false so that we can send messages
         // Send a reply
         uint8_t toSend[] = "Hello From the big computer!"; 
         rf95.send(toSend, sizeof(toSend));
         rf95.waitPacketSent();
         SerialUSB.println("Sent a reply");
         digitalWrite(LED, LOW); //Turn off status LED
         rf95.setTxPower(14, true);               // set this to true so that we can receive messages
    
        }
        else
          SerialUSB.println("Recieve failed");
      }
      //Turn off status LED if we haven't received a packet after 1s
      if(millis() - timeSinceLastPacket > 1000){
        digitalWrite(LED, LOW); //Turn off status LED
        timeSinceLastPacket = millis(); //Don't write LED but every 1s
      }

  }

  void setTimer4Frequency(int frequencyHz) {
    int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * frequencyHz)) - 1;
    TcCount16* TC = (TcCount16*) TC4;
    // Make sure the count is in a proportional position to where it was
    // to prevent any jitter or disconnect when changing the compare value.
    // map(value, fromLow, fromHigh, toLow, toHigh) takes a value in a range of [fromLow,fromHigh] to a value in a range of [toLow,toHigh]
    TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
    TC->CC[0].reg = compareValue;
    while (TC->STATUS.bit.SYNCBUSY == 1);
    while (TC->STATUS.bit.SYNCBUSY == 1);
  }



  void startTimer4(int frequencyHz) {
  REG_GCLK_CLKCTRL = (uint16_t)(GCLK_CLKCTRL_CLKEN | 
                                GCLK_CLKCTRL_GEN_GCLK0 |
                                GCLK_CLKCTRL_ID_TC4_TC5) ;
    while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync

    TcCount16* TC = (TcCount16*) TC4;
    
    TC->CTRLA.reg &= ~TC_CTRLA_ENABLE; //Disable timer
    while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
    
    // Use the 16-bit timer
    // Use match mode so that the timer counter resets when the count matches the compare register
    // Set prescaler to 1024
    TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1024;
    while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

    setTimer4Frequency(frequencyHz);
    
    // Enable the compare interrupt
    TC->INTENSET.reg = 0;
    TC->INTENSET.bit.MC0 = 1;
    TC->INTENSET.bit.OVF = 1;
    NVIC_EnableIRQ(TC4_IRQn);
    
    TC->CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
  }





  // WDT EW Interrupt Handler
  void WDT_Handler() {
    WDT->INTFLAG.reg = WDT_INTFLAG_EW;
    WDT->CLEAR.bit.CLEAR = 0xA5;
  }

  void TC4_Handler() {
    TcCount16* TC = (TcCount16*) TC4;

    if (TC->INTFLAG.bit.OVF == 1) {
      TC->INTFLAG.bit.OVF = 1;
      sec++;
      if (sec > 4) {
        sec = 0;
      }
      else {
        SerialUSB.println(String(sec) + " seconds");
      }
    } 
  }
