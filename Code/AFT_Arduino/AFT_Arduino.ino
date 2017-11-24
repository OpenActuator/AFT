#define   CURRENT_PIN   A0
#define   STROKE_PIN    A3
#define   FORCE_PIN     A6

void setup()
{  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  while (!Serial){
    ; // wait for serial port to connect. Needed for native USB port only
  } 

  analogReadResolution(12);
}

String makeLength(int data, int len)
{
  String strReturn = "";
  strReturn = String(data, DEC);
  
  int nLength = strReturn.length();

  for(int i=0; i<len-nLength; i++)
  {
    strReturn = "0" + strReturn;
  }
  return strReturn;
}

void loop() 
{
  byte btReceiveByte;
  String strTransmittData = "";

  if (Serial.available() > 0)
  {    
    btReceiveByte = Serial.read();
 
    switch (btReceiveByte)
    {
      case 'i':
        // Identity
        strTransmittData = "arduino";
        Serial.print(strTransmittData);  
        break;      
      
      case 'r':
        // Current    
        strTransmittData = "c";
        strTransmittData = strTransmittData +  makeLength(analogRead(CURRENT_PIN), 4);
        Serial.print(strTransmittData);
        
        // Stroke
        strTransmittData = "t";
        strTransmittData = strTransmittData +  makeLength(analogRead(STROKE_PIN), 4);
        Serial.print(strTransmittData);

        // Force
        strTransmittData = "f";
        strTransmittData = strTransmittData +  makeLength(analogRead(FORCE_PIN), 4);
        Serial.print(strTransmittData);

        strTransmittData = "e";
        Serial.print(strTransmittData);
        break;

      default:
        //Serial.println("Received a wrong command.");
        break;
    }      
  }
}

