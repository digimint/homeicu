/*---------------------------------------------------------------------------------
  ADS1292R driver - hardware for ECG

Inspired by:
Healthy Pi V4 project
https://github.com/ericherman/eeg-mouse
http://www.mccauslandcenter.sc.edu/CRNL/ads1298


SCLK   |  Device attempts to decode and execute commands every eight serial clocks. 
       |  It is recommended that multiples of 8 SCLKs be presented every serial transfer 
       |  to keep the interface in a normal operating mode.
       |
DRDY   |  low = new data are available, regardless of CS
       |
START  |  keep low if using start opcode. The START pin or the START command is used to 
       |  place the device either in normal data capture mode or pulse data capture mode.
       |
PWDN   |  When PWDN is pulled low, all on-chip circuitry is powered down
       |
CS     |  low = SPI is active, must remain low during communication, then wait 4-5 tCLK 
       |  cycles before returning to high, set CS high then low to reset the communication
RESET  |  low = force a reset, RESET is automatically issued to the digital filter whenever 
       |  registers CONFIG1 and RESP are set to new values with a WREG command
       |
CLKSEL |  internal clock

SPI library
the spi clock divider (setClockDivider) sets relative to the system clock, the frequency of SLK 
relative to the chip frequency. it needs to be at least 32, or the clock is too slow, 64 to be safe

Quote from http://www.mit.edu/~gari/CODE/ECG_lab/ecg_ads1292.ino
----------------------------------------------------------------------

Another solution is to use ADS1293, but ADS1293 and ADS1292R has very little in common on the 
interface and register map definition.

-ADS1292 has digital lead-off detection. ADS1293 has both analog and digital lead-off detection
-ADS1293 can generate Wilson or Goldberger References.
-ADS1293 has a flexible routing switch that can connect any input pin to any channel. 
-ADS1292 has assigned input pins per channel
-ADS1293 has lower power-consumption per channel and has shutdown control bits for individual blocks
-ADS1293’s SPI has a read back feature that can read a specific register
-ADS1293 diagnostics is interrupt driven (alarm pin). ADS1293 polls error flags to detect diagnostic condition
-ADS1293 does not have a respiration channel, ADS1292R can do respiration measurement. 
Quote from "https://e2e.ti.com/support/data-converters/f/73/t/300611?1292-vs-1293"

Other IC for ECG : MAX30001 ~ MAX30004
https://www.maximintegrated.com/en/design/technical-documents/app-notes/6/6952.html


The tri-wavelength MAX30101 sensor and bi-wavelength MAX30102 sensor were evaluated on the forehead, in a truly non-obtrusive wearable friendly position. Heart rate, respiration rate and blood oxygen saturation were extracted from both sensors and compared with a FDA/TGA/CE approved photoplethysmography device placed on the finger. All data were captured simultaneously and at rest. The MAX30101 sensor was more accurate in measuring heart rate, blood oxygen saturation and respiration rate compared to the MAX30102. 


---------------------------------------------------------------------------------*/
#include "Arduino.h"
#include <SPI.h>
#include "firmware.h" 

#define CONFIG_SPI_MASTER_DUMMY 0xFF

// Register Read Commands
#define RREG    0x20    //Read n nnnn registers starting at address r rrrr
                        //first byte 001r rrrr (2xh)(2) - second byte 000n nnnn(2)
#define WREG    0x40    //Write n nnnn registers starting at address r rrrr
                        //first byte 010r rrrr (2xh)(2) - second byte 000n nnnn(2)
#define START   0x08    //Start/restart (synchronize) conversions
#define STOP    0x0A    //Stop conversion
#define RDATAC  0x10    //Enable Read Data Continuous mode.

//This mode is the default mode at power-up.
#define SDATAC  0x11    //Stop Read Data Continuously mode
#define RDATA   0x12    //Read data by command; supports multiple read back.

//register address
#define ADS1292R_REG_ID        0x00
#define ADS1292R_REG_CONFIG1   0x01
#define ADS1292R_REG_CONFIG2   0x02
#define ADS1292R_REG_LOFF      0x03
#define ADS1292R_REG_CH1SET    0x04
#define ADS1292R_REG_CH2SET    0x05
#define ADS1292R_REG_RLDSENS   0x06
#define ADS1292R_REG_LOFFSENS  0x07
#define ADS1292R_REG_LOFFSTAT  0x08
#define ADS1292R_REG_RESP1     0x09
#define ADS1292R_REG_RESP2     0x0A

