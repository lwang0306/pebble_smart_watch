/***************************************************************************

                     Copyright 2008 Gravitech
                        All Rights Reserved

****************************************************************************/

/***************************************************************************
 File Name: I2C_7SEG_Temperature.pde

 Hardware: Arduino Diecimila with 7-SEG Shield

 Description:
   This program reads I2C data from digital thermometer and display it on 7-Segment

 Change History:
   03 February 2008, Gravitech - Created

****************************************************************************/

#include <Wire.h> 
 
#define BAUD (9600)    /* Serial baud define */
#define _7SEG (0x38)   /* I2C address for 7-Segment */
#define THERM (0x49)   /* I2C address for digital thermometer */
#define EEP (0x50)     /* I2C address for EEPROM */
#define RED (3)        /* Red color pin of RGB LED */
#define GREEN (5)      /* Green color pin of RGB LED */
#define BLUE (6)       /* Blue color pin of RGB LED */

#define COLD (23)      /* Cold temperature, drive blue LED (23c) */
#define HOT (26)       /* Hot temperature, drive red LED (27c) */

const byte NumberLookup[16] =   {0x3F,0x06,0x5B,0x4F,0x66,
                                 0x6D,0x7D,0x07,0x7F,0x6F, 
                                 0x77,0x7C,0x39,0x5E,0x79,0x71};

/* Function prototypes */
void Cal_temp (int&, byte&, byte&, bool&);
void Dis_7SEG (int, byte, byte, bool, bool);
void Send7SEG (byte, byte);
void SerialMonitorPrint (byte, int, bool, bool);
void UpdateRGB (byte);
void Cal_temp_f (int&, byte&);
void Dis_7SEG_Timer();
void Light_Changing();

bool Working = true;
int Timer_Counter = -1;
int Light_Counter = -1;

/***************************************************************************
 Function Name: setup

 Purpose: 
   Initialize hardwares.
****************************************************************************/

void setup() 
{ 
  Serial.begin(BAUD);
  Wire.begin();        /* Join I2C bus */
  pinMode(RED, OUTPUT);    
  pinMode(GREEN, OUTPUT);  
  pinMode(BLUE, OUTPUT);   
  delay(500);          /* Allow system to stabilize */
} 

/***************************************************************************
 Function Name: loop

 Purpose: 
   Run-time forever loop.
****************************************************************************/
 
void loop() 
{ 
  int Decimal;
  byte Temperature_H, Temperature_L, counter, counter2;
  bool IsPositive;
  bool IsFahrenheit = false;
  int input = 0;
  
  /* Configure 7-Segment to 12mA segment output current, Dynamic mode, 
     and Digits 1, 2, 3 AND 4 are NOT blanked */
     
  Wire.beginTransmission(_7SEG);   
  byte val = 0; 
  Wire.write(val);
  val = B01000111;
  Wire.write(val);
  Wire.endTransmission();
  
  /* Setup configuration register 12-bit */
     
  Wire.beginTransmission(THERM);  
  val = 1;  
  Wire.write(val);
  val = B01100000;
  Wire.write(val);
  Wire.endTransmission();
  
  /* Setup Digital THERMometer pointer register to 0 */
     
  Wire.beginTransmission(THERM); 
  val = 0;  
  Wire.write(val);
  Wire.endTransmission();
  
  /* Test 7-Segment */
  for (counter=0; counter<8; counter++)
  {
    Wire.beginTransmission(_7SEG);
    Wire.write(1);
    for (counter2=0; counter2<4; counter2++)
    {
      Wire.write(1<<counter);
    }
    Wire.endTransmission();
    delay (250);
  }
   
  int swallow = 0;
  
  while (1)
  {
    Wire.requestFrom(THERM, 2);
    Temperature_H = Wire.read();
    Temperature_L = Wire.read();
    
    // check if "Working"
    if (Working) {
      /* Calculate temperature */
      Cal_temp (Decimal, Temperature_H, Temperature_L, IsPositive);
      if (IsFahrenheit) {
        Cal_temp_f(Decimal, Temperature_H);
      }    
    
      /* Display temperature on the serial monitor. 
         Comment out this line if you don't use serial monitor.*/
      SerialMonitorPrint (Temperature_H, Decimal, IsPositive, IsFahrenheit);
    
      /* Update RGB LED.*/
      if (Light_Counter >= 0) { // in light changing mode
        Light_Changing();
      } else { // not in light changing mode
        UpdateRGB (Temperature_H);
      }
      
    
      /* Display temperature or timer on the 7-Segment */ 
      if (Timer_Counter >= 0) { // in timer mode
        Dis_7SEG_Timer();
        if (swallow > 1) {
          Timer_Counter++;
        } else {
          swallow++;
        }
      } else { // not in timer mode
        Dis_7SEG (Decimal, Temperature_H, Temperature_L, IsPositive, IsFahrenheit);
      }
    
      delay (1000);        /* Take temperature read every 1 second */
    }
    
    // check input from server
    if (Serial.available() > 0) {
      input = Serial.read();
      
      // if input is '3' - Celsius mode
      if (input == 51) {
        Timer_Counter = -1;
        IsFahrenheit = false;
      } else if (input == 52) { // if input is '4' - Fahrenheit mode
        Timer_Counter = -1;
        IsFahrenheit = true;
      } else if (input == 50) { // if input is '2' - Pause mode
        Timer_Counter = -1;
        Working = false;
        Dis_7SEG(0, 0, 0, true, false);
      } else if (input == 49) { // if input is '1' - Resume mode
        Timer_Counter = -1;
        Working = true;
      } else if (input == 54) { // if input is '6' - Timer mode
        swallow = 0;
        Timer_Counter++;
      } else if (input == 53) { // if input is '5' - Light mode
        Light_Counter++;
      }
      
    }
  }
} 


