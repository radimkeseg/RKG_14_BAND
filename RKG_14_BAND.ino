//   Project: 14 Band Spectrum Analyzer using WS2812B/SK6812
//   Target Platform: Arduino Mega2560 or Mega2560 PRO MINI  
//   The original code has been modified by PLATINUM to allow a scalable platform with many more bands.
//   It is not possible to run modified code on a UNO,NANO or PRO MINI. due to memory limitations.
//   The library Si5351mcu is being utilized for programming masterclock IC frequencies. 
//   Special thanks to Pavel Milanes for his outstanding work. https://github.com/pavelmc/Si5351mcu
//   Analog reading of MSGEQ7 IC1 and IC2 use pin A0 and A1.
//   Clock pin from MSGEQ7 IC1 and IC2 to Si5351mcu board clock 0 and 1
//   Si5351mcu SCL and SDA use pin 20 and 21
//   See the Schematic Diagram for more info
//   Programmed and tested by PLATINUM
//   Version 1.0    
//***************************************************************************************************

#include <Adafruit_NeoPixel.h>
#include <si5351mcu.h>    //Si5351mcu library
Si5351mcu Si;             //Si5351mcu Board

#define NOISE         50
#define ROWS          8  //num of row MAX=20
#define COLUMNS       14  //num of column

#define DATA_PIN      D7 //9   //led data pin

#define STROBE_PIN    D5 //6   //MSGEQ7 strobe pin
#define RESET_PIN     D6 //7   //MSGEQ7 reset pin

#define BAND_SWITCH   D3
#define ANALOG        A0

#define NUMPIXELS    ROWS * COLUMNS

//#define DEBUG
/*
// MSGEQ7
#include "MSGEQ7.h"
#define MSGEQ7_INTERVAL ReadsPerSecond(50)
#define MSGEQ7_SMOOTH true

CMSGEQ7<MSGEQ7_SMOOTH, RESET_PIN, STROBE_PIN, ANALOG> MSGEQ7;
*/
struct Point{
char x, y;
char  r,g,b;
bool active;
};
struct TopPoint{
int position;
int peakpause;
};
Point spectrum[ROWS][COLUMNS];
TopPoint peakhold[COLUMNS];
TopPoint spechold[COLUMNS];
int spectrumValue[COLUMNS];
long int counter = 0;
int long pwmpulse = 0;
bool toggle = false;
int long time_change = 0;
int effect = 0;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup() 
 {
 Serial.begin(115200L);
  
 Si.init(25000000L);
 Si.setFreq(0, 104570);
 Si.setFreq(1, 166280);
 Si.setPower(0, SIOUT_4mA);
 Si.setPower(1, SIOUT_4mA);
 Si.enable(0);
 Si.enable(1);
 
 pinMode      (STROBE_PIN,    OUTPUT);
 pinMode      (RESET_PIN,     OUTPUT);
 pinMode      (DATA_PIN,      OUTPUT);
 pinMode      (BAND_SWITCH,   OUTPUT);
 
 pixels.setBrightness(20); //set Brightness
 pixels.begin();
 pixels.show();
 pinMode      (STROBE_PIN, OUTPUT);
 pinMode      (RESET_PIN,  OUTPUT);  
 digitalWrite (RESET_PIN,  LOW);
 digitalWrite (STROBE_PIN, LOW);
 delay        (1);  
 digitalWrite (RESET_PIN,  HIGH);
 delay        (1);
 digitalWrite (RESET_PIN,  LOW);
 digitalWrite (STROBE_PIN, HIGH);
 delay        (1);
 }

