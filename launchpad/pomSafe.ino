#include <Wire.h>
#include "tmp006.h"
#define USE_USCI_B1
#include <BMA222.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

BMA222 mySensor;
tmp006 tmp006;

WiFiClient http;
//char buf[1025];
//int i;
//size_t total = 0;

int accXp;
int accYp;
int accZp;

int tempPrev;
int openPrev;

int slamValPrev;
int slamValCur;

const char ssid[] = "Intel Mashery 2.4GHz";
const char wifipw[] = "hackandmash";

void setup()
{
  //Start Serial at 115200 baud
  Serial.begin(115200);
  //Initialize the Temperature Sensor
  tmp006.begin(EIGHT_SAMPLES);  // Takes 8 averaged samples for measurement
  //Initilaize the Accelerometer
  mySensor.begin();
  uint8_t chipID = mySensor.chipID();
  //Initilaize the WiFi
  uint32_t msave;
  Serial.println("WiFi:");
  WiFi.begin((char *)ssid, (char *)wifipw);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(50);
  }
  Serial.println();
  Serial.println("Connected- waiting for IP");
  msave = millis();
  while (WiFi.localIP() == INADDR_NONE) {
    Serial.print('.');
    delay(100);
    if (millis()-msave > 5000) {
      Serial.println("Timeout waiting for IP; disconnecting & reconnecting.");
      WiFi.disconnect();
      WiFi.begin((char*)ssid);
      msave = millis();
    }
  }
  Serial.print(" done; IP=");
  IPAddress my_ip = WiFi.localIP();
  Serial.println(my_ip);
  //Setup the starting position
  slamValPrev = mySensor.readXData();
  accXp = mySensor.readXData();
  accYp = mySensor.readYData();
  accZp = mySensor.readZData();
  Serial.println("Initial Values");
  Serial.println("X: " + String(accXp) + "\tY: " + String(accYp) + "\tZ: " + String(accZp));  
  delay(1000);
}





void loop()
{
  unsigned long time = millis();
  //Read all of the data
  checkSlam();
  int temp = tmp006.getTemp();
  int accX = mySensor.readXData();
  int accY = mySensor.readYData();
  int accZ = mySensor.readZData();
  int open = 0;
  int threshold = 10;
  //Work on the data
  //Open
  Serial.println(abs(abs(accX)-abs(accXp)));
  Serial.println(abs(abs(accY)-abs(accYp)));
  Serial.println(abs(abs(accZ)-abs(accZp)));
  if (abs(abs(accX)-abs(accXp))>threshold || abs(abs(accY)-abs(accYp))>threshold ||abs(abs(accZ)-abs(accZp))>threshold)
  {
      open = 1;
  }
  //Send all of the Data 
  checkSlam();
  if(temp > tempPrev + 3 || temp < tempPrev - 3)
  {
    String payload1 = "{ \"temp\" : " + String(temp) + " }";
    tempPrev=temp; 
    sslSender(payload1);  
  }
  if(open != openPrev)
  {
    String payload3 = "{ \"open\" : " + String(open) + " }";
    openPrev=open;
    sslSender(payload3);
  }
  delay(200);
  Serial.println("Time to run: " + String(time));
  checkSlam();
}

void sslSender(String payload)
{
  Serial.println("The payload is: " + payload);
  int payloadSize = payload.length( );  

  Serial.println("Connecting to Firebase using SSL");
  if (http.sslConnect("pomsafe.firebaseio.com", 443)) 
  {
    Serial.println("Sending HTTPS traffic to firebase");
    //Make a HTTP Header
    http.println("PATCH /users/a/room.json  HTTP/1.1");
    http.println("Host: pomsafe.firebaseio.com");
    http.print("Content-Length: ");
    http.println(payloadSize);
    http.println();
    //JSON Data to be Entered at that address
    http.println(payload);
    http.println();
    delay(250); 
    Serial.println("Sent Payload, closing socket.");
    http.stop();
    delay(250);
  }  
}

void setSlam()
{
  sslSender("{ \"slam\" : 1 }");
  delay(150);
  sslSender("{ \"slam\" :  0 }");
}

void checkSlam()
{
  slamValPrev=slamValCur;
  slamValCur = mySensor.readXData();
  if(abs(abs(slamValCur)-abs(slamValPrev))>60){ setSlam();} 
}