#define HISTGRM_CALC_TH               10

uint32_t  heart_rate_histogram[HISTGRM_DATA_SIZE];
uint8_t   histogram_percent   [HISTGRM_PERCENT_SIZE];
uint8_t   hr_percent_count    = 0;
uint8_t   hrv_array[HVR_ARRAY_SIZE];
uint8_t   heart_rate_pack  [3];
bool      ecgBufferReady      = false;
bool      hrvDataReady        = false;
bool      histogramReady      = false;
bool      heartRateReady      = false;

volatile uint8_t  heart_rate  = 0;
volatile uint8_t  npeakflag   = 0;
volatile uint8_t  heart_rate_prev = 0;
volatile uint8_t  respirationRate = 0;
volatile bool     ads1292r_interrupt_flag   = false;

ADS1292R          ads1292r;
ADS1292Data       ads1292r_raw_data;
ADS1292Process    ecg_respiration_algorithm; 

uint8_t   lead_flag = 0;
uint8_t   ecg_data_buff[ECG_BUFFER_SIZE];
int16_t   ecg_wave_sample,  ecg_filterout ;
int16_t   res_wave_sample,  resp_filterout;
uint16_t  ecg_stream_cnt = 0;

void IRAM_ATTR ads1292r_interrupt_handler(void)
{
  portENTER_CRITICAL_ISR(&ads1292rMux);
  ads1292r_interrupt_flag = true;
  portEXIT_CRITICAL_ISR (&ads1292rMux);  
}

//FIXME too many delays
void ADS1292R :: init(void)
{
  SPI.setDataMode(SPI_MODE1);

  // start the SPI library:
  pin_high_time(ADS1292R_PWDN_PIN,100);
  pin_low_time (ADS1292R_PWDN_PIN,100);
  pin_high_time(ADS1292R_PWDN_PIN,100);
  
  pin_low_time (ADS1292R_START_PIN,20);       // disable Start
  pin_high_time(ADS1292R_START_PIN,20);       // enable Start
  pin_low_time (ADS1292R_START_PIN,100);      //  hard Stop 
  
  SendCommand(START); // Send 0x08, start data conv
  SendCommand(STOP);  // Send 0x0A, soft stop
  delay(50);
  SendCommand(SDATAC);// Send 0x11, stop read data continuous
  delay(300);
  WriteRegister(ADS1292R_REG_CONFIG1,      0x00);  //Set sampling rate to 125 SPS
  WriteRegister(ADS1292R_REG_CONFIG2,0b10100000);	//Lead-off comp off, test signal disabled
  WriteRegister(ADS1292R_REG_LOFF,   0b00010000);	//Lead-off defaults
  WriteRegister(ADS1292R_REG_CH1SET, 0b01000000);	//Ch 1 enabled, gain 6, connected to electrode in
  WriteRegister(ADS1292R_REG_CH2SET, 0b01100000);	//Ch 2 enabled, gain 6, connected to electrode in
  WriteRegister(ADS1292R_REG_RLDSENS,0b00101100);	//RLD settings: fmod/16, RLD enabled, RLD inputs from Ch2 only
  WriteRegister(ADS1292R_REG_LOFFSENS,     0x00);	//LOFF settings: all disabled
  WriteRegister(ADS1292R_REG_RESP1,  0b11110010);	//Respiration: MOD/DEMOD turned only, phase 0
  WriteRegister(ADS1292R_REG_RESP2,  0b00000011);	//Respiration: Calib OFF, respiration freq defaults
  //Skip register 8, LOFF Settings default

  SendCommand(RDATAC);					              // Send 0x10, Start Read Data Continuous
  delay(10);

  pin_high_time (ADS1292R_START_PIN,20);                // enable Start
}

