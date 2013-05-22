#include "LPD8806.h"
#include <SPI.h>

/*****************************************************************************/
// LED Array Control Code by Overhauser
// LPD8806 library by Adafruit Industries
// SPI additions by cjbaar
// Ensure that matrixSizeX, matrixSizeY and number of pixels in 
// LPD8806() instantiation are set to your array resolution.
/*****************************************************************************/


// Set the size of the matrix here.
const int matrixSizeX = 6;
const int matrixSizeY = 64;


// Set the parameter to the NUMBER of pixels. Should equal matrixSizeX * matrixSizeY.
// Storing the states of the LEDs requires 3 bytes per LED. The most common failure is 
// having insufficient memory for the size of the array. Using SRAM is necessary as 
// EEPROM and Flash are far slower. 

// Below is a list of maximum LED capacities for various boards.
// For very large arrays (2000+) we recommend the BeagleBone or Raspberry Pi

// Arduino Mega 2560  - 333 LEDs
// Arduino Due        - 4000 LEDs
// Teensy 3.0         - 680 LEDs
LPD8806 strip = LPD8806(384);
#define PI 3.14159265
#define dist(a, b, c, d) sqrt(double((a - c) * (a - c) + (b - d) * (b - d)))


// Gamma correction compensates for our eyes' nonlinear perception of
// intensity.  It's the LAST step before a pixel value is stored, and
// allows intermediate rendering/processing to occur in linear space.
// The table contains 256 elements (8 bit input), though the outputs are
// only 7 bits (0 to 127).  This is normal and intentional by design: it
// allows all the rendering code to operate in the more familiar unsigned
// 8-bit colorspace (used in a lot of existing graphics code), and better
// preserves accuracy where repeated color blending operations occur.
// Only the final end product is converted to 7 bits, the native format
// for the LPD8806 LED driver.  Gamma correction and 7-bit decimation
// thus occur in a single operation.
unsigned char gammaTable[]  = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
    4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,
    7,  7,  7,  8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11,
   11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 16, 16,
   16, 17, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22,
   23, 23, 24, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30,
   30, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39,
   40, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48, 49, 50,
   50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 58, 58, 59, 60, 61, 62,
   62, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73, 74, 74, 75,
   76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
   92, 93, 94, 95, 96, 97, 98, 99,100,101,102,104,105,106,107,108,
  109,110,111,113,114,115,116,117,118,120,121,122,123,125,126,127
};

// These tables require 720 bytes of memory but greatly increase processing speed.
// This is only 30 LEDs worth of data and is recommended for many effects where 
//calculation speed is the bottleneck more so than pushing out pixels to the matrix/array.
float sineTable[90];
float cosTable[90];


// Buffers used for certain visual effects.
// Used to control 8 bit data such as intensities etc.
byte matrixLayer1[matrixSizeX][matrixSizeY];
byte matrixLayer2[matrixSizeX][matrixSizeY];
  
long modeController = 0;

// Helper functions
  
byte gamma(byte x);
byte highPassFilter(uint32_t c);
void pixelSet(int x, int y, uint32_t c);

// veryFastSine/Cos are approximately 50% faster but are less accurate than fastSine/Cos
float fastCos(float x);
float fastSine(float x);
float veryFastCos(float x);
float veryFastSine(float x);

uint32_t setBrightness(uint32_t c, int brightness);
uint32_t Wheel(uint16_t WheelPos);
uint32_t bgWheel(uint16_t WheelPos);
uint32_t inverseWheel(uint16_t WheelPos);

// Effects
void motionPlasma(int cycles);
void experimentalPlasma();
void fire();
void twoChannelFire();
void classicPlasma(int cycles);

void fastColorPulse();
void meteors();
void plasma();
void plasmaComplex();
void plasmaGoGo();
void povLine();
void pulsingGeometry(uint8_t wait);
void rain();
void randomPlasma();
void rainbowCycleUnity(uint8_t wait);
void rainbowCycleCentreSplit(uint8_t wait);
void rainbowRadial(uint8_t wait, int powerX, int powerY);
void richPlasma();
void noDivisionPlasma();
void sineWave();
void sparkler();
void sparklerMultiColor();
void stipesOfAwesome(uint8_t wait);
void sineWaveCheckers();
void ultimateTrap(uint8_t wait);

void setup() {
  
    randomSeed(analogRead(2));

	// Start up the LED strip
	strip.begin();

	// Update the strip, to start they are all 'off'
	strip.show();
  
  for (int i = 0; i < 90; i++)
  {
      sineTable[i] = sin(PI/180 * i);
      cosTable[i] = cos(PI/180 *i);
  }

  for(int x = 0; x < matrixSizeX; x++) 
    {
    for(int y = 0; y < matrixSizeY; y++)
      {
      matrixLayer1[x][y] = 0;
      matrixLayer2[x][y] = 0;
      }
    }

}



void loop() {
  
    modeController++;
  if(modeController>700)
    modeController = 0;
    
  //sparkler();
  //rainbow(0);
  //rainbowSineWave();
  //twoChannelFire();
  //off();
  //sineWave();
  //rainbowRadial(0, 4, 7);
  
  // Good to go
  //plasmaComplex();
  
//  if(modeController > 0 && modeController < 100)
//  {
//  plasmaComplex(); 
//  }
//
//  if(modeController > 100 && modeController <200)
//  {
//  sparkler();
//  }
//  
//  if(modeController > 200 && modeController < 300)
//  {
//  sineWaveCheckers();
//  }
//  
//  if(modeController > 300 && modeController < 400)
//  {
//  rainbowRadial(50, 4, 4);
//  }
//  
//  if(modeController > 400 && modeController < 700)
//  {
//  twoChannelFire();
//  }
  
  //noDivisionPlasma();
  //twoChannelFire();
  //fire();
  
  //sineWaveTriLayer();
  // Multi color sparkler
  //sparkler();
  
  //ultimateAcidTrap(10);
  
  //plasmaComplex();
  //richTwoAxisPlasma();
 // richPurplePlasma();
 // fastColorPulse();
  //plasmaComplexA();
  //rain();
  //classicPlasma(50);
  
  // Really good
  ///sineWaveCheckers();
  
  //rain();
  //sineWaveCheckers();
  //sineWave();
  //stipesOfAwesome(50);
  //rainbowCycleCentreSplit(50);
  //rainbowCycleUnity(50);
  //rainbowCycle(50);
  //experimentalPlasma();
  //mandelbrot();
  //povLine();
  //richPlasma();
  //meteors();
  //plasmaGoGo();
  //  classicPlasma(300);
  //sineWave();
//    motionPlasma(500);
	//colorWipe(strip.Color(127,0,0), 10);		// red
	//colorWipe(strip.Color(0,0,127), 10);		// blue

}

