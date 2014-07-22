// Libraries for the Adafruit RGB/LCD Shield
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

// Libraries for the DS18B20 Temperature Sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// So we can save and retrieve settings
#include <EEPROM.h>

// ************************************************
// Pin definitions
// ************************************************

// Output Relay
#define RelayPin 7

// Output Fan
#define FAN_PIN 8

// star/stop button
#define S_BUTT 9

//buzzer
#define BUZ 12

// One-Wire Temperature Sensor
#define ONE_WIRE_BUS 2
#define ONE_WIRE_PWR 3
#define ONE_WIRE_GND 4

//Define Variables we'll be connecting to
double Setpoint;
double Input;
double Output;

volatile long onTime = 0; // bho .-.

unsigned long previousMillis =0;
unsigned long ttemp = 0;
int istru=0;

//valore iniziale start button
int buttonState = 0;
boolean start = false;

// tempi
int T;
//temp min
double Tm;

// EEPROM addresses for persisted data
const int SpAddress = 0;
const int TAddress = 8;
const int TmAddress = 16;

// 10 second Time Proportional Output window
int WindowSize = 10000; 
unsigned long windowStartTime;

// ************************************************
// DiSplay Variables and constants
// ************************************************

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define BUTTON_SHIFT BUTTON_SELECT

unsigned long lastInput = 0; // last button press

byte degree[8] = // define the degree symbol 
{ 
 B00110, 
 B01001, 
 B01001, 
 B00110, 
 B00000,
 B00000, 
 B00000, 
 B00000 
}; 

//erase??
//const int logInterval = 10000; // log every 10 seconds
//long lastLogTime = 0;

// ************************************************
// States for state machine
// ************************************************
enum operatingState { OFF = 0, SETP, TUNE_T, TUNE_TM, RUN};
operatingState opState = OFF;

// ************************************************
// Sensor Variables and constants
// Data wire is plugged into port 2 on the Arduino

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress tempSensor;

// ************************************************
// Setup and diSplay initial screen
// ************************************************
void setup()
{
   Serial.begin(9600);

   // Initialize Relay Control:

   pinMode(RelayPin, OUTPUT);    // Output mode to drive relay
   digitalWrite(RelayPin, LOW);  // make sure it is off to start

   pinMode(FAN_PIN, OUTPUT); // Output fan
   digitalWrite(FAN_PIN, LOW); // setto a low (spento)

   //bottone di start
   pinMode(S_BUTT, INPUT);
   
   //Buz
   pinMode(BUZ, OUTPUT);

   pinMode(ONE_WIRE_GND, OUTPUT);
   digitalWrite(ONE_WIRE_GND, LOW);

   pinMode(ONE_WIRE_PWR, OUTPUT);
   digitalWrite(ONE_WIRE_PWR, HIGH);

   // LCD DiSplay 

   lcd.begin(16, 2);
   lcd.createChar(1, degree);
   
   lcd.print(F("Software By"));
   lcd.setCursor(0, 1);
   lcd.print(F("Marco Ferraioli"));

   // Start up the DS18B20 One Wire Temperature Sensor

   sensors.begin();
   if (!sensors.getAddress(tempSensor, 0)) 
   {
      lcd.setCursor(0, 1);
      lcd.print(F("Errore Sensore"));
   }
   sensors.setResolution(tempSensor, 12);
   sensors.setWaitForConversion(false);

   delay(5000);  // Splash screen
   
   LoadParameters();
   playShortBeep();
   playShortBeep();
   playShortBeep();
}

void loop()
{
   // wait for button release before changing state
   while(ReadButtons() != 0) {}
   
   Serial.println(opState);
   
   lcd.clear();

   switch (opState)
   {
    case OFF:
      Off();
      break;
    case SETP:
      Tune_Sp();
      break;
    case RUN:
      Run();
      break;
    case TUNE_T:
      TuneT();
      break;
    case TUNE_TM:
      TuneTM();
      break;
    }
  
}

void playShortBeep()
{
	digitalWrite(BUZ, HIGH);
	delay(400);
	digitalWrite(BUZ, LOW);
	delay(400);
}

void bstart(){
     buttonState = digitalRead(S_BUTT);
   
   if (buttonState == HIGH) {
        if(!start)     
          start=true;
        else
          start=false;
  } 
}


