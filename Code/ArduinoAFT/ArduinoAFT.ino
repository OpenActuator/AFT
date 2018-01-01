#include <Stepper.h>     // Step Moter
#include "hx711.h"        // LoadCell A/D Convertor -> Force Measurement
#include "Wire.h"         // PCF8591 D/A Convertor  -> Current Output

#define PCF8591 (0x90 >> 1) 

#define   CURRENT_PIN       A0
#define   STROKE_PIN        A2
#define   FORCE_PIN         A4

#define   FORCE_SCK_PIN     A2
#define   FORCE_DOUT_PIN    A3

#define   VF_SDA_PIN        A4
#define   VF_SCL_PIN        A5

// Only Arduino Uno, Fio
#if defined(__AVR_ATmega328P__)                     
  Hx711 hx711Scale(FORCE_DOUT_PIN, FORCE_SCK_PIN);
#endif

const int MOTOR_STEPS = 20;

const int C_MODE = 11;
const int N_MODE = 12;
int g_iPositionCount;

const int NUMBER_SIZE = 5;

int g_iDataMode;
int g_nNumberDataCount;
int g_iNumberData;
byte g_btCommand;
int g_iSign = 1;

bool g_bUno;
bool g_bStepMotor;
bool g_bVoltageFollower;
bool g_bLoadcell;

Stepper stepper(MOTOR_STEPS, 8, 9, 10, 11);