// Extremely Beautiful
void plasmaComplex() {
  
    double time = 0;
    
    for(int looper = 0; looper<50; looper++)
    {
        //time = millis()%100;
        time = looper;
        
        for(int x = 0; x < 6; x++)
        {
          for(int y = 0; y < 64; y++)
          {
//              double value = sin(dist(x + time, y, 64.0, 64.0) / 4.0)
//                         + sin(dist(x, y, 32.0, 32.0) / 4.0)
//                         + sin(dist(x, y + time / 7, 95.0, 32) / 3.5)
//                         + sin(dist(x, y, 95.0, 50.0) / 4.0);


//              double value = interpolateSine(dist(x + time, y, 64.0, 64.0) / 4.0)
//                         + interpolateSine(dist(x, y, 32.0, 32.0) / 4.0)
//                         + interpolateSine(dist(x, y + time / 7, 95.0, 32) / 3.5)
//                         + interpolateSine(dist(x, y, 95.0, 50.0) / 4.0);


              double value = veryFastSine(dist(x + time, y, 64.0, 64.0) / 4.0)
                         + veryFastSine(dist(x, y, 32.0, 32.0) / 4.0)
                         + veryFastSine(dist(x, y + time / 7, 95.0, 32) / 3.5)
                         + veryFastSine(dist(x, y, 95.0, 50.0) / 4.0);


            int color = int((4 + value)*384)%384;
//            pixelSet(x, y, Wheel(color));
            pixelSet(x, y, setBrightness(Wheel(color), 2));
//            pixelSet(x, y, blueChannel(Wheel(color)));
          }    
        }
      //strip.showCompileTime<ClockPin, DataPin>();   
      strip.show();
    }  
}

float interpolateSine(float x)
{
 return sineTable[int(x)] + (sineTable[int(x + .5)] - sineTable[int(x)]) / 2; 
}

float interpolateCos(float x)
{
 return cosTable[int(x)] + (cosTable[int(x + .5)] - cosTable[int(x)]) / 2; 
}

// Extremely Beautiful
void noDivisionPlasma() {
  
    double time = 0;
    
    for(int looper = 0; looper<700; looper++)
    {
        //time = millis()%100;
        time = looper;
        
        for(int x = 0; x < 6; x++)
        {
          for(int y = 0; y < 64; y++)
          {
              double value = sin(dist(x + time, y, 64.0, 64.0))
                         + sin(dist(x, y, 32.0, 32.0))
                         + sin(dist(x, y + time, 95.0, 32))
                         + sin(dist(x, y, 95.0, 50.0));
            int color = int((4 + value)*384)%384;
//            pixelSet(x, y, Wheel(color));
            pixelSet(x, y, setBrightness(Wheel(color), 2));
//            pixelSet(x, y, blueChannel(Wheel(color)));
          }    
        }
      //strip.showCompileTime<ClockPin, DataPin>();   
      strip.show();
    }  
}


void fire() {
  
  for(int x = 0; x < 6; x++)
    {
    matrixLayer1[x][0] = random(-20, 127);
    matrixLayer1[x][1] = random(-50, 127);
    }

  for(int y = 0; y < 64; y++)
    {
    for(int x = 0; x < 6; x++) 
      {
      byte newPoint = (matrixLayer1[x-1][y] + matrixLayer1[x+1][y] + matrixLayer1[x][y-1] + matrixLayer1[x][y+1]) / 4 - 10;
      matrixLayer2[x][y+1] = newPoint;
      }
    }
   
  strip.show();
  delay(40);
    
    for(int x = 0; x < 6; x++) 
    {
    for(int y = 0; y < 64; y++)
      {
      pixelSet(x,y, strip.Color(gamma(matrixLayer1[x][y]), 0, 0));
      matrixLayer1[x][y] = matrixLayer2[x][y];
      }
    }
}

void twoChannelFire() {
   
  // Getting a new random seed each time creates an improved fire effect. It is responsible for the tall columns of yellow flame.
  randomSeed(analogRead(2));

  for(int x = 0; x < 6; x++)
    {
    int randomOff = (0, 2);
    if(randomOff == 0)
      {
      matrixLayer1[x][0] = 0;
      matrixLayer1[x][1] = 0;      
      } else {
      matrixLayer1[x][0] = random(-100, 255);
      matrixLayer1[x][1] = random(-60, 255);
      }
    }

  for(int y = 0; y < 64; y++)
    {
    for(int x = 0; x < 6; x++) 
      {
      byte newPoint;
      int flameDampen = random(0, 5);
      int flameValue = (matrixLayer1[x-1][y] + matrixLayer1[x+1][y] + matrixLayer1[x][y-1] + matrixLayer1[x][y+1]) / flameDampen;
      if(flameDampen < flameValue)
        {
        newPoint = byte(flameValue - flameDampen/flameValue);
        } else {
        newPoint = byte(0);  
        }
      matrixLayer2[x][y+1] = newPoint;
      }
    }
   
  strip.show();
  delay(50);
    
    for(int x = 0; x < 6; x++) 
    {
    for(int y = 0; y < 64; y++)
      {
      pixelSet(x,y, strip.Color(gamma(matrixLayer1[x][y]), gamma(byte(matrixLayer1[x][y]/(y+2))), 0));
      matrixLayer1[x][y] = matrixLayer2[x][y];
      }
    }
}


