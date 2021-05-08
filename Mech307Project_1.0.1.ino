/* Drew Blais
 * Dust Boss v1.0.1 IDE - 05/05/2021
 * The following code is to be used with an Arduino Uno and an ESP8266 w/ AT Firmware, other firmwares will not work
 * with AT commands. This code could technically be flashed onto the Huzzah breakout and used as a solitary microcontroller.
 * Experimenting with this was difficult and may cause shortcomings in amperage and execution time. 
 * If you're more comfortable in Lua this ESP8266 comes factory with NodeMCU. 
 * This program outputs data to ThingSpeak for analysis but can be used to output in other ways as well. It connects
 * through a wifi network.
 * The only particulate sensor this will work with this AQI library is the Adafruit PMSA003I through I2C it can
 * be done through serial using the software serial library but using I2C makes it a little smoother and higher baudrate. 
 */
// --------------Libraries-------------- 
// PMSA003I
//#include <Adafruit_Sensor.h>
#include "Adafruit_PM25AQI.h" //This library also assigns the sensor its I2C adderss, 0x12, must edit library for multiple
// WiFi (ESP8266 w/ AT Firmware)
#include <SoftwareSerial.h>
// DHT11
#include <DHT.h>
#include <DHT_U.h>
// OLED
//#include <Wire.h>
////#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
// IR sensor?

// --------------Definitions-------------- 
// PMSA003I AQI
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();
// WiFi 
#define RX 2
#define TX 3
String AP = "Slow&Steady2G";       // AP NAME Hint: SSID
String PASS = "Jameson01"; // AP PASSWORD
String API = "RVQW9XVCGZM29QJW";   // Write API KEY - Found on ThingSpeak channel
String HOST = "api.thingspeak.com";
String PORT = "80";
String field = "field1";
int countTrueCommand;
int countTimeCommand; 
boolean found = false; 
SoftwareSerial esp8266(RX,TX);
// OLED
//#define SCREEN_WIDTH 128 // OLED display width, in pixels
//#define SCREEN_HEIGHT 64 // OLED display height, in pixels
//#define OLED_RESET  4
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 
// DHT11
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
// Relay
const byte relayPin = 12; 
byte relayState = 0;
// Piezo
byte piezoPin = 9;
byte fanPin = 11;

/* Using I2C for higher baudrates and pin real estate. If GPIO serial for PM sensor, must also baudrate9600
to use GPIO serial & uncomment this: */
//SoftwareSerial pmSerial(2, 3);

void setup() {
  pinMode(relayPin,OUTPUT);
  pinMode(fanPin,OUTPUT);
  digitalWrite(fanPin,HIGH);
  digitalWrite(relayPin,LOW); // Make sure device is off to begin with
  Serial.begin(115200);
   esp8266.begin(115200);
   delay(1500); //Give initialization time for sensor and wifi
  sendCommand("AT",5,"OK");
  sendCommand("AT+CWMODE=1",5,"O K");
  sendCommand("AT+CWJAP=\""+ AP +"\",\""+ PASS +"\"",20,"OK");
  

//For using OLED
/* if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // I2C Address 0x3D for 128x60 OLED, this allocates to the OLED
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
 display.display(); //OLED buffer
 
 display.clearDisplay(); */

  Serial.println("Adafruit PMSA003I Air Quality Sensor");

  if (!aqi.begin_I2C()) {      // connect to the sensor over I2C; Check library for software serial cmd
    Serial.println("Could not find PM 2.5 sensor!");
    while (1) delay(10);
  }
  Serial.println("PM25 found!");// For Debugging in Serial
  
  dht.begin();
}

