/********************************************************************/
// First we include the libraries
#include <OneWire.h> 
#include <EEPROM.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"
/*********************************************************************/
// Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 3
#define SAMPLE_MAX 4096
// External eeprom
#define EEPROM_ADDRESS 0x50 
/********************************************************************/
// Setup the RTC instance DS1307 Adafruit 0x68 I2C address
RTC_DS1307 rtc;
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 
// Configure the LCD pins.
const int rs = 12, en = 11, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
/********************************************************************/ 
// Configure the Relay Pin for the heater and the water pump
const int relayHeater = 9; //LED pins for temperature control 
//Key connections 
const int up_key = 8, down_key = 10, select_key = 2;
/********************************************************************/ 
// Initialization of additional status variables
bool storeT = LOW, relayStatus = LOW;
// eeprom setPoint address, time interval
int eeSetPointAddress = 0, eeSetDeltaTAddress = sizeof(float), interval = 500, temp = 0;
// temperature t set to 0
float temperat = 0, setPoint = 0, deltaT = 0;
// initialization of time interval
long trTemp = 0, trStore = 0, teStore = 0, tTime = 0, tStart = 0, tStop = 0, tTotalRun = 0;
// FSM State
unsigned int state;
// Initialization of string
String readString;
/********************************************************************/ 
// in order to store the temperature into an external eeprom we need 
// a data structure which will facilitate the byte handling
union tempU {
  float tempF;
  byte tempB[4];
};
/********************************************************************/ 
// we capture the last write cell to avoid to overwrite if we restart.
union lastU {
  int lastI;
  byte lastB[2];
};
/********************************************************************/ 
// in order to store the temperature into an external eeprom we need 
// a data structure which will facilitate the byte handling
union timeU {
  long timeL;
  byte timeB[4];
};

int tempSample = 4;
/********************************************************************/ 
// write the basic strings to the lcd
void lcdSetup()
{
 // set up the LCD's number of columns and rows:
 lcd.begin(20, 4);             
 lcd.setCursor(0,0); 
}

void lcdMenu() {
 lcd.clear();             
 lcd.setCursor(0,0); 
 lcd.print("Temperature:");// print in lcd this word 
 lcd.setCursor(0,1);//Change cursor position
 lcd.print("Setpoint:");// print in lcd this word 
 lcd.setCursor(0,2);//Change cursor position
 lcd.print("Hysteresis:");// print in lcd this word
 lcd.setCursor(0,3);//Change cursor position
 lcd.print("Status:");// print in lcd this word
}
/********************************************************************/ 
// read all eeprom content
void eepromReadAll(){
  // dump all the stored temp from the ext eeprom
  //read data back
  timeU timeFromEEprom;
  tempU tempFromEEprom;
  Serial.println("Reading data, full temperature log ...");
  for (int i = 0; i<tempSample; i+=4 )
  {
    for(int j = 0; j<4; j++)
    {
       timeFromEEprom.timeB[j]=readData(i+j);
    }
    i+=4;
    for(int j = 0; j<4; j++)
    {
       tempFromEEprom.tempB[j]=readData(i+j);
    }
    Serial.print(timeFromEEprom.timeL);
    Serial.print(" : ");
    Serial.println(tempFromEEprom.tempF);
  }
  Serial.println("Complete");
}