// Oh yeah! Its very good
void sparkler() {
  
 for (int r=0; r < 50; r++) {	

  for(int x = 0; x < 6; x++)
    {
    matrixLayer1[x][0] = random(0, 127);
    matrixLayer1[x][1] = random(0, 127);
    }
  
  for(int x = 0; x < 6; x++) 
    {
    for(int y = 0; y < 64; y++)
      {
      byte newPoint = (matrixLayer1[x-1][y] + matrixLayer1[x+1][y] + matrixLayer1[x][y-1] + matrixLayer1[x][y+1]) / 4 - 15;
      matrixLayer2[x][y+1] = newPoint;
      if(newPoint>80)
        pixelSet(x,y, setBrightness(Wheel(((newPoint/5)+r)%384),1));      
      if(newPoint<80)
        pixelSet(x,y, strip.Color(0,0,0)); 
      }
    }
   
   strip.show();
  delay(90);
    
    for(int x = 0; x < 6; x++) 
    {
    for(int y = 0; y < 64; y++)
      {
      matrixLayer1[x][y] = matrixLayer2[x][y];
      }
    }
    
 }
    
}

void sparklerMultiColor() {
  
 for (int r=0; r < 900; r++) {	

  for(int x = 0; x < 6; x++)
    {
    matrixLayer1[x][0] = random(0, 127);
    matrixLayer1[x][1] = random(0, 127);
    }
  
  for(int x = 0; x < 6; x++) 
    {
    for(int y = 0; y < 64; y++)
      {
      byte newPoint = (matrixLayer1[x-1][y] + matrixLayer1[x+1][y] + matrixLayer1[x][y-1] + matrixLayer1[x][y+1]) / 4 - 15;
      matrixLayer2[x][y+1] = newPoint;
      pixelSet(x,y, strip.Color(gamma(newPoint)/r, gamma(newPoint)/(r/300), gamma(newPoint)*(r/900)));
      }
    }
   
  strip.show();
  delay(40);
    
    for(int x = 0; x < 6; x++) 
    {
    for(int y = 0; y < 64; y++)
      {
      matrixLayer1[x][y] = matrixLayer2[x][y];
      }
    }
    
 }
    
}

// sucks
void randomPlasma() {
    
   double time = random(0, 34);
          
    for(int looper = 0; looper<500; looper++)
    {
        //time = millis()%100;
        //time = looper;
        time = time+looper;

        for(int x = 0; x < 32; x++)
        {
          for(int y = 0; y < 10; y++)
          {
              double value = sin(dist(x + time, y, 64.0, 64.0) / 4.0)
                         + sin(dist(x-time, y, 32.0, 32.0) / 4.0)
                         + sin(dist(x, y + time / 7, 95.0, 32) / 3.5)
                         + sin(dist(x, y-time, 95.0, 50.0) / 4.0);
            int color = int((4 + value)*384)%384;
//            pixelSet(x, y, Wheel(color));
            pixelSet(x, y, setBrightness(Wheel(color), 1));
          }    
        }
      //strip.showCompileTime<ClockPin, DataPin>();   
      strip.show();
      delay(10);
    }  
}

void plasmaComplexA() {
  
    double time = random(0, 34);
    
    for(int looper = 0; looper<500; looper++)
    {
        for(int x = 0; x < 32; x++)
        {
          for(int y = 0; y < 10; y++)
          {
            double value = ((128.0 * sin(x+looper / 2.0))+(128.0 * sin(y+looper / 4.0)) + (128.0 * sin(sqrt(double((x - 32 / 2.0)* 
            (x - 32 / 2.0) + (y - 5 / 2.0) * (y - 5 / 2.0))) / 8.0))+ (128.0 * sin(sqrt(double(x * x + y * y)) / 4.0))) / 4;
              int color = int(value);
              pixelSet(x, y, Wheel(color));
          }    
        }
      //strip.showCompileTime<ClockPin, DataPin>();   
      strip.show();
    }  
}
void sineWave() {
  int i, j;
  int center = 32;
	 
  for (int r=0; r < 400; r++) {	
    for (j=0; j < 105; j++) 
    {
      for (i=0; i < 11; i++) 
      {
        // add i so it progresses
        // sine function
        float sineValue = sin(0.2*(r+j));
        float calculatedValue = ((2*(float)i)/(31))-1;
        
        // Change of range function (-1 - +1) -> (0 - 31)		        
        float shiftSine = ((31*(sineValue+1))/(2))+0;

        if(calculatedValue<sineValue+1)
        {
          if(calculatedValue<sineValue-1)
          { 
           pixelSet(j, i, bgWheel(50));
          } else {
          // Rolling portion
//          pixelSet(j, i, bgWheel((int(abs(calculatedValue-sineValue)))*384));
          pixelSet(j, i+r, gWheel(int(shiftSine*i)%384));
          }  
      } else {
          pixelSet(j, i, bgWheel(50));
        }
      }
    }
    strip.show();		// write all the pixels out
    delay(10);
  }  
  
}

