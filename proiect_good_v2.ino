#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

#define WLAN_SSID       "Roscatesipalmelafund"        // cannot be longer than 32 characters!
#define WLAN_PASS       "987AlaBala654"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

Adafruit_CC3000_Client www;  //defines how we will refer to the cc3000 connection object

// What page to grab!
#define WEBSITE      "192.168.0.101"  //NOTE: CC3000 doesn't seem to like the default localhost address of 127.0.0.1 - need to enter actual IP
#define WEBPAGE     "/SMP_test/json.php"


//LiquidCrystal lcd(12, NULL, 11, 9,8,7,6);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
RTC_Millis rtc;
const int buzzerPin = 4;
const int ledPin = 6;
const int buttonPin = 8;
int db_hour, db_minute, db_second;
String db_description = "";

//this is the counter that will be used for uploading data
long pollCounter = 1;
//interval at which to poll base station
//takes a value in seconds; add more multipliers if you want a more convenient unit
long pollInterval = 15L * 1000L;  //converts from millis; initial default value is to poll every 10 sec
long lastPoll;                     //stores the last time we polled
boolean pollFlag = false;          //tells the sketch whether to poll or not

//declare a variable to hold a numeric IP address
//can be overridden below if you use lookup
uint32_t ip = (192L << 24) | (168L<<16) | (0<<8) | 101;

// Setting Buzzer mode to False
boolean buzzer_mode = true;

// For LED
int ledState = LOW;
long previousMillis = 0; 
long interval = 100;  // Interval at which LED blinks

//read from database
int alarms[2][3];
String descriptions[2]; 
int line = 0;

void alarm_ON(){
  // If our button is pressed Switch off ringing and Setup
  int button_state = digitalRead(buttonPin);
  Serial.print(" \n\n--- STARE BUTON: ");
  Serial.print(button_state);

  if (button_state) {buzzer_mode = false;}
  
  if (buzzer_mode){
    Serial.print(" \n\n--- SE CANTA !! ");
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;   
      if (ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;
      // Switch the LED
      digitalWrite(ledPin, ledState);
    }
    tone(buzzerPin,50000);
  }
 
  // If alarm is off
  if (buzzer_mode == false) {
    // No tone & LED off
    Serial.print("--- NU SE MAI CANTA ---\n");
    noTone(buzzerPin);  
    digitalWrite(ledPin, LOW);
  
  }
  
}
  
void isTime(byte h, byte m){
  DateTime now = rtc.now();
  if(h == now.hour() && (m == now.minute())){
      alarm_ON();
    }
    else {buzzer_mode = true;}
}

void setup () {
  Serial.begin(9600);
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
  lcd.begin(16, 2);
  //lcd.noCursor();

  Serial.begin(9600);
  //Serial.println(F("Hello, CC3000!\n")); 

  //Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  /* Initialise the module */
  //Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    while(1);
  }
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));

   /* Wait for DHCP to complete */
  //Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
    {
      delay(100); // ToDo: Insert a DHCP timeout!
    }

  cc3000.printIPdotsRev(ip);
  
  //The Following are our output
  pinMode(ledPin,OUTPUT);
  pinMode(buzzerPin,OUTPUT);

  //Button is our Input
  pinMode(buttonPin, INPUT);
}

void displayTime(){
  Serial.print("\n---- SE AFISEAZA TIMPU ---\n");
  byte second, minute, hour, day, month, year, dayOfWeek;
  DateTime now = rtc.now();
  year = now.year();
  month = now.month();
  day = now.day();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  dayOfWeek = now.dayOfTheWeek();

  //LCD
  lcd.setCursor(0, 0);
  lcd.print(day);
  lcd.print("/");
  lcd.print(month);
  lcd.print("/");
  lcd.print(now.year());
  lcd.print(" ");
  switch(dayOfWeek){
    case 1:
      lcd.print("Mon");
      break;
    case 2:
      lcd.print("Tue");
      break;
    case 3:
      lcd.print("Wed");
      break;
    case 4:
      lcd.print("Thu");
      break;
    case 5:
      lcd.print("Fri");
      break;
    case 6:
      lcd.print("Sat");
      break;
    case 7:
      lcd.print("Sun");
      break;
}

  lcd.setCursor(4, 1);
  lcd.print(hour);
  lcd.print(":");
  if(now.minute() < 10){
    lcd.print("0");
   }
  lcd.print(minute);
  lcd.print(":");
  
  if(now.second() < 10){
    lcd.print("0");
   }
  lcd.print(now.second());

}

void loop () {
  displayTime();
  
  delay(1000);

  //check if it is time to poll
  if(pollFlag == true){
      Serial.println();
      Serial.print("Poll counter: "); Serial.println(pollCounter);
     
      String targetURI = WEBPAGE;
      targetURI += "?id=1";
//      targetURI = targetURI + "?id=" + pollCounter;
//      targetURI = targetURI + "&data_1=" + data_1;
//      targetURI = targetURI + "&data_2=" + data_2;
//      targetURI = targetURI + "&data_3=" + data_3;  
//      targetURI = targetURI + "?id=" + "1";
      
      /* Try connecting to the website */
      Serial.print("\r\nAttempting connection with data ");
      //Serial.println(targetURI);
      Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
      if (www.connected()) {
        Serial.println("Connected; requesting commands");
        www.fastrprint(F("GET "));
        www.print(targetURI);  //can't use fastrprint because it won't accept a variable
        www.fastrprint(F(" HTTP/1.0\r\n"));
        www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\n"));
        www.fastrprint(F("\r\n"));
        www.fastrprint(F("\r\n"));
        www.println();
      } else {
        Serial.println(F("Connection failed"));
        return;
      }
    
    
      //define a String to hold our json object
      String commands = "";
      //set a flag for when to start adding to String
      boolean readflag = false;
      
      //read in data one character at a time while connected    
      while (www.connected()) {
        while (www.available()) {
          char c = www.read();
          Serial.print(c);
         //start storing at the [ marker
         if (c=='[') {readflag = true;}
         if (c==']') {readflag = false;}
         //build data read from www into a String
         if ((readflag == true) && (c != '['))  {
          commands = commands + c;
         }
         if ((readflag == false) && (c != '!') && (c != ']'))  {
          db_description = db_description + c;
          }
        }
      }

      www.close();
       
      //now we should have all our commands in the String 'commands'
      //next we need to parse it into three separate integers
      //find out where the commas are
      int comma1 = commands.indexOf(',');
      int comma2 = commands.lastIndexOf(',');
    
      db_hour = (commands.substring(0,comma1)).toInt();
      db_minute = (commands.substring(comma1+1,comma2)).toInt();
      db_second = (commands.substring(comma2+1)).toInt(); 
    
//      isTime(db_hour,db_minute);
        
      Serial.print(("\r\nRESULTS:\r\n  db_hour: "));
      Serial.println(db_hour);
      Serial.print(("  db_minute: "));
      Serial.println(db_minute);
      Serial.print(("  db_second: "));
      Serial.println(db_second);
      Serial.print(("  db_description: "));
      Serial.println(db_description);
     
      //now that we've polled, get ready for next poll
      pollCounter = pollCounter + 1;  //increment the counter
      pollFlag = false;
      lastPoll = millis();
      //Serial.print("\r\n Next poll in ");
      Serial.println(pollInterval/1000);
  }  //end of what we do if pollFlag == true
   else  //what we do if pollFlag == false
  {
    
    //check to see if it's time to poll
    if (millis() >= (lastPoll + pollInterval))
     { //it's time to poll!
     pollFlag = true;
     }    
  }
  isTime(db_hour,db_minute);
}