void Light_Changing() {
  
  if (Light_Counter % 4 == 0) { // no color
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }
  if (Light_Counter % 4 == 1) { // red
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }
  if (Light_Counter % 4 == 2) { // green
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BLUE, LOW);
  }
  if (Light_Counter % 4 == 3) { // blue
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, HIGH);
  }
  if (Light_Counter >= 8) {
    Light_Counter = -1;
  } else {
    Light_Counter++;
  }
  
}


/***************************************************************************
 Function Name: Cal_temp

 Purpose: 
   Calculate temperature from raw data.
****************************************************************************/
void Cal_temp (int& Decimal, byte& High, byte& Low, bool& sign)
{
  if ((High&B10000000)==0x80)    /* Check for negative temperature. */
    sign = 0;
  else
    sign = 1;
    
  High = High & B01111111;      /* Remove sign bit */
  Low = Low & B11110000;        /* Remove last 4 bits */
  Low = Low >> 4; 
  Decimal = Low;
  Decimal = Decimal * 625;      /* Each bit = 0.0625 degree C */
  
  if (sign == 0)                /* if temperature is negative */
  {
    High = High ^ B01111111;    /* Complement all of the bits, except the MSB */
    Decimal = Decimal ^ 0xFF;   /* Complement all of the bits */
  }  
}

/***************************************************************************
 Function Name: Cal_temp_f

 Purpose: 
   Convert temperature from celsius to fahrenheit.
****************************************************************************/
void Cal_temp_f (int& Decimal, byte& High)
{
  long x = High;
  long C_temp_10k = x * 10000 + Decimal;
  long F_temp_10k = C_temp_10k * 9 / 5 + 320000;
  Decimal = F_temp_10k % 10000;
  High = F_temp_10k / 10000;
}

/***************************************************************************
 Function Name: Dis_7SEG

 Purpose: 
   Display number on the 7-segment display.
****************************************************************************/
void Dis_7SEG (int Decimal, byte High, byte Low, bool sign, bool IsFahrenheit)
{
  byte Digit = 4;                 /* Number of 7-Segment digit */
  byte Number;                    /* Temporary variable hold the number to display */
  
  if (!Working) {
    while (Digit > 0)                 /* Clear the rest of the digit */
    {
      Send7SEG (Digit,0x40);
      Digit--;    
    }
    return;
  }
  
  if (sign == 0)                  /* When the temperature is negative */
  {
    Send7SEG(Digit,0x40);         /* Display "-" sign */
    Digit--;                      /* Decrement number of digit */
  }
  
  if (High > 99)                  /* When the temperature is three digits long */
  {
    Number = High / 100;          /* Get the hundredth digit */
    Send7SEG (Digit,NumberLookup[Number]);     /* Display on the 7-Segment */
    High = High % 100;            /* Remove the hundredth digit from the TempHi */
    Digit--;                      /* Subtract 1 digit */    
  }
  
  if (High > 9)
  {
    Number = High / 10;           /* Get the tenth digit */
    Send7SEG (Digit,NumberLookup[Number]);     /* Display on the 7-Segment */
    High = High % 10;            /* Remove the tenth digit from the TempHi */
    Digit--;                      /* Subtract 1 digit */
  }
  
  Number = High;                  /* Display the last digit */
  Number = NumberLookup [Number]; 
  if (Digit > 1)                  /* Display "." if it is not the last digit on 7-SEG */
  {
    Number = Number | B10000000;
  }
  Send7SEG (Digit,Number);  
  Digit--;                        /* Subtract 1 digit */
  
  if (Digit > 0)                  /* Display decimal point if there is more space on 7-SEG */
  {
    Number = Decimal / 1000;
    Send7SEG (Digit,NumberLookup[Number]);
    Digit--;
  }

  if (Digit > 0)                 /* Display "c" or "f" if there is more space on 7-SEG */
  {
  if (!IsFahrenheit) 
    Send7SEG(Digit, 0x58);
  else
    Send7SEG(Digit, 0x71);
  Digit--;
  }
  
  if (Digit > 0)                 /* Clear the rest of the digit */
  {
    Send7SEG (Digit,0x00);
    Digit--;    
  }  
}