void sineWaveCheckers() {
  int i, j;
  int center = 32;
	 
  for (int r=0; r < 300; r++) {	
    for (i=0; i < 64; i++) 
    {
      for (j=0; j < 6; j++) 
      {
        // add i so it progresses
        // sine function
        float sineValue = sin(0.2*(r+j));
        float calculatedValue = ((2*(float)i)/(64))-1;
        
        // Change of range function (-1 - +1) -> (0 - 31)		        
        float shiftSine = ((31*(sineValue+1))/(2))+0;

        if(calculatedValue<sineValue+1)
        {
          if(calculatedValue<sineValue-0)
          { 
           pixelSet(j, i, strip.Color(0,0,0));
          } else {
          // Rolling portion
          
          pixelSet(j, i, setBrightness(Wheel((((i+r)*6))%384), 1));

          
//          pixelSet(j, i, bgWheel((int(abs(calculatedValue-sineValue)))*384));
//          if(i<32)
//            pixelSet(j, i, setBrightness(Wheel(int(shiftSine*(j*5))%384+(i*5))));
//          if(i>32)
//            pixelSet(j, i, setBrightness(Wheel(int(shiftSine*(j*5))%384-(i*5))));
          }  
      } else {
//          pixelSet(j, i, bgWheel(20+j));
           pixelSet(j, i, strip.Color(0,0,0));
        }
      }
    }
    strip.show();		// write all the pixels out
    delay(30);
  }  
  
}

void sineWaveTriLayer() {
  int i, j;
  int center = 32;
	 
  for (int r=0; r < 900; r++) {	
    for (i=0; i < 64; i++) 
    {
      for (j=0; j < 6; j++) 
      {
        // add i so it progresses
        
        float sineValue = sin(0.2*(r+j));		
        float calculatedValue = ((2*(float)j)/(32))-1;
        
//        Serial.println("___________________");
//        Serial.println(sineValue);
//        Serial.println(j);
//        Serial.println(calculatedValue);
//        Serial.println("___________________");

        if(calculatedValue<sineValue+0.3)
        {
          if(calculatedValue<sineValue-0.3)
          { 
           pixelSet(j, i, Wheel((calculatedValue-sineValue)*384));
          } else {
          pixelSet(j, i, bgWheel((calculatedValue-sineValue)*384));
          }  
      } else {
          pixelSet(j, i, Wheel((calculatedValue-sineValue)*384));
        }
      }
    }
    strip.show();		// write all the pixels out
    delay(50);
  }  
  
}


void mandelbrot() {
  
double ImageHeight = 64;
double ImageWidth = 6;
double MinRe = -2.0;
double MaxRe = 1.0;
double MinIm = -1.2;
double MaxIm = MinIm+(MaxRe-MinRe)*ImageHeight/ImageWidth;
double Re_factor = (MaxRe-MinRe)/(ImageWidth-1);
double Im_factor = (MaxIm-MinIm)/(ImageHeight-1);
unsigned MaxIterations = 30;

for(int looper = 0; looper<200; looper++)
{
  for(unsigned y=0; y<ImageHeight; ++y)
  {
      double c_im = MaxIm - y*Im_factor - looper;
      for(unsigned x=0; x<ImageWidth; ++x)
      {
          double c_re = MinRe + x*Re_factor;
  
          double Z_re = c_re, Z_im = c_im;
          bool isInside = true;
          for(unsigned n=0; n<MaxIterations; ++n)
          {
              double Z_re2 = Z_re*Z_re, Z_im2 = Z_im*Z_im;
              if(Z_re2 + Z_im2 > 4)
              {
                  isInside = false;
                  break;
              }
              Z_im = 2*Z_re*Z_im + c_im;
              Z_re = Z_re2 - Z_im2 + c_re;
              
          }
          
          pixelSet(x, y, Wheel(Z_im));
//          strip.show();
  //        if(isInside) 
  //        { 
  //        pixelSet(x, y, strip.Color(0,0,127));
  //        } else {
  //        pixelSet(x, y, strip.Color(0,127,0));          
  //        }
      }
  }
            strip.show();
}  

}

  
void classicPlasma(int cycles) {

    double time;
    for(int looper = 0; looper<cycles; looper++)
    {
      // Does nto work "now" not declared
      //        time = now() / 50.0;
        //time = millis();
        time = looper;
        
        for(int x = 0; x <= 9; x++)
        {
          for(int y = 0; y <= 32; y++)
          {
              double value = double
              (
                    128.0 + (128.0 * sin(x+time / 16.0))
                  + 128.0 + (128.0 * sin(y / 32.0))
                  + 128.0 + (128.0 * sin(sqrt(double((x - 10 / 2.0)* (x - 10 / 2.0) + (y - 32 / 2.0) * (y - 32 / 2.0))) / 8.0))
                  + 128.0 + (128.0 * sin(sqrt(double(x * x + y * y)) / 8.0))
              ) / 4;
              
              pixelSet(y, x, Wheel(value));
          }    
        }
      //strip.showCompileTime<ClockPin, DataPin>();   
      strip.show();
    }  
}

void meteors() {
	int i, j, n;
        int center = 32;
        uint16_t meteorColor = Wheel(120);
        uint16_t blank = strip.Color(0,0,0);
        
	 
        for (int r=0; r < 10; r++) {
          // Should be increased to 9 for the dress
          int randomColumn = random(0, 52);
          for(n=0; n<52; n++)
          {
	  for (j=0; j < 11; j++) {
            pixelSet(j-1, n, blank);
            pixelSet(j, n, Wheel(110));
            pixelSet(j+1, n, Wheel(150));
            pixelSet(j+2, n, Wheel(190));
            strip.show();		// write all the pixels out
            //delay(10);
	  }
          }
          
          

        }
}

void povLine() {
	int i, j;
        int center = 32;
        uint32_t blank = strip.Color(0,0,127);
        uint32_t red = strip.Color(127,0,0);
        
	 
        for (int r=0; r < 33; r++) 
        {
	  for (j=0; j < 10; j++) 
          {
            for(int x = 0; x <= 32; x++)
            {
              if((j+r)%2==0)
              {
                pixelSet(x, j, red);
              } else {
                pixelSet(x, j, blank);
              }
            }
            strip.show();
            //delay(200);
	  }

        }
        
        delay(5000);
        
}