void loop() {
  PM25_AQI_Data data;
  
  if (! aqi.read(&data)) {  // Test if data can be retreived from sensor
    Serial.println("Could not read from AQI");
    delay(500);  // try again in a bit!
    return;
  }
  Serial.println("AQI reading success"); // For Debugging in Serial

//For debugging the sensor in Serial
  Serial.println();
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Concentration Units (standard)"));
  Serial.println(F("---------------------------------------"));
  Serial.print(F("PM 1.0: ")); Serial.print(data.pm10_standard);
  Serial.print(F("\t\tPM 2.5: ")); Serial.print(data.pm25_standard);
  Serial.print(F("\t\tPM 10: ")); Serial.println(data.pm100_standard);
/*
  Serial.println(F("Concentration Units (environmental)"));
  Serial.println(F("---------------------------------------"));
  Serial.print(F("PM 1.0: ")); Serial.print(data.pm10_env);
  Serial.print(F("\t\tPM 2.5: ")); Serial.print(data.pm25_env);
  Serial.print(F("\t\tPM 10: ")); Serial.println(data.pm100_env);
  Serial.println(F("---------------------------------------"));
  Serial.print(F("Particles > 0.3um / 0.1L air:")); Serial.println(data.particles_03um);
  Serial.print(F("Particles > 0.5um / 0.1L air:")); Serial.println(data.particles_05um);
  Serial.print(F("Particles > 1.0um / 0.1L air:")); Serial.println(data.particles_10um);
  Serial.print(F("Particles > 2.5um / 0.1L air:")); Serial.println(data.particles_25um);
  Serial.print(F("Particles > 5.0um / 0.1L air:")); Serial.println(data.particles_50um);
  Serial.print(F("Particles > 10 um / 0.1L air:")); Serial.println(data.particles_100um);
  Serial.println(F("---------------------------------------"));
  */
//  //Printing to OLED
//  display.setTextSize(1);
//  display.setTextColor(WHITE);
//  display.setCursor(2, 10);
//  display.print(F("PM 1.0: ")); display.println(data.pm10_standard);
//  display.setCursor(2, 20);
//  display.print(F("PM 2.5: ")); display.println(data.pm25_standard);
//  display.setCursor(2, 30);
//  display.print(F("PM 10: ")); display.println(data.pm100_standard);
//  display.setCursor(2, 40);
  
  if (data.pm10_standard >=30 || data.pm25_standard >=10 || data.pm100_standard >= 100){
    digitalWrite(relayPin,HIGH);
    if (relayState != 1){
      tone(piezoPin, 2500, 1500);
    }
//  display.print(F("Fan Status: ON"));
  relayState = 1;
  }
  else if(relayState == 1 && (data.pm10_standard <=30 || data.pm25_standard <=20 || data.pm100_standard <= 100)){
//          display.print(F("Fan Status: COOLDOWN")); 
//          display.display();
          tone(piezoPin, 3500, 250);
          delay(500);
          tone(piezoPin, 3500, 250);          
          delay(10000);
//          display.print(F("Fan Status: OFF"));
//          display.display();
            aqi.read(&data);
            delay(2500);
            Serial.print(F("PM 1.0: ")); Serial.print(data.pm10_standard);
            Serial.print(F("\t\tPM 2.5: ")); Serial.print(data.pm25_standard);
            Serial.print(F("\t\tPM 10: ")); Serial.println(data.pm100_standard);
            delay(500);
          if (data.pm10_standard <=5 || data.pm25_standard <=5 || data.pm100_standard <= 5){
          relayState = 0;
          tone(piezoPin, 3500, 1000);
          digitalWrite(relayPin,LOW);
  }   
  }
  else{
    digitalWrite(relayPin,LOW);
//  display.print(F("Fan Status: OFF"));
 }

 
//  display.display();
delay(5000);
//display.clearDisplay();


 String getData = "GET /update?api_key="+ API +"&field1="+data.pm10_standard+"&field2="+data.pm25_standard
 +"&field3="+data.pm100_standard+"&field4="+getTemperatureValue()+"&field5="+getHumidityValue()+"&field6="+relayState;
 sendCommand("AT+CIPMUX=1",5,"OK");
 sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,15,"OK");
 sendCommand("AT+CIPSEND=0," +String(getData.length()+4),4,">");
 esp8266.println(getData);delay(1500);countTrueCommand++;
 sendCommand("AT+CIPCLOSE=0",5,"OK");


}


String getTemperatureValue(){ //Temp Value

   float f = dht.readTemperature(true); // (true) for farenheit
   Serial.print(" Temperature(F)= ");
   
   Serial.println(f); 
   delay(50); //had issues with DHT11 sending signal until pulling it like this
   return String(f); 
  
}

String getHumidityValue(){ // Humidity Value

   float h = dht.readHumidity();
   Serial.print(" Humidity in %= ");
   
   Serial.println(h);
   delay(50);
   return String(h); 
  
}

void sendCommand(String command, int maxTime, char readReplay[]) {
  Serial.print(countTrueCommand);
  Serial.print(". at command => ");
  Serial.print(command);
  Serial.print(" ");
  while(countTimeCommand < (maxTime*1))
  {
    esp8266.println(command);//at+cipsend
    if(esp8266.find(readReplay))//ok
    {
      found = true;
      break;
    }
  
    countTimeCommand++;
  }
  
  if(found == true)
  {
    Serial.println("PASS");
    countTrueCommand++;
    countTimeCommand = 0;
  }
  
  if(found == false)
  {
    Serial.println("FAIL");
    countTrueCommand = 0;
    countTimeCommand = 0;
  }
  
  found = false;
 }