void ADS1292R :: getData()
{
  uint8_t     LeadStatus  = 0;
  signed long secgtemp    = 0;
  long        status_byte = 0;

  unsigned long uecgtemp  = 0;
  unsigned long resultTemp= 0;

  // Sampling rate is set to 125SPS ,DRDY ticks for every 8ms
  if (ads1292r_interrupt_flag==false)
    return;   //wait data to be ready

  // processing the data
  SPI.setDataMode(SPI_MODE1);

  portENTER_CRITICAL_ISR(&ads1292rMux);
  ads1292r_interrupt_flag = false;
  portEXIT_CRITICAL_ISR (&ads1292rMux);  

  ReadToBuffer(); // Read the data to SPI_ReadBuffer

  uecgtemp = (unsigned long) (((unsigned long)SPI_ReadBuffer[3] << 16)|  \
                              ((unsigned long)SPI_ReadBuffer[4] << 8) |  \
                                (unsigned long)SPI_ReadBuffer[5]);
  uecgtemp = (unsigned long)(uecgtemp << 8);
  secgtemp = (signed long)  (uecgtemp);
  ads1292r_raw_data.raw_resp    = secgtemp;

  uecgtemp = (unsigned long)(((unsigned  long)SPI_ReadBuffer[6] << 16) | \
                              ((unsigned long)SPI_ReadBuffer[7] <<  8) | \
                                (unsigned long)SPI_ReadBuffer[8]);
  
  uecgtemp = (unsigned long)(uecgtemp << 8);
  secgtemp = (signed long)  (uecgtemp);
  secgtemp = (signed long)  (secgtemp >> 8);
  ads1292r_raw_data.raw_ecg     = secgtemp;

  status_byte = (long)((long)SPI_ReadBuffer[2] |      \ 
                      ((long)SPI_ReadBuffer[1]) <<8 | \
                      ((long) SPI_ReadBuffer[0])<<16); // First 3 bytes represents the status
  
  status_byte  = (status_byte & 0x0f8000) >> 15;  // bit15 gives the lead status
  LeadStatus = (uint8_t ) status_byte ;
  ads1292r_raw_data.status_reg = LeadStatus;

  /////////////////////////////////////
  // ignore the lower 8 bits out of 24bits 
  ecg_wave_sample = (int16_t)(ads1292r_raw_data.raw_ecg  >> 8);  
  res_wave_sample = (int16_t)(ads1292r_raw_data.raw_resp >> 8);

  if (!((ads1292r_raw_data.status_reg & 0x1f) == 0))
  { // measure lead is OFF the body 
    lead_flag         = 0;
    ecg_filterout     = 0;
    resp_filterout    = 0;      
  }  
  else
  { // the measure lead is ON the body 
    lead_flag         = 1;
    // filter out the line noise @40Hz cutoff 161 order
    ecg_respiration_algorithm.Filter_CurrentECG_sample  (&ecg_wave_sample,&ecg_filterout);   
    ecg_respiration_algorithm.Calculate_HeartRate       (ecg_filterout,   &heart_rate,  &npeakflag); 
    ecg_respiration_algorithm.Filter_CurrentRESP_sample (res_wave_sample, &resp_filterout);
    ecg_respiration_algorithm.Calculate_RespRate        (resp_filterout,  &respirationRate);   
    
    if(heart_rate_prev != heart_rate)
    {
      heart_rate_pack[0] = heart_rate;        //from ads1292r
      heart_rate_pack[1] = (uint8_t) heart_rate_from_oximeter; 
      heart_rate_pack[2] = lead_flag; 
      heart_rate_prev = heart_rate;
    }  

    
    if(npeakflag == 1)
    {
      fillTxBuffer(heart_rate, respirationRate);
      add_heart_rate_histogram(heart_rate);
      npeakflag = 0;
    }
  
    ecg_data_buff[ecg_stream_cnt++] = (uint8_t)ecg_wave_sample; //ecg_filterout;
    ecg_data_buff[ecg_stream_cnt++] = (ecg_wave_sample>>8);     //ecg_filterout>>8;
    
    if(ecg_stream_cnt >=ECG_BUFFER_SIZE)
    {
      ecgBufferReady = true;
      ecg_stream_cnt = 0;
    }
  }
  /////////////////////////////////////
}

void ADS1292R :: ReadToBuffer(void)
{
  pin_low_time (ADS1292R_CS_PIN,0);

  for (int i = 0; i < 9; ++i)
    SPI_ReadBuffer[i] = SPI.transfer(CONFIG_SPI_MASTER_DUMMY);
  
  pin_high_time(ADS1292R_CS_PIN,0);
}
 