void richPlasma() {
  
  // This slows down over time as millis() returns really large numbers
//  double time = millis();

  double time = 0;

  // 30 frames of animation
  for (int r=0; r < 300; r++) 
    {

     for(int y = 0; y < 10; y++)
    {
      for(int x = 0; x < 32; x++)
      {
        uint16_t richColor =
        (
              128.0 + (128.0 * sin(x+time / 2.0))
            + 128.0 + (128.0 * sin(y / 1.0))
            + 128.0 + (128.0 * sin((x + y + time) / 2.0))
            + 128.0 + (128.0 * sin(sqrt(double(x * x + y * y + time)) / 1.0))
        ) / 3;
        pixelSet(x, y, Wheel(richColor));
      }
    }
    
    strip.show();		// write all the pixels out
    time = time+10;
    delay(50);
  }
}



void experimentalPlasma() {
  
  // This slows down over time as millis() returns really large numbers
//  double time = millis();

  double time = 0;

  for (int r=0; r < 900; r++) 
    {

     for(int x = 0; x < 32; x++)
    {
      for(int y = 0; y < 10; y++)
      {
        uint16_t richColor =
        (
              128.0 + (128.0 * sin(x+time / 2.0))
            + 128.0 + (128.0 * sin(y+time / 1.0))
            + 128.0 + (128.0 * sin((x + y + time) / 2.0))
            + 128.0 + (128.0 * sin(sqrt(double(x * x + y * y + time)) / 1.0))
        ) / 3;
        pixelSet(x, y, Wheel(richColor));
      }
    }
    
    strip.show();		// write all the pixels out
    time = time+10;
    delay(100);
  }
}

// This thing is totally wild. Changes wildly with time.
void richTwoAxisPlasma() {
  
  double time = millis();

  for (int r=0; r < 900; r++) 
    {
    time = millis();

     for(int x = 0; x < 32; x++)
    {
      for(int y = 0; y < 10; y++)
      {
        uint16_t richColor =
        (
              128.0 + (128.0 * sin(x+time / 2.0))
            + 128.0 + (128.0 * sin(y+time / 1.0))
            + 128.0 + (128.0 * sin((x + y + time) / 2.0))
            + 128.0 + (128.0 * sin(sqrt(double(x * x + y * y + time)) / 1.0))
        )/2;
        pixelSet(x, y, Wheel(richColor));
      }
    }
    
    strip.show();		// write all the pixels out
    delay(10);
  }
}



void richPurplePlasma() {
  
  double time = millis();

  for (int r=0; r < 900; r++) 
    {
    time = r*2;

     for(int x = 0; x <= 6; x++)
    {
      for(int y = 0; y <= 65; y++)
      {
        uint16_t richColor =
        (
              128.0 + (128.0 * sin(x+time / 2.0))
            + 128.0 + (128.0 * sin(y / 1.0))
            + 128.0 + (128.0 * sin((x + y + time) / 2.0))
            + 128.0 + (128.0 * sin(sqrt(double(x * x + y * y + time)) / 1.0))
        ) / 9;
        pixelSet(x, y, Wheel(richColor));
      }
    }
    
    strip.show();		// write all the pixels out
  }
}

void rain() {
	int i, j;
        int center = 32;
	 
        for (int r=0; r < 30; r++) {			// 3 cycles of all 384 colors in the wheel
	  for (j=32; j > 0; j--) {			// 3 cycles of all 384 colors in the wheel
            uint32_t color = (128.0 + (128.0 * sin(j+r / 2.0))) / 4;
            pixelSet(j+1, 0, bgWheel(color));
            pixelSet(j, 1, bgWheel(color));
            pixelSet(j+3, 2, bgWheel(color));
            pixelSet(j+4, 3, bgWheel(color));
            pixelSet(j+1, 4, bgWheel(color));
            pixelSet(j+1, 5, bgWheel(color));
            pixelSet(j, 6, bgWheel(color));
            pixelSet(j+3, 7, bgWheel(color));
            pixelSet(j+4, 8, bgWheel(color));
            pixelSet(j+1, 9, bgWheel(color));
	  }
          strip.show();		// write all the pixels out
          //delay(40);
        }
}

void fastColorPulse() {
	int i, j;
        int center = 32;
	 
        for (int r=0; r < 384; r++) {			// 3 cycles of all 384 colors in the wheel
	  for (j=0; j < 64; j++) {			// 3 cycles of all 384 colors in the wheel
            pixelSet(0, j, Wheel((j+r)%384));
            pixelSet(1, j, Wheel((j+r)%384));
            pixelSet(2, j, Wheel((j+r)%384));
            pixelSet(3, j, Wheel((j+r)%384));
            pixelSet(4, j, Wheel((j+r)%384));
            pixelSet(5, j, Wheel((j+r)%384));
                  
	  }
          strip.show();		// write all the pixels out
        }
}

void rainbow(uint8_t wait) {
	int i, j;
	 
	for (j=0; j < 384; j++) {			// 3 cycles of all 384 colors in the wheel
		for (i=0; i < strip.numPixels(); i++) {
			strip.setPixelColor(i, Wheel( (i + j) % 384));
		}	 
		strip.show();		// write all the pixels out
		delay(wait);
	}
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t wait) {
	uint16_t i, j;
	
	for (j=0; j < 384 * 5; j++) {			// 5 cycles of all 384 colors in the wheel
		for (i=0; i < strip.numPixels(); i++) {
			// tricky math! we use each pixel as a fraction of the full 384-color wheel
			// (thats the i / strip.numPixels() part)
			// Then add in j which makes the colors go around per pixel
			// the % 384 is to make the wheel cycle around
			strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
		}	 
		strip.show();		// write all the pixels out
		delay(wait);
	}
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
	int i;
	
	for (i=0; i < strip.numPixels(); i++) {
		strip.setPixelColor(i, c);
	}
		strip.show();

}

