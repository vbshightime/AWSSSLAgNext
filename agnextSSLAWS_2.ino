
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "cert.h"
#include <time.h>
#include <EEPROM.h>
#include "AgNextCaptive.h" 
#include <ESP8266WebServer.h>
//#include <ArduinoJson.h> 
//#include <NTPClient.h>
//#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#define THINGNAME "AgnextIoT"
#define MESSAGE_MAX_LEN 256

const int MQTT_PORT = 8883;
const char MQTT_SUB_TOPIC[]  = "$aws/things/AgnextIoT/shadow/update";
const char MQTT_PUB_TOPIC[] = "$aws/things/AgnextIoT/shadow/update";
const char MQTT_DEVICE_PUB_TOPIC[] = "device/compass/deviceID";
const char MQTT_DEVICE_SUB_TOPIC[] = "$aws/things/AgnextIoT/shadow/update";

const char *HOST_ID = "a30uivx7gx8wse-ats.iot.us-east-1.amazonaws.com";
int PORT = 8883;
const char MESSAGE_BODY[] = "{\"temp\":%.d, \"batteryLevel\":%.d}";
int counter = 0;
uint8_t DST = 1;


//************** Auxillary functions******************//
ESP8266WebServer server(80);
//**********softAPconfig Timer*************//
unsigned long APTimer = 0;
unsigned long APInterval = 60000;

//*********SSID and Pass for AP**************//
const char* ssidAPConfig = "adminesp55";
const char* passAPConfig = "adminesp55";

//**********check for connection*************//
bool isConnected = true;
bool isAPConnected = true;

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);


//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP,"pool.ntp.org");

WiFiClientSecure clientWiFi;


BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);

PubSubClient clientPub(clientWiFi);

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

unsigned long lastMillis = 0;
time_t now;
time_t nowish = 1510592825;

void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, DST * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial);
  delay(100);
  EEPROM.begin(512); 
  handleWebForm();     
  WiFi.disconnect(true);
  delay(100);  
  reconnectWiFi();
  delay(10);
  NTPConnect();
  clientWiFi.setTrustAnchors(&cert);
  clientWiFi.setClientRSACert(&client_crt, &key);
  clientPub.setServer(HOST_ID,PORT);
  clientPub.setCallback(callback);
  //timeClient.begin();
  //  while(!timeClient.update()){
  //    timeClient.forceUpdate();
  //  }
  //  clientWiFi.setX509Time(timeClient.getEpochTime());
  reconnectMQTT(false);
}


void loop() {
  // put your main code here, to run repeatedly:
  if(!clientPub.connected()){
      reconnectMQTT(false);
    }
  int tempValue = random(0,30);
  int percent = random(20,100);
  char messagePayload[MESSAGE_MAX_LEN];
  snprintf(messagePayload,MESSAGE_MAX_LEN,MESSAGE_BODY,tempValue,percent);
  Serial.println(clientPub.publish(MQTT_PUB_TOPIC,messagePayload) ? "published" : "Error in Publishing");
  Serial.println(clientPub.publish(MQTT_DEVICE_PUB_TOPIC,messagePayload) ? "published" : "Error in Publishing");
  Serial.println(clientPub.state());
}


//****************************Connect to WiFi****************************//
void reconnectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
        String string_Ssid="";
        String string_Password="";
        string_Ssid= read_string(30,0); 
        string_Password= read_string(30,50);        
        Serial.println("ssid: "+ string_Ssid);
        Serial.println("Password: "+string_Password);               
  delay(400);
  WiFi.begin(string_Ssid.c_str(),string_Password.c_str());
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {   
      delay(500);
      Serial.print(".");
      if(counter == 20){
          ESP.restart();
        }
        counter++;
  }
  Serial.print("Connected to:\t");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT(bool blocking){
      while(!clientPub.connected()){
          Serial.println("Attempting MQTT connection");
          if(clientPub.connect(THINGNAME)){
              Serial.println("connected");
              Serial.println(clientPub.subscribe(MQTT_SUB_TOPIC) ? "Subscribed":"Subscribed False");
            }else{
                  if(!blocking){
                       Serial.print("failed, rc=");
                       Serial.print(clientPub.state());
                       Serial.println(" try again in 5 seconds");
                       // Wait 5 seconds before retrying
                       delay(5000);  
                    }
                    if(blocking){
                       break;
                      }
             }
       }
}


  void handleDHCP(){
  if(server.args()>0){
       for(int i=0; i<=server.args();i++){
          Serial.println(String(server.argName(i))+'\t' + String(server.arg(i)));
        }
       if(server.hasArg("ssid")&&server.hasArg("passkey")){
          /*for (int i = 0 ; i < EEPROM.length() ; i++) {
            EEPROM.write(i, 0);
          }*/
           ROMwrite(String(server.arg("ssid")),String(server.arg("passkey")),String(server.arg("device")),String(server.arg("sensor_list")));
           isConnected =false;
        }    
    }else{
         String webString = FPSTR(HTTPHEAD);
         webString+= FPSTR(HTTPBODYSTYLE);
         webString+= FPSTR(HTTPBODY);
         webString+= FPSTR(HTTPCONTENTSTYLE);
         webString+= FPSTR(HTTPDEVICE);
         String device = String(read_string(20,100));
         webString.replace("{s}",device);
         webString+= FPSTR(HTTPFORM);
         webString+= FPSTR(HTTPLABLE1);
         webString+= FPSTR(HTTPLABLE2);
         webString+= FPSTR(HTTPSUBMIT);
         webString+= FPSTR(HTTPCLOSE);
         //File file = SPIFFS.open("/AgNextCaptive.html", "r");
         //server.streamFile(file,"text/html");
         server.send(200,"text/html",webString);
         //file.close();
      }
  }


//****************HANDLE NOT FOUND*********************//
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  server.send(404, "text/plain", message);
}


void handleWebForm(){ 
      WiFi.mode(WIFI_AP);
      delay(100);
      WiFi.softAPConfig(apIP, apIP, netMsk);
      Serial.println(WiFi.softAP(ssidAPConfig,passAPConfig) ? "Configuring softAP" : "kya yaar not connected");    
      delay(100);
      Serial.println(WiFi.softAPIP());
      server.begin();
      server.on("/", handleDHCP); 
      server.onNotFound(handleNotFound);
      APTimer = millis();
      delay(5000);
      if(WiFi.softAPgetStationNum()>0){
      while(isConnected && millis()-APTimer<= APInterval) {
       server.handleClient();  
        }ESP.restart();
        reconnectWiFi();
      }
}

//----------Write to ROM-----------//
void ROMwrite(String s, String p, String id,String delays){
 s+=";";
 write_EEPROM(s,0);
 p+=";";
 write_EEPROM(p,50);
 EEPROM.commit();   
}


//***********Write to ROM**********//
void write_EEPROM(String x,int pos){
  for(int n=pos;n<x.length()+pos;n++){
  //write the ssid and password fetched from webpage to EEPROM
   EEPROM.write(n,x[n-pos]);
  }
}

  
//****************************EEPROM Read****************************//
String read_string(int l, int p){
  String temp;
  for (int n = p; n < l+p; ++n)
    {
   // read the saved password from EEPROM
     if(char(EEPROM.read(n))!=';'){
     
       temp += String(char(EEPROM.read(n)));
     }else n=l+p;
    }
  return temp;
}