void ADS1292R :: SendCommand(uint8_t data_in)
{
  pin_low_time (ADS1292R_CS_PIN,2);
  pin_high_time(ADS1292R_CS_PIN,2);
  pin_low_time (ADS1292R_CS_PIN,2);

  SPI.transfer(data_in);

  pin_high_time(ADS1292R_CS_PIN,2);
}

void ADS1292R :: WriteRegister(uint8_t READ_WRITE_ADDRESS, uint8_t DATA)
{
  switch (READ_WRITE_ADDRESS)
  {
    case 1:
            DATA = DATA & 0x87;
	          break;
    case 2:
            DATA = DATA & 0xFB;
	          DATA |= 0x80;
	          break;
    case 3:
      	    DATA = DATA & 0xFD;
      	    DATA |= 0x10;
      	    break;
    case 7:
      	    DATA = DATA & 0x3F;
      	    break;
    case 8:
    	      DATA = DATA & 0x5F;
	          break;
    case 9:
      	    DATA |= 0x02;
      	    break;
    case 10:
      	    DATA = DATA & 0x87;
      	    DATA |= 0x01;
      	    break;
    case 11:
      	    DATA = DATA & 0x0F;
      	    break;
    default:
            break;
  }
  pin_low_time (ADS1292R_CS_PIN,2);
  pin_high_time(ADS1292R_CS_PIN,2);
  
  pin_low_time (ADS1292R_CS_PIN,2);    // select the device

  SPI.transfer(uint8_t (READ_WRITE_ADDRESS | WREG)); //Send register location
  SPI.transfer(0x00);		    //number of register to wr
  SPI.transfer(DATA);		    //Send value to record into register
  delay(2);

  pin_high_time (ADS1292R_CS_PIN,10);  // de-select the device
}
//FIXME check delays
void ADS1292R :: pin_high_time(int pin, uint32_t ms)
{
  digitalWrite(pin, HIGH);
  delay(ms);
}

void ADS1292R :: pin_low_time(int pin, uint32_t ms)
{
  digitalWrite(pin, LOW);
  delay(ms);
}
/*--------------------------------------------------------------------------------- 
 heart rate variability (HRV)
---------------------------------------------------------------------------------*/
#define ARRAY_SIZE                           20

uint8_t* ADS1292R :: fillTxBuffer(uint8_t peakvalue,uint8_t respirationRate)
{  
  unsigned int array[ARRAY_SIZE];
  int rear = -1;
  int sqsum;
  int k=0;
  int count = 0;
  int min_f=0;
  int max_f=0;
  int max_t=0;
  int min_t=0;
  volatile unsigned int RR;

  int meanval;

  float sdnn_f;
  float mean_f;
  float rmssd_f;
  float pnn_f;

  uint16_t sdnn;
  uint16_t pnn;
  uint16_t rmsd;
  RR = peakvalue;
  k++;

  if(rear == ARRAY_SIZE-1)
  {
    for(int i=0;i<(ARRAY_SIZE-1);i++)
      array[i]=array[i+1];
    array[ARRAY_SIZE-1] = RR;   
  }
  else
  {     
    rear++;
    array[rear] = RR;
  }

  if(k>=ARRAY_SIZE)
  { 
    // 1. HRVMAX
    for(int i=0;i<ARRAY_SIZE;i++)
    {
      if(array[i]>max_t)
        max_t = array[i];
    }
    max_f = max_t;

    // 2. HRVMIN
    min_t = max_f;
    for(int i=0;i<ARRAY_SIZE;i++)
    {
      if(array[i]< min_t)
        min_t = array[i]; 
    }
    min_f = min_t;

    // 3. mean
    { 
      int sum = 0;
      float mean_rr;
      for(int i=0;i<(ARRAY_SIZE);i++)
        sum = sum + array[i];
      mean_rr = (((float)sum)/ ARRAY_SIZE);  
      mean_f = mean_rr;
    } 

    // 3. sdnn_ff
    {
      int sumsdnn = 0;
      int diff;
      for(int i=0;i<(ARRAY_SIZE);i++)
      {
        diff = (array[i]-(mean_f));
        diff = diff*diff;
        sumsdnn = sumsdnn + diff;   
      }
      sdnn_f = sqrt(sumsdnn/(ARRAY_SIZE));
    }

    // 4. pnn_ff
    { 
      unsigned int pnn50[ARRAY_SIZE];
      long l;

      count = 0;
      sqsum = 0;

      for(int i=0;i<(ARRAY_SIZE-2);i++)
      {
        l = array[i+1] - array[i];       //array[] is unsigned integer 0-65535, l = -65535 ~ +65535
        pnn50[i]= abs(l);                //abs() return 0~65535
        sqsum = sqsum + (pnn50[i]*pnn50[i]);

        if(pnn50[i]>50)
          count = count + 1;    
      }
      pnn_f = ((float)count/ARRAY_SIZE)*100;

    // 5. rmssd_ff
      rmssd_f = sqrt(sqsum/(ARRAY_SIZE-1));
    }
    ////////////////////////////////////////

    meanval = mean_f *100;
    sdnn    = sdnn_f *100;
    pnn     = pnn_f  *100;
    rmsd    = rmssd_f*100;

    hrv_array[0]= meanval;
    hrv_array[1]= meanval>>8;
    hrv_array[2]= meanval>>16;
    hrv_array[3]= meanval>>24;
    hrv_array[4]= sdnn;
    hrv_array[5]= sdnn>>8;
    hrv_array[6]= pnn;
    hrv_array[7]= pnn>>8;
    hrv_array[10]=rmsd;
    hrv_array[11]=rmsd>>8;
    hrv_array[12]=respirationRate;
    hrvDataReady = true;
  }
}