// Chase a dot down the strip
// good for testing purposes
void colorChase(uint32_t c, uint8_t wait) {
	int i;
	
	for (i=0; i < strip.numPixels(); i++) {
		strip.setPixelColor(i, 0);	// turn all pixels off
	} 
	
	for (i=0; i < strip.numPixels(); i++) {
		strip.setPixelColor(i, c);
		if (i == 0) { 
			strip.setPixelColor(strip.numPixels()-1, 0);
		} else {
			strip.setPixelColor(i-1, 0);
		}
		strip.show();
		delay(wait);
	}
}





void plasma() {

    for(int y = 0; y <= 9; y++)
    {
      for(int x = 0; x <= 52; x++)
      {
        uint32_t color =
        (
              128.0 + (128.0 * sin(x / 2.0))
            + 128.0 + (128.0 * sin(y / 1.0))
            + 128.0 + (128.0 * sin((x + y) / 2.0))
            + 128.0 + (128.0 * sin(sqrt(double(x * x + y * y)) / 1.0))
        ) ;
        pixelSet(x, y, Wheel(color));
      }
    }
    
    //strip.showCompileTime<ClockPin, DataPin>();   
    strip.show();

  
}

void plasmaGoGo() {

    double time = 0;
    
    for(int loopIt = 0; loopIt < 400; loopIt++)
    {
      for(int y = 0; y < 10; y++)
      {
        for(int x = 0; x < 32; x++)
        {
              double value = sin(dist(x + time, y, 128.0, 128.0) / 2.0)
                           + sin(dist(x, y, 64.0, 64.0) / 2.0)
                           + sin(dist(x, y + time / 7, 192.0, 64) / 2.0)
                           + sin(dist(x, y, 192.0, 100.0) / 2.0);
              int color = int((4 + value)) * 32;
  
          pixelSet(x, y, Wheel(color));
        }
      }
      
      //strip.showCompileTime<ClockPin, DataPin>();   
      time++;
      strip.show();
    }
  
}




void motionPlasma(int cycles) {

    double time;
    for(int looper = 0; looper<cycles; looper++)
    {
      // Does nto work "now" not declared
      //        time = now() / 50.0;
        time = millis();
        for(int x = 0; x <= 9; x++)
        {
          for(int y = 0; y <= 52; y++)
          {
              double value = sin(dist(x + time, y, 128.0, 128.0) / 2.0)
                           + sin(dist(x+time, y, 64.0, 64.0) / 2.0);
                           //+ sin(dist(x, y + time / 7, 192.0, 64) / 1.75)
                           //+ sin(dist(x, y, 192.0, 100.0) / 2.0);
              int color = int((4 + value)) * 32;
              //pset(x, y, ColorRGB(color, color * 2, 255 - color));
              //pixelSet(x, y, bgWheel(color));
          
            int i =0;
  
            if(x%2==0)
            {
              i = (x*52)+y;
            } else {
              i = (x*52)+51-y;    
            }
            strip.setPixelColor(i, bgWheel(color)); 
          }    
        }
      //strip.showCompileTime<ClockPin, DataPin>();   
      strip.show();
    }  
}


void rainbowCycleUnity(uint8_t wait) {
  uint16_t i, j,p;
  // The higher this number the more rainbows there are on the array
  // CompressionFactor of 10 yields pretty much exactly 1 rainbow.
  // Greater than 60 breaks the linearity of the rainbow.
  int compressionFactor = 5;

  for (j=384*5; j > 0; j--) {     // 5 cycles of all 384 colors in the wheel
    for(p=0; p<6;p++)
    {
    for (i=0; i <= 64; i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      //strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
      //strip.setPixelColor(i+(i*103)+p, Wheel((i+j+p) % 384));
      pixelSet(p, i, setBrightness(Wheel(((i*compressionFactor)+j)%384),2));

    }
    }
    //strip.showCompileTime<ClockPin, DataPin>();    
    strip.show();   // write all the pixels out
    //delay(50);
    //delay(wait);

  }
}    

void rainbowCycleCentreSplit(uint8_t wait) {
  uint16_t i, j,p;
  // The higher this number the more rainbows there are on the array
  // CompressionFactor of 10 yields pretty much exactly 1 rainbow.
  // Greater than 60 breaks the linearity of the rainbow.
  int compressionFactor = 10;

  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for(p=0; p<32;p++)
    {
    for (i=0; i < 10; i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      //strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
      //strip.setPixelColor(i+(i*103)+p, Wheel((i+j+p) % 384));
      if(p<16) 
        {
        pixelSet(p, i, Wheel(((p*compressionFactor)+j)%384));  
        }
      if(p>15) 
        {
        pixelSet(p, i, Wheel((((32-p)*compressionFactor)+j)%384)); 
      }
  }
    }
    //strip.showCompileTime<ClockPin, DataPin>();    
    strip.show();   // write all the pixels out
    //delay(50);
    //delay(wait);

  }
}    

void rainbowRadial(uint8_t wait, int powerX, int powerY) {
  // Power creates different patterns (2-9 is good)

  uint16_t i, j,p;
  // The higher this number the more rainbows there are on the array
  // CompressionFactor of 10 yields pretty much exactly 1 rainbow.
  // Greater than 60 breaks the linearity of the rainbow.
  int compressionFactor = 50;

  for (j=50; j > 0; j--) {     // 5 cycles of all 384 colors in the wheel
    for(p=0; p < 6; p++)
    {
    for (i=0; i < 64; i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      //strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
      //strip.setPixelColor(i+(i*103)+p, Wheel((i+j+p) % 384));
      int wheelSetting = sqrt((abs((15-p))^powerX)+(abs((4-i))^powerY));
       pixelSet(p, i, setBrightness(Wheel(((wheelSetting*compressionFactor)+j)%384), 3));  

  }
    }
    //strip.showCompileTime<ClockPin, DataPin>();    
    strip.show();   // write all the pixels out
    //delay(50);
    //delay(wait);

  }
}