void Off()
{
   lcd.setBacklight(1);
   digitalWrite(RelayPin, LOW);  // make sure it is off
   lcd.print(F("Macchina 1"));
   lcd.setCursor(0, 1);
   lcd.print(F("Firmware: 1.2 v."));
   
   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();
      if (buttons & BUTTON_RIGHT)
      {
        opState = RUN;
        return;
      }
   }
   sensors.requestTemperatures(); // Start an asynchronous temperature reading

}

void Tune_Sp()
{
  digitalWrite(RelayPin, LOW);  // make sure it is off
   lcd.print(F("Temperatura:"));
   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 0.1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = RUN;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = TUNE_T;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Setpoint += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Setpoint -= increment;
         delay(200);
      }
      
      lcd.setCursor(0,1);
      lcd.print(Setpoint);
      lcd.print(" ");
   }
}

void TuneT()
{
  digitalWrite(RelayPin, LOW);  // make sure it is off
   lcd.print(F("Tempo Staz.:"));

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();
      int increment = 1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = SETP;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = TUNE_TM;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         T += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         T -= increment;
         delay(200);
      }
      
      lcd.setCursor(0,1);
      lcd.print(T);
      lcd.print(" ");
   }
}

void TuneTM()
{
  digitalWrite(RelayPin, LOW);  // make sure it is off
   lcd.print(F("Temp. Min.:"));
   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 0.1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = TUNE_T;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = RUN;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Tm += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Tm -= increment;
         delay(200);
      }
      
      lcd.setCursor(0,1);
      lcd.print(Tm);
      lcd.print(" ");
   }
}

// ************************************************
// Funzione Principale
// ************************************************
void Run()
{
  
   // set up the LCD's number of rows and columns: 
   lcd.print(F("Temp.:"));
   lcd.print(Setpoint);
   lcd.write(1);
   lcd.print(F("C"));

   SaveParameters();

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();
      if (buttons & BUTTON_RIGHT)
      {
        opState = SETP;
        return;
      }
      else if (buttons & BUTTON_LEFT)
      {
        opState = OFF;
        return;
      }
      bstart();

    Input = sensors.getTempC(tempSensor);
    sensors.requestTemperatures(); // prime the pump for the next one - but don't wait

      
      lcd.setCursor(0,1);
      lcd.print(F("T. Att.:"));
      lcd.print(Input);
      lcd.write(1);
      lcd.print(F("C"));
      
      if(start)
        controll();
     
      delay(100);
   }
}

void onResistance(){
  Input = sensors.getTempC(tempSensor);
  sensors.requestTemperatures(); // prime the pump for the next one - but don't wait

  digitalWrite(RelayPin, HIGH);
  lcd.setCursor(0,1);
  lcd.print(F("T. Att.:"));
  lcd.print(Input);
  lcd.write(1);
  lcd.print(F("C"));

  Input = sensors.getTempC(tempSensor);
  sensors.requestTemperatures(); // prime the pump for the next one - but don't wait
  
  if(Input<Setpoint){ onResistance(); }
  else{ digitalWrite(RelayPin, LOW); istru++; return ; }

  /*
  do{
    Input = sensors.getTempC(tempSensor);
    sensors.requestTemperatures(); // prime the pump for the next one - but don't wait

    digitalWrite(RelayPin, HIGH);
    lcd.setCursor(0,1);
    lcd.print(F("T. Att.:"));
    lcd.print(Input);
    lcd.write(1);
    lcd.print(F("C"));
  }while(Input<Setpoint);
  digitalWrite(RelayPin, LOW);
  */

  return ;
}

void statTemp(){  
    EEPROM_writeDouble(24, (T * 1000.00) + millis());
    long n = EEPROM_readDouble(24);  
    //static long n= (T * 1000.00) + millis();
    Serial.print(T);
    Serial.print(" ");
    static long a = T * 1000.00;
    Serial.print(a);
    Serial.print(" ");
    Serial.print(millis());
    Serial.print(" ");
    Serial.println(n);
    
     while(millis()< n){
      
      Input = sensors.getTempC(tempSensor);
      sensors.requestTemperatures();
      lcd.setCursor(0,1);
      lcd.print(F("T. Att.:"));
      lcd.print(Input);
      lcd.write(1);
      lcd.print(F("C"));
      
      if (Input<Setpoint) {
        digitalWrite(RelayPin, HIGH);
      } else if(Input>Setpoint || Input == Setpoint) {
        digitalWrite(RelayPin, LOW);}
        Serial.print("--");
        Serial.print(millis());
    Serial.print(" ");
    Serial.println(n);
    }
    istru++;
    return ;
}