void ADS1292R :: add_heart_rate_histogram(uint8_t hr)
{
  uint8_t   index = hr/10;
  uint32_t  sum   = 0;

  heart_rate_histogram[index-4]++;

  if(hr_percent_count++ > HISTGRM_CALC_TH)
  {
    hr_percent_count = 0;

    for(int i = 0; i < HISTGRM_DATA_SIZE; i++)
      sum += heart_rate_histogram[i];

    if(sum != 0)
    {
      for(int i = 0; i < HISTGRM_DATA_SIZE/4; i++)
      {
        uint32_t percent = ((heart_rate_histogram[i] * 100) / sum);
        histogram_percent[i] = percent;
      }
    }
    histogramReady = true;
  }
}
/*---------------------------------------------------------------------------------
 fake ecg data for testing
---------------------------------------------------------------------------------*/
#if ECG_BLE_TEST
const uint8_t fakeEcgSample[180] = { \
223, 148, 148,  30, 178, 192, 214, 184, 180, 162, \
168, 176, 229, 152, 182, 112, 184, 218, 231, 203, \
185, 168, 177, 181, 225, 170,  99, 184, 201, 228, \
207, 185, 164, 165, 170, 178, 241, 165, 122, 174, \
189, 216, 203, 189, 172, 176, 184, 223, 165, 168, \
104, 190, 222, 225, 200, 189, 194, 194, 246, 189, \
160, 137, 241, 239, 238, 196, 174, 160, 161, 186, \
153, 164,  43, 180, 205, 221, 185, 182, 166, 168, \
178, 228, 170, 155,  40, 192, 205, 225, 185, 170, \
147, 155, 164, 165, 186, 152,  69, 180, 190, 216, \
193, 180, 162, 153, 161, 182, 148, 147,  12, 177, \
194, 207, 168, 165, 146, 156, 160, 216, 142, 160, \
 79, 182, 208, 203, 188, 176, 176, 178, 211, 176, \
177,  76, 211, 245, 222, 204, 189,  39, 169, 181, \
148, 169, 182, 196, 194, 207, 205, 214, 209, 214, \
209,  23, 197, 138, 215, 226, 246, 243, 226, 218, \
190, 208, 211,  17, 203, 209, 151, 236,   3,  14, \
223, 211, 197, 201, 204, 208, 216, 188,  94, 211, \
};

void ADS1292R :: getTestData(void)
{
  static uint8_t index=0;
  if (ecgBufferReady == false )     // refill the data
  {
    // build the data when the function called at the first time 
    for(int i=0;i<ECG_BUFFER_SIZE;i++)
      {
        ecg_data_buff[i] = fakeEcgSample[index];
        index++;
        if (index == 180) 
          index = 0;
      }
    ecgBufferReady = true; 
 }
}
#endif //ECG_BLE_TEST