/***************************************************************************
 Function Name: Dis_7SEG_Timer

 Purpose: 
   Display timer on the 7-segment display.
****************************************************************************/
void Dis_7SEG_Timer() {
  byte Digit = 4;                 /* Number of 7-Segment digit */
  byte Number;                    /* Temporary variable hold the number to display */
  int Counter = Timer_Counter;
  
  if (!Working) {
    while (Digit > 0)                 /* Clear the rest of the digit */
    {
      Send7SEG (Digit,0x40);
      Digit--;    
    }
    return;
  }
  
  if (Counter > 9999) {
    Counter = Counter % 10000;
  }
  
  if (Counter > 999) {
    Number = Counter / 1000;
    Send7SEG (Digit,NumberLookup[Number]);
    Counter = Counter % 1000;
  } else if (Timer_Counter < 1000) {
    Send7SEG (Digit,0x0);
  } else {
    Send7SEG (Digit,NumberLookup[0]);
  }
  Digit--;
  
  if (Counter > 99) {
    Number = Counter / 100;
    Send7SEG (Digit,NumberLookup[Number]);
    Counter = Counter % 100;
  } else if (Timer_Counter < 100) {
    Send7SEG (Digit,0x0);
  } else {
    Send7SEG (Digit,NumberLookup[0]);
  }
  Digit--;
  
  if (Counter > 9) {
    Number = Counter / 10;
    Send7SEG (Digit,NumberLookup[Number]);
    Counter = Counter % 10;
  } else if (Timer_Counter < 10) {
    Send7SEG (Digit,0x0);
  } else {
    Send7SEG (Digit,NumberLookup[0]);
  }
  Digit--;
  
  Number = Counter;
  Send7SEG (Digit,NumberLookup[Number]);
  Digit--;
  
}

/***************************************************************************
 Function Name: Send7SEG

 Purpose: 
   Send I2C commands to drive 7-segment display.
****************************************************************************/

void Send7SEG (byte Digit, byte Number)
{
  Wire.beginTransmission(_7SEG);
  Wire.write(Digit);
  Wire.write(Number);
  Wire.endTransmission();
}

/***************************************************************************
 Function Name: UpdateRGB

 Purpose: 
   Update RGB LED according to define HOT and COLD temperature. 
****************************************************************************/

void UpdateRGB (byte Temperature_H)
{
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);        /* Turn off all LEDs. */
  
  if (Temperature_H <= COLD)
  {
    digitalWrite(BLUE, HIGH);
  }
  else if (Temperature_H >= HOT)
  {
    digitalWrite(RED, HIGH);
  }
  else 
  {
    digitalWrite(GREEN, HIGH);
  }
}

/***************************************************************************
 Function Name: SerialMonitorPrint

 Purpose: 
   Print current read temperature to the serial monitor.
****************************************************************************/
void SerialMonitorPrint (byte Temperature_H, int Decimal, bool IsPositive, bool IsFahrenheit)
{
//    Serial.print("The temperature is ");
    if (!IsPositive)
    {
      Serial.print("-");
    }
    Serial.print(Temperature_H, DEC);
    Serial.print(".");
    Serial.print(Decimal, DEC);
    if (IsFahrenheit)
      Serial.print(" F");
    else
      Serial.print(" C");
    Serial.print("\n\n");
}
    