void startFan(){
  Input = sensors.getTempC(tempSensor);
  sensors.requestTemperatures(); // prime the pump for the next one - but don't wait

  digitalWrite(FAN_PIN, HIGH);
  lcd.setCursor(0,1);
  lcd.print(F("T. Att.:"));
  lcd.print(Input);
  lcd.write(1);
  lcd.print(F("C"));

  Input = sensors.getTempC(tempSensor);
  sensors.requestTemperatures(); // prime the pump for the next one - but don't wait
  
  if(Input>Tm){ startFan(); }
  else{ digitalWrite(FAN_PIN, LOW); istru++; return ; }

  /*
  do{
    Input = sensors.getTempC(tempSensor);
    sensors.requestTemperatures(); // prime the pump for the next one - but don't wait

    digitalWrite(FAN_PIN, HIGH);
    lcd.setCursor(0,1);
    lcd.print(F("T. Att.:"));
    lcd.print(Input);
    lcd.write(1);
    lcd.print(F("C"));
  }while(Input>Tm);
  digitalWrite(FAN_PIN, LOW);
  */

  return ;
}

void controll(){


  if(!start)
    return ;

  if(start){
    if(istru==0){
      digitalWrite(RelayPin, LOW);
      digitalWrite(FAN_PIN, LOW);
      onResistance();
    }
    if(istru==1){
      digitalWrite(RelayPin, LOW);
      digitalWrite(FAN_PIN, LOW);
      statTemp();
    }
    if(istru==2){
      digitalWrite(RelayPin, LOW);
      digitalWrite(FAN_PIN, LOW);
      startFan();
    }
    if(istru==3){
      digitalWrite(RelayPin, LOW);
      digitalWrite(FAN_PIN, LOW);      
      start =false;
      opState = OFF;
      playShortBeep();
      playShortBeep();
      playShortBeep();
      istru=0;
    }
  }
  delay(100);
  return ;
} 

// ************************************************
// Save to EEPROM
// ************************************************
void SaveParameters()
{
   if (Setpoint != EEPROM_readDouble(SpAddress))
   {
      EEPROM_writeDouble(SpAddress, Setpoint);
   }
   if (T != EEPROM_readDouble(TAddress))
   {
      EEPROM_writeDouble(TAddress, T);
   }
   if (Tm != EEPROM_readDouble(TmAddress))
   {
      EEPROM_writeDouble(TmAddress, Tm);
   }
}

// ************************************************
// Load to EEPROM
// ************************************************
void LoadParameters()
{
   Setpoint = EEPROM_readDouble(SpAddress);
   T = EEPROM_readDouble(TAddress);
   Tm = EEPROM_readDouble(TmAddress);
   
   if (isnan(Setpoint))
   {
     Setpoint = 60;
   }
   if (isnan(T))
   {
     T = 1;
   }
   if (isnan(Tm))
   {
     Tm = 45;
   }
}

uint8_t ReadButtons()
{
  uint8_t buttons = lcd.readButtons();
  if (buttons != 0)
  {
    lastInput = millis();
  }
  return buttons;
}

// ************************************************
// Write floating point values to EEPROM
// ************************************************
void EEPROM_writeDouble(int address, double value)
{
   byte* p = (byte*)(void*)&value;
   for (int i = 0; i < sizeof(value); i++)
   {
      EEPROM.write(address++, *p++);
   }
}

// ************************************************
// Read floating point values from EEPROM
// ************************************************
double EEPROM_readDouble(int address)
{
   double value = 0.0;
   byte* p = (byte*)(void*)&value;
   for (int i = 0; i < sizeof(value); i++)
   {
      *p++ = EEPROM.read(address++);
   }
   return value;
}