// Very strong shift pulses.
void ultimateTrap(uint8_t wait) {
  uint16_t i, j,p;
  // The higher this number the more rainbows there are on the array
  // CompressionFactor of 10 yields pretty much exactly 1 rainbow.
  // Greater than 60 breaks the linearity of the rainbow.
  int compressionFactor = 50;

  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for(p=0; p < 6; p++)
    {
    for (i=0; i < 64; i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      //strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
      //strip.setPixelColor(i+(i*103)+p, Wheel((i+j+p) % 384));
      int wheelSetting = sqrt(((6-p)^(j/5))+((64-i)^(j/5)));
       pixelSet(p, i, Wheel(((wheelSetting*compressionFactor)+j+(i*5))%384));  

  }
    }
    //strip.showCompileTime<ClockPin, DataPin>();    
    strip.show();   // write all the pixels out
    //delay(50);
    delay(wait);

  }
}    


// Freaking amazing mode
void pulsingGeometry(uint8_t wait) {
  uint16_t i, j,p;
  // The higher this number the more rainbows there are on the array
  // CompressionFactor of 10 yields pretty much exactly 1 rainbow.
  // Greater than 60 breaks the linearity of the rainbow.
  int compressionFactor = 50;

  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for(p=0; p < 32; p++)
    {
    for (i=0; i < 10; i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      //strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
      //strip.setPixelColor(i+(i*103)+p, Wheel((i+j+p) % 384));
      int wheelSetting = sqrt(((15-p)^2)+((6-i)^2));
       pixelSet(p, i, Wheel(((wheelSetting*compressionFactor)+j)%384));  

  }
    }
    //strip.showCompileTime<ClockPin, DataPin>();    
    strip.show();   // write all the pixels out
    //delay(50);
    //delay(wait);

  }
}    

void stipesOfAwesome(uint8_t wait) {
  uint16_t i, j,p;

  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for(p=0; p<10;p++)
    {
    for (i=0; i <= 53; i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      //strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
      //strip.setPixelColor(i+(i*103)+p, Wheel((i+j+p) % 384));
      pixelSet(p, i, Wheel((p*j)%384));

    }
    }
    //strip.showCompileTime<ClockPin, DataPin>();    
    strip.show();   // write all the pixels out
    delay(50);
    //delay(wait);

  }
}    


void crazyBounce(uint8_t wait) {
  uint16_t i, j,p;

  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for(p=0; p<32;p++)
    {
    for (i=0; i <= 5; i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      //strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
      //strip.setPixelColor(i+(i*103)+p, Wheel((i+j+p) % 384));
      pixelSet(p, i, Wheel((p*j)%384));

    }
    }
    //strip.showCompileTime<ClockPin, DataPin>();    
    strip.show();   // write all the pixels out
    //delay(wait);

  }
} 




/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
	byte r, g, b;
	switch(WheelPos / 128)
	{
		case 0:
			r = 127 - WheelPos % 128;		//Red down
			g = WheelPos % 128;			 // Green up
			b = 0;									//blue off
			break; 
		case 1:
			g = 127 - WheelPos % 128;	 //green down
			b = WheelPos % 128;			 //blue up
			r = 0;									//red off
			break; 
		case 2:
			b = 127 - WheelPos % 128;	 //blue down 
			r = WheelPos % 128;			 //red up
			g = 0;									//green off
			break; 
	}
	return(strip.Color(r,g,b));
}

uint32_t inverseWheel(uint16_t WheelPos)
{
	byte r, g, b;
	switch(WheelPos / 128)
	{
		case 0:
			b = 127 - WheelPos % 128;	 //blue down 
			r = WheelPos % 128;			 //red up
			g = 0;									//green off
			break; 
		case 1:
			g = 127 - WheelPos % 128;	 //green down
			b = WheelPos % 128;			 //blue up
			r = 0;									//red off
			break; 
		case 2:

			r = 127 - WheelPos % 128;		//Red down
			g = WheelPos % 128;			 // Green up
			b = 0;									//blue off
			break; 
	}
	return(strip.Color(r,g,b));
}

uint32_t bgWheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 0; // red down
      g = WheelPos % 128;       // green up
      b = 0;                    // blue off
      break;
    case 1:
      g = 127 - WheelPos % 128; // green down
      b = WheelPos % 128;       // blue up
      r = 0;                    // red off
      break;
    case 2:
      b = 127 - WheelPos % 128; // blue down
      r = 0;       // red up
      g = WheelPos % 128;                    // green off
      break;
  }
  return(strip.Color(r,g,b));
}


uint32_t gWheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 0; // red down
      b = WheelPos % 128;       // green up
      g = 0;                    // blue off
      break;
    case 1:
      b = WheelPos % 128; // green down
      g = 0;//127 - WheelPos % 128;       // blue up
      r = 0;                    // red off
      break;
    case 2:
      g = 0;//127 - WheelPos % 128; // blue down
      r = 0;       // red up
      b = WheelPos % 128;                    // green off
      break;
  }
  return(strip.Color(r,g,b));
}


void pixelSet(int x, int y, uint32_t c) {
 
  // Converts an x,y pair into a linear position
  // Expect coordinate system with origin in bottom left at (0,0)
  int i =0;
  
  if(y%2==0)
  {
    i = (y*matrixSizeX)+x;
  } else {
    i = (y*matrixSizeX)+(matrixSizeX-1)-x;    
  }
  
  strip.setPixelColor(i, c); 
  
}



// This function (which actually gets 'inlined' anywhere it's called)
// exists so that gammaTable can reside out of the way down here in the
// utility code...didn't want that huge table distracting or intimidating
// folks before even getting into the real substance of the program, and
// the compiler permits forward references to functions but not data.
byte gamma(byte x) {
  return gammaTable[x];
}

// Brightness 0-10. 1 being maximum.
uint32_t setBrightness(uint32_t c, int brightness) {

 byte  r, g, b;
  
  // Need to decompose color into its r, g, b elements
  g = ((c >> 16) & 0x7f)/brightness;
  r = ((c >>  8) & 0x7f)/brightness;
  b = (c        & 0x7f)/brightness; 
  
  
  
  //return(strip.Color(gammaTable[r], gammaTable[g], gammaTable[b]));
  return(strip.Color(r,g,b));

}