/********************************************************************/ 
//defines the writeEEPROM function
void writeData(unsigned int eaddress, byte data)
{
  Wire.beginTransmission(EEPROM_ADDRESS);
  // set the pointer position
  Wire.write((int)(eaddress >> 8));
  Wire.write((int)(eaddress & 0xFF));
  Wire.write(data);
  Wire.endTransmission();
  delay(10);
}
/********************************************************************/ 
//defines the readEEPROM function
byte readData(unsigned int eaddress)
{
  byte result = 0xFF;
  Wire.beginTransmission(EEPROM_ADDRESS);
  // set the pointer position
  Wire.write((int)(eaddress >> 8));
  Wire.write((int)(eaddress & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_ADDRESS,1); // get the byte of data
  result = Wire.read();
  return result;
}
/********************************************************************/ 
//RTC setup
void rtc_setup() {
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    //while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  //rtc.adjust(DateTime(2020, 04, 05, 22, 53, 0));
}

int readSerialDate(){ 
    int n = 0;
    while(1) {
      while (Serial.available()) {
        char c = Serial.read();  //gets one byte from serial buffer
        readString += c; //makes the string readString
        delay(2);  //slow looping to allow buffer to fill with next character
      }       
      if (readString.length() > 2) {
        n = readString.toInt();
        readString = "";
        break;
      }
    }
    return n;
}

void setTimeRTC() {
  int yearT = 2020;
  int monthT = 04;
  int dayT = 05;
  int hourT = 24;
  int minuteT = 00;

  Serial.println("Enter year : ");
  yearT = readSerialDate();
  Serial.println("Enter month : ");
  monthT = readSerialDate();
  Serial.println("Enter day : ");
  dayT = readSerialDate();
  Serial.println("Enter hours : ");
  hourT = readSerialDate();
  Serial.println("Enter minutes : ");
  minuteT = readSerialDate();

  Serial.println("Setting RTC Clock...");
  Serial.println(yearT);
  Serial.println(monthT);
  Serial.println(dayT);
  Serial.println(hourT); 
  Serial.println(minuteT);

  rtc.adjust(DateTime(yearT, monthT, dayT, hourT, minuteT, 0));
  Serial.print("Current date time is : ");

  rtc_time();
}
/********************************************************************/ 
// rtc print time
void rtc_time() {
  DateTime now = rtc.now();
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print(" ");
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.println("");
}
/********************************************************************/ 
// help function for the buttons, up and down
int getKey(int setValue) {
 //Get user input for setpoints  
 if(digitalRead(down_key)==LOW)
  {
    if(setValue>0)
    {
      setValue--;    
    }
    storeT = HIGH;
  }
 if(digitalRead(up_key)==LOW)
  {
    if(setValue<95)
    {
      setValue++;
    }
    storeT = HIGH;
  }   
  return setValue;
}


/********************************************************************/ 
// Initialize the total timer 
void resetRunTime() {
    Serial.println("Clearing total runtime... ");
    // Initialize the total timer 
    tTotalRun = 0;
    EEPROM.put(8*sizeof(float), tTotalRun);
    Serial.println("Completed");
}

/********************************************************************/ 
// Initialize the last eeprom location index
void resetEEProm() {
    lastU lastToEEprom;
    Serial.println("Clearing eeprom... ");
    // Initialize the last eeprom location index
    lastToEEprom.lastI = 0;
    for (int i=0; i<2; i++){
      writeData(i,lastToEEprom.lastB[i]);
    }
    Serial.println("Completed");
}

/********************************************************************/ 
// Banner menu
void bannerMenu() {
  Serial.println("Select one of the following options:");
  Serial.println("1 Reset runtime");
  Serial.println("2 Reset ext EEProm content");
  Serial.println("3 Dump ext EEProm content");
  Serial.println("4 Setup RTC time");
  Serial.println("5 Exit configuration menu");
}
/********************************************************************/ 
// help function to print the serial menu, dump data, set rtc.
void serialMenu() {
    bannerMenu();
    lcd.setCursor(7,1);
    lcd.print("Serial");
    lcd.setCursor(7,2);
    lcd.print("Config");
    Serial.println("Function menu");
    for (;;) {
        switch (Serial.read()) {
            case '1': resetRunTime(); bannerMenu(); break;
            case '2': resetEEProm(); bannerMenu(); break;
            case '3': eepromReadAll(); bannerMenu(); break;
            case '4': setTimeRTC(); bannerMenu(); break;
            case '5': return;
            default: continue;  // includes the case 'no input'
        }
    }
}

/********************************************************************/ 
// Main setup
void setup(void) 
{ 
 // start serial port 
 Serial.begin(9600); 
 Serial.println("Heater Temperature Control"); 
 
 // init relay pins 
 pinMode(relayHeater, OUTPUT);
 // heater relay is switched off 
 digitalWrite(relayHeater,LOW); 
 // Set point control, pins
 pinMode(up_key,INPUT);
 pinMode(down_key,INPUT);
 pinMode(select_key,INPUT);
 // pull up internal control
 digitalWrite(up_key,HIGH);
 digitalWrite(down_key,HIGH);
 digitalWrite(select_key,HIGH);
 
 // EEProm read the stored value check
 EEPROM.get(eeSetPointAddress, setPoint);
 EEPROM.get(eeSetDeltaTAddress, deltaT);
 EEPROM.get(8*sizeof(float), tTotalRun);

 // Activate the wire for I2C e2prom comm.
 Wire.begin();
 // Start up the library 
 sensors.begin(); 
 // RTC setup
 rtc_setup();
 // LCD setup 
 lcdSetup();
 
 //FSM State
 state = 2;
 
 // Recover stored variables, Setpoint, deltaT, samples
 if (isnan(setPoint))
  {
  setPoint = 25;
  } 
 if (isnan(deltaT))
  {
  deltaT = 3;
  }
  
  // restore the last saved ext eeprom location
  lastU lastSampleRestore;
  for (int i = 0; i<2; i++)
  {
    lastSampleRestore.lastB[i]=readData(i);
  }
  // tempSample will restart from the last saved location
  tempSample = lastSampleRestore.lastI;
  
  // time stamp 
  rtc_time();

  // if both key are pressed at boot, then the ext eeprom will be
  // reinitialized
  if(digitalRead(down_key)==LOW && digitalRead(up_key==LOW))
  {
    serialMenu();
  }
  // print the lcd template
  lcdMenu();

} 

void loop(void) 
{ 

 DateTime now = rtc.now(); 
 // call sensors.requestTemperatures() to issue a global temperature 
 // request to all devices on the bus 
 /********************************************************************/
 //Serial.print(" Requesting temperatures..."); 
 sensors.requestTemperatures(); // Send the command to get temperature readings 
 //Serial.println("Done"); 
 
 /********************************************************************/
 //Serial.print("Temperature is: "); 
 temperat = sensors.getTempCByIndex(0);
 //Serial.print(temperat);
 
 /********************************************************************/
 // Check which value has to update setPoint or DetlaT
 if(digitalRead(select_key)==LOW)
  {
   // if the switch is selected to the setPoint value then sets the temp
   temp = setPoint;
   setPoint = getKey(temp);
   lcd.setCursor(18,3);
   lcd.print("S");
  } else {
   // otherwise set the deltaT for the hysteresis
   temp = deltaT;
   deltaT = getKey(temp); 
   lcd.setCursor(18,3);
   lcd.print("H");
  }
  
 /********************************************************************/
 // Store the data into the internal eeprom
 if(millis() - trStore > 2000 && storeT == HIGH) 
  { 
    trStore = millis();
    storeT = LOW;
    Serial.println("Storing setpoint and hysteresis..."); 
    // Write to the eeprom the setPoint Value & the deltaT
    Serial.print("setpoint is: ");
    Serial.println(setPoint);
    Serial.print("hysteresis is: ");
    Serial.println(deltaT);
    EEPROM.put(eeSetPointAddress, setPoint);
    EEPROM.put(eeSetDeltaTAddress, deltaT);
    Serial.println("DONE"); 
  }

  /********************************************************************/
  // Store the data into the external eeprom for statistical reason every 84 seconds
  // 24h*60'*60" = 86400 seconds in a day we have 4Kbyte available - every float is 4 byte
  // we are capable to sample every 86400/1024 = 84 seconds -> ms are 84000
  if(millis() - teStore > 84000) 
  {
    teStore = millis();
    now = rtc.now();
    
    tempU tempToEEprom ;
    timeU timeToEEprom ;
    lastU lastToEEprom ;
    
    timeToEEprom.timeL = now.unixtime();
    
    tempToEEprom.tempF = temperat;
    
    if (tempSample == SAMPLE_MAX){
       tempSample = 0;
       Serial.println("Max external storage reached... restarting from 0");
    }
    
    for(int j = 0; j<4; j++)
    {
       writeData(tempSample+j,timeToEEprom.timeB[j]); 
    }
    tempSample+=4;
    
    for(int j = 0; j<4; j++)
    {
       writeData(tempSample+j,tempToEEprom.tempB[j]);
    }
    tempSample+=4;
           
    // store the last eeprom location index
    lastToEEprom.lastI = tempSample;
    for (int i=0; i<2; i++){
      writeData(i,lastToEEprom.lastB[i]);
    }
    Serial.print(timeToEEprom.timeL);
    Serial.print(" Stored latest temperature :");
    Serial.println(tempToEEprom.tempF);

  }

 /********************************************************************/
 // Check the temperature in the given intervall
 if(millis() - trTemp > interval) //Comparison between the elapsed time and the time in which the action is to be executed
  {
    trTemp = millis(); //"Last time is now"

    temperat = sensors.getTempCByIndex(0);//temperature value centigrades if you want farenheit change to
     //Below is for print data sensors in lcd 
    lcd.setCursor(14,0);
    lcd.print(temperat);
    lcd.setCursor(14,1);
    lcd.print(setPoint);
    lcd.setCursor(14,2);
    lcd.print(deltaT);
    // finally print the run information
    lcd.setCursor(17,3);
    lcd.print(relayStatus);
    lcd.setCursor(10,3);
    lcd.print(tTotalRun/60);

    digitalWrite(relayHeater,relayStatus);
    
    //Temperature control state machine
    switch (state)
    {
      case 1:
        if(temperat >= setPoint ) // if temperature above of 25 degrees
        {
          relayStatus = LOW;

          now = rtc.now();
          tStop = now.unixtime();

          tTotalRun += tStop - tStart;
          EEPROM.put(8*sizeof(float), tTotalRun);
          
          Serial.print("total on time: ");
          Serial.println(tTotalRun);

          state = 2;
        }
        break;
      case 2:
        if(temperat <= (setPoint-deltaT)) // if temperature is under 23 degrees
        {
          relayStatus = HIGH;
          
          now = rtc.now();
          tStart = now.unixtime();
          
          state = 1;
        }
        break;
    }

  }
}