void setup()
{  
  Wire.begin();
  // Have to make 0 volt immediately because the first voltage is 5V after connect aduino board with PC.
  outputVoltage(0);
  
  //Initialize serial and wait for port to open
  Serial.begin(9600);

  while (!Serial){
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  stepper.setSpeed(50);  
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  
  g_iPositionCount = 0;

  g_iDataMode = C_MODE;
  g_nNumberDataCount = 0;
  g_iNumberData = 0;  

  // basic condition is Arduino Due, using a stroke sensor, using a power supply.
  g_bUno = false;
  g_bStepMotor = false;
  g_bVoltageFollower = false;
  g_bLoadcell = false;

  #if defined(__AVR_ATSAM3X8E__)      // Arduino Due
    analogReadResolution(12);
  #elif defined(__AVR_ATmega328P__)   // Arduino Uno, Fio
    g_bUno = true;
  #endif  

}

String makeLength(int data, int len)
{
  String strReturn = "";

  // Remove the sign before converting to string.
  strReturn = String(abs(data), DEC);
  
  int nLength = strReturn.length();

  for(int i=0; i<len-nLength-1; i++)
    strReturn = "0" + strReturn;

  // Finally, writing the sign.
  if(data >= 0)
    strReturn = "+" + strReturn;
  else
    strReturn = "-" + strReturn;
  
  return strReturn;
}

void releaseMotor()
{
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
}

void outputVoltage(int level)
{
  Wire.beginTransmission(PCF8591); 
  Wire.write(0x40);                 // sets the PCF8591 into a DA mode
  Wire.write(level);                // sets the outputn
  Wire.endTransmission();
}

void commandMode(byte btReceiveByte)
{
  String strTransmitData = "";

  switch (btReceiveByte)
  {
    // Identify a arduion board
    case 'i':
      strTransmitData = "Arduino";
      Serial.print(strTransmitData);  
      break;
  
    // Set zero of stroke
    case 'z':
      g_iPositionCount = 0;
      break;     

    case 'o':
      g_bVoltageFollower = true;
      break;
  
    case 't':
      g_bStepMotor = true;
      break;

    case 'l':
      g_bLoadcell = true;
      break;      
  
    case 'e':
      releaseMotor();
      break;

    case 'w':       // Write the stroke
    case 'm':       // Move stroke
    case 'v':       // Output voltage
      g_btCommand = btReceiveByte;
      g_iDataMode = N_MODE;
      g_iNumberData = 0;
      g_nNumberDataCount = 0;
      break;
  
    // Request data to PC
    case 'r':
      // Current    
      strTransmitData = "c";
      strTransmitData = strTransmitData + makeLength(analogRead(CURRENT_PIN), NUMBER_SIZE);
  
      // Stroke
      strTransmitData = strTransmitData + "s";
      if (true == g_bStepMotor)
      { 
        // Caution : the sign is inverted. 
        strTransmitData = strTransmitData + makeLength(-g_iPositionCount, NUMBER_SIZE);
      }
      else
      {
        strTransmitData = strTransmitData + makeLength(analogRead(STROKE_PIN), NUMBER_SIZE);    
      }          
  
      // Force
      strTransmitData = strTransmitData + "f";
      if (g_bLoadcell == true)
      {
        #if defined(__AVR_ATmega328P__)
          strTransmitData = strTransmitData + makeLength(-hx711Scale.getGram(), NUMBER_SIZE);
        #else
          ' Can't use the loadcell except Uno.
          strTransmitData = strTransmitData + makeLength(0, NUMBER_SIZE);
        #endif
      }
      else
        strTransmitData = strTransmitData + makeLength(analogRead(FORCE_PIN), NUMBER_SIZE);
  
      strTransmitData =  strTransmitData + "e";
      Serial.print(strTransmitData);
      break;
  
    default:
      // Received a wrong command
      Serial.print("x1");
      break;
  }    
}

void finishN_Mode()
{
  g_iDataMode = C_MODE;             
  g_nNumberDataCount = 0;
  g_iNumberData = 0;
  g_iSign = 1;      
}

void dataMode(byte btReceiveByte)
{
  String strTemp = "";  
  int iTempData = 0;
  int iVoltageLevel = 0;  
  
  if(g_iDataMode == N_MODE)
  {
    strTemp = (char)btReceiveByte;
    
    switch (g_nNumberDataCount)
    {
      case 0:
        if(strTemp == "+")
          g_iSign = 1;
        else if(strTemp == "-")
          g_iSign = -1;
        else
        {
          Serial.print("x5");    // Bad sign charactor
          finishN_Mode();
          return;
        }
        break;
      case 1:
        iTempData = strTemp.toInt() * 1000 * g_iSign;
        break;
      case 2:
        iTempData = strTemp.toInt() * 100 * g_iSign;
        break;
      case 3:
        iTempData = strTemp.toInt() * 10 * g_iSign;
        break;
      case 4:
        iTempData = strTemp.toInt() * g_iSign;
        break;
      default:
        // Count Error of a Number Data
        Serial.print("x2");
        finishN_Mode();       
        return;        
        break;
    }
                    
    g_iNumberData += iTempData;       
    g_nNumberDataCount ++;
  
    // Check the NuberDataCount after increasing.
    if( g_nNumberDataCount >= NUMBER_SIZE)
    {
      switch (g_btCommand)
      {
        case 'w':
          // Caution : the sign is inverted.
          g_iPositionCount = -g_iNumberData;
          break;
        
        case 'm':
          // Even though Arduino received move command, it don't execute motor if g_bStepMotor is off.
          // Caution : the sign is inverted.
          if (true == g_bStepMotor)
          {        
            stepper.step(-g_iNumberData);
            g_iPositionCount += -g_iNumberData;
          }
          break;
  
        case 'v':
          // Even though Arduino received voltage command, it don't output voltage if g_bVoltageFollower is off.
          if (true == g_bVoltageFollower)
          {        
            iVoltageLevel = g_iNumberData;
            outputVoltage(iVoltageLevel);              
          }
          break;
  
        default:
          // Command Error while a Number Data is saving
          Serial.print("x3");
          break;
      }

      finishN_Mode();
    }
  }
  else
  {
    // Received a Number Data during C_MODE
    Serial.print("x4");
  }  
}

void loop() 
{
  byte btReceiveByte;

  if (Serial.available() > 0)
  {    
    btReceiveByte = Serial.read();

    if(false == isDigit(btReceiveByte) && btReceiveByte != '-' && btReceiveByte != '+')
    {
      commandMode(btReceiveByte);
    }
    else
    {    
      dataMode(btReceiveByte);
    }  
  }
}