byte highPassFilter(uint32_t c) {

 byte  r, g, b;
  
  // Need to decompose color into its r, g, b elements
  g = ((c >> 16) & 0x7f);
  r = ((c >>  8) & 0x7f);
  b = (c        & 0x7f); 
  
  
  
  //return(strip.Color(gammaTable[r], gammaTable[g], gammaTable[b]));
  return(strip.Color(r,g,b));

}

uint32_t redChannel(uint32_t c) {

 byte  r, g, b;
  
  // Need to decompose color into its r, g, b elements
  g = ((c >> 16) & 0x7f);
  r = ((c >>  8) & 0x7f);
  b = (c        & 0x7f); 
  
  
  
  //return(strip.Color(gammaTable[r], gammaTable[g], gammaTable[b]));
  return(strip.Color(r,0,0));

}


uint32_t greenChannel(uint32_t c) {

 byte  r, g, b;
  
  // Need to decompose color into its r, g, b elements
  g = ((c >> 16) & 0x7f);
  r = ((c >>  8) & 0x7f);
  b = (c        & 0x7f); 
  
  
  
  //return(strip.Color(gammaTable[r], gammaTable[g], gammaTable[b]));
  return(strip.Color(0,g,0));

}

uint32_t blueChannel(uint32_t c) {

 byte  r, g, b;
  
  // Need to decompose color into its r, g, b elements
  g = ((c >> 16) & 0x7f);
  r = ((c >>  8) & 0x7f);
  b = (c        & 0x7f); 
  
  
  
  //return(strip.Color(gammaTable[r], gammaTable[g], gammaTable[b]));
  return(strip.Color(0,0,b));

}


// Trigonometric heuristic for speeding up Sin and Cos heavy methods. 
// Adapter from Michael Baczynski
// http://lab.polygonal.de
/*********************************************************
 * low precision sine/cosine
 *********************************************************/

float veryFastSine(float x)
{

  float returnSin = 0;
  float returnCos = 0;
  //always wrap input angle to -PI..PI
if (x < -3.14159265)
    x += 6.28318531;
else
if (x >  3.14159265)
    x -= 6.28318531;

//compute sine
if (x < 0)
    returnSin = 1.27323954 * x + .405284735 * x * x;
else
    returnSin = 1.27323954 * x - 0.405284735 * x * x;
    
    return returnSin;
}

float veryFastCos(float x)
{

  float returnSin = 0;
  float returnCos = 0;
  //always wrap input angle to -PI..PI
if (x < -3.14159265)
    x += 6.28318531;
else
if (x >  3.14159265)
    x -= 6.28318531;

//compute sine
if (x < 0)
    returnSin = 1.27323954 * x + .405284735 * x * x;
else
    returnSin = 1.27323954 * x - 0.405284735 * x * x;

//compute cosine: sin(x + PI/2) = cos(x)
x += 1.57079632;
if (x >  3.14159265)
    x -= 6.28318531;

if (x < 0)
    returnCos = 1.27323954 * x + 0.405284735 * x * x;
else
    returnCos = 1.27323954 * x - 0.405284735 * x * x;
  
  return returnCos;
}


/*********************************************************
 * high precision sine/cosine
 *********************************************************/

float fastSine(float x)
{
  float returnSin = 0;
  float returnCos = 0;

//always wrap input angle to -PI..PI
if (x < -3.14159265)
    x += 6.28318531;
else
if (x >  3.14159265)
    x -= 6.28318531;

//compute sine
if (x < 0)
{
    returnSin = 1.27323954 * x + .405284735 * x * x;

    if (returnSin < 0)
        returnSin = .225 * (returnSin *-returnSin - returnSin) + returnSin;
    else
        returnSin = .225 * (returnSin * returnSin - returnSin) + returnSin;
}
else
{
    returnSin = 1.27323954 * x - 0.405284735 * x * x;

    if (returnSin < 0)
        returnSin = .225 * (returnSin *-returnSin - returnSin) + returnSin;
    else
        returnSin = .225 * (returnSin * returnSin - returnSin) + returnSin;
}

return returnSin;

}

float fastCos(float x)
{
  float returnSin = 0;
  float returnCos = 0;

//always wrap input angle to -PI..PI
if (x < -3.14159265)
    x += 6.28318531;
else
if (x >  3.14159265)
    x -= 6.28318531;

//compute sine
if (x < 0)
{
    returnSin = 1.27323954 * x + .405284735 * x * x;

    if (returnSin < 0)
        returnSin = .225 * (returnSin *-returnSin - returnSin) + returnSin;
    else
        returnSin = .225 * (returnSin * returnSin - returnSin) + returnSin;
}
else
{
    returnSin = 1.27323954 * x - 0.405284735 * x * x;

    if (returnSin < 0)
        returnSin = .225 * (returnSin *-returnSin - returnSin) + returnSin;
    else
        returnSin = .225 * (returnSin * returnSin - returnSin) + returnSin;
}

//compute cosine: sin(x + PI/2) = cos(x)
x += 1.57079632;
if (x >  3.14159265)
    x -= 6.28318531;

if (x < 0)
{
    returnCos = 1.27323954 * x + 0.405284735 * x * x;

    if (returnCos < 0)
        returnCos = .225 * (returnCos *-returnCos - returnCos) + returnCos;
    else
        returnCos = .225 * (returnCos * returnCos - returnCos) + returnCos;
}
else
{
    returnCos = 1.27323954 * x - 0.405284735 * x * x;

    if (returnCos < 0)
        returnCos = .225 * (returnCos *-returnCos - returnCos) + returnCos;
    else
        returnCos = .225 * (returnCos * returnCos - returnCos) + returnCos;
}

  return returnCos;
}