void loop() 
{    
  counter++;   

  digitalWrite(RESET_PIN, HIGH);
  delayMicroseconds(3000);
  digitalWrite(RESET_PIN, LOW);
  
  for(int i=0; i < COLUMNS; i++){ 
    
  if(i%2){ //strobe once read twice
   digitalWrite(STROBE_PIN, LOW);
   delayMicroseconds(1000);
  }
  digitalWrite(BAND_SWITCH, (i%2) ? LOW : HIGH); //lower or higher half addresing FSA3157P6X
  
  spectrumValue[i] = analogRead(0);
#ifdef DEBUG  
    Serial.print(spectrumValue[i]); Serial.write(",");
#endif
//  spectrumValue[i] = log10(spectrumValue[i])/3*1023;
  spectrumValue[i] = map(spectrumValue[i], 120, 1023, 0, ROWS);
  

  if((i+1)%2) //strobe once read twice
    digitalWrite(STROBE_PIN, HIGH);  
    
  }
#ifdef DEBUG  
    Serial.println("");
#endif


  
  for(int j = 0; j < COLUMNS; j++){
    if(spectrumValue[j] > spechold[j].position)
     {
      spechold[j].position = spectrumValue[j];
      spechold[j].peakpause = spectrumValue[j]/2;
    }
    for(int i = 0; i < ROWS; i++){ 
      if(i<spechold[j].position){  
        spectrum[i][j].active = 1;
        spectrum[i][j].r =i*32;           //COLUMN Color red
        spectrum[i][j].g =255;         //COLUMN Color green
        spectrum[i][j].b =j*16;           //COLUMN Color blue
      }else{
        spectrum[i][j].active = 0;
        spectrum[i][j].r =0;           
        spectrum[i][j].g =0;         
        spectrum[i][j].b =0;           
      }
    }  

    if(spectrumValue[j] > peakhold[j].position)
     {
      peakhold[j].position = spectrumValue[j];
      peakhold[j].peakpause = spectrumValue[j]*4;
    }
    else
    {
      spectrum[peakhold[j].position][j].active = 1;
      spectrum[peakhold[j].position][j].r = 255;  //Peak Color red
      spectrum[peakhold[j].position][j].g = 0;  //Peak Color green
      spectrum[peakhold[j].position][j].b = 0;    //Peak Color blue
    }

  } 
 
  flushMatrix();

  if(counter > 3) { topSinking(); spectrumSinking(); counter=0; } //peak delay
}
  
void topSinking()
{
  for(int j = 0; j < COLUMNS; j++)
  {
    if(peakhold[j].position > 0 && peakhold[j].peakpause <= 0) peakhold[j].position--;
    else if(peakhold[j].peakpause > 0) peakhold[j].peakpause--;       
  } 
}

void spectrumSinking()
{
  for(int j = 0; j < COLUMNS; j++)
  {
    if(spechold[j].position > 0 && spechold[j].peakpause <= 0) spechold[j].position--;
    else if(spechold[j].peakpause > 0) spechold[j].peakpause--;       
  } 
 
}

void clearspectrum()
{
  for(int j = 0; j < COLUMNS; j++)
    for(int i = 0; i < ROWS; i++)
      spectrum[i][j].active = 0;  
}

void flushMatrix()
{
  for(int j = 0; j < COLUMNS; j++)
  {
    if( j % 2 != 0)
    {
      for(int i = 0; i < ROWS; i++)
      {
        if(spectrum[ROWS - 1 - i][j].active)
        {
          pixels.setPixelColor(j * ROWS + i, pixels.Color(
          spectrum[ROWS - 1 - i][j].r, 
          spectrum[ROWS - 1 - i][j].g, 
          spectrum[ROWS - 1 - i][j].b));         
        }
        else
        {
          pixels.setPixelColor( j * ROWS + i, 0, 0, 0);  
        } 
      }
    }
    else
    {
      for(int i = 0; i < ROWS; i++)
      {
        if(spectrum[i][j].active)
        {
          pixels.setPixelColor(j * ROWS + i, pixels.Color(
          spectrum[i][j].r, 
          spectrum[i][j].g, 
          spectrum[i][j].b));     
        }
        else
        {
          pixels.setPixelColor( j * ROWS + i, 0, 0, 0);  
        }
      }      
    } 
  }
  pixels.show();
}
  
