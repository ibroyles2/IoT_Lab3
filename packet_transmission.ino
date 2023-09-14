#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024

void startTimer4(int frequencyHz);
void setTimer4Frequency(int frequencyHz);
void TC4_Handler();

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
  

  // Setup WDT

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
}


void loop() {
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
      SerialUSB.println("5 seconds");
      sec = 0;
    }
    else {
      SerialUSB.println(String(sec) + " seconds");
    }
  } 
}
