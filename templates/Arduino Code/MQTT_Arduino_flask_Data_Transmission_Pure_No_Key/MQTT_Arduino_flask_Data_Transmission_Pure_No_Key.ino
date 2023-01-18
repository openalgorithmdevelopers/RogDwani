/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

/*
 * Send comma separated data only over MQTT without any key and starting or ending character
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "FS.h"
// Update these with values suitable for your network.

const char* ssid = "mine";
const char* password = "9560033900";
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

// Control message byte from flask/python
//# 0000 00 00   #0 # do nothing or future use
//# 0000 01 01   #5  # default short duration and low quality
//# 0000 01 10   #6  # short duration and medium quality
//# 0000 01 11   #7 # short duration and high quality
//# 0000 10 01   #9 # medium duration and low quality
//# 0000 10 10   #10 # medium duration and medium quality
//# 0000 10 11   #11  # medium duration and high quality
//# 0000 11 01   #13 # long duration and low quality
//# 0000 11 10   #14 # long duration and medium quality
//# 0000 11 11   #15  # long duration and high quality  

int SIGNAL_DURATION_MEDIUM = 1000;  //medium duration is considered as 1 seconds
int SIGNAL_DURATION_SHORT = 100;  //short duration is considered as 0.1 seconds
int SIGNAL_DURATION_LONG = 5000;  //long duration is considered as 5 seconds

//Higher the signal quality, lesser the delay in sampling
int SIGNAL_QUALITY_LOW = 2;    //delay in milliseconds, corresponds to 500Hz
int SIGNAL_QUALITY_MEDIUM = 1;  //delay in milliseconds, corresponds to 1KHz
float SIGNAL_QUALITY_HIGH = 0.5;  //delay in milliseconds, corresponds to 2KHz

bool REPEAT_CAPTURE_AND_SEND = false;
int currentSignalDuration = SIGNAL_DURATION_SHORT;
float currentSignalQuality = SIGNAL_QUALITY_MEDIUM;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
//  Serial.print("Message arrived [");
//  Serial.print(topic);
//  Serial.print("] ");
  String signalParams = "";
  for (int i = 0; i < length; i++) {
//    Serial.print((char)payload[i]);
      signalParams += (char)payload[i];
  }
//  Serial.println();
  setSignalParams(signalParams);
  REPEAT_CAPTURE_AND_SEND = true;
}

void setSignalParams(String signalParams)
{
  int params = signalParams.toInt();
//  Serial.println("param recieved as = ");
//  Serial.print(params);
  
  if(params == 5)
  {
    currentSignalDuration = SIGNAL_DURATION_SHORT;
    currentSignalQuality = SIGNAL_QUALITY_LOW;
  }
  else if(params == 6)
  {
    currentSignalDuration = SIGNAL_DURATION_SHORT;
    currentSignalQuality = SIGNAL_QUALITY_MEDIUM;
  }
  else if(params == 7)
  {
    currentSignalDuration = SIGNAL_DURATION_SHORT;
    currentSignalQuality = SIGNAL_QUALITY_HIGH;
  }
  else if(params == 9)
  {
    currentSignalDuration = SIGNAL_DURATION_MEDIUM;
    currentSignalQuality = SIGNAL_QUALITY_LOW;
  }
  else if(params == 10)
  {
    currentSignalDuration = SIGNAL_DURATION_MEDIUM;
    currentSignalQuality = SIGNAL_QUALITY_MEDIUM;
  }
  else if(params == 11)
  {
    currentSignalDuration = SIGNAL_DURATION_MEDIUM;
    currentSignalQuality = SIGNAL_QUALITY_HIGH;
  }
  else if(params == 13)
  {
    currentSignalDuration = SIGNAL_DURATION_LONG;
    currentSignalQuality = SIGNAL_QUALITY_LOW;
  }
  else if(params == 14)
  {
    currentSignalDuration = SIGNAL_DURATION_LONG;
    currentSignalQuality = SIGNAL_QUALITY_MEDIUM;
  }
  else if(params == 15)
  {
    currentSignalDuration = SIGNAL_DURATION_LONG;
    currentSignalQuality = SIGNAL_QUALITY_HIGH;
  }
//  Serial.println("Current duration and quality set to ");
//  Serial.println(currentSignalDuration);
//  Serial.println(currentSignalQuality);
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
      if (client.connect(clientId.c_str()), "tornado", "") {
      Serial.println("connected");
      // Once connected, publish an announcement...
//      client.publish("/DS/Arduino", "Digital Stethoscope sending.....");
      client.subscribe("/DS/flask");        
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  
    
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void loop() {
  if(REPEAT_CAPTURE_AND_SEND)
  {
    Serial.println("starting to capture and send");
    capture_and_write_signal();
    Send_File_MQTT();
    REPEAT_CAPTURE_AND_SEND = false;
    Serial.println("Done with the task");
  }
  
  client.loop();
  client.subscribe("/DS/flask");
//  client.subscribe("topi/c");
  //Send_File_MQTT();
}

void capture_and_write_signal(){
  String DS_signal = "";
  File file;
   
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
 
  file = SPIFFS.open("/stethoscope_signal.txt", "w");
 
  if (!file) {
    Serial.println("There was an error opening the file for writing");
    return;
  }

  int dataPoints = currentSignalDuration/currentSignalQuality;
  int loopCounter = 0;
  while(loopCounter < dataPoints){
    loopCounter += 1;  

    int sample = analogRead(A0);
//    Serial.print("Loop counter = ");
//    Serial.println(loopCounter);
    DS_signal += String(sample) + ",";
      
    delay(currentSignalQuality); 
  }  

  if (file.print(DS_signal)) {
      ;
//    Serial.println("File was written with ");
//    Serial.println(DS_signal);
  } 
  else {
    Serial.println("File write failed");
  }
  file.close();
//  Serial.println("Signal recording on the file completed..");
}

void Send_File_MQTT(){
  File file;
  
  file = SPIFFS.open("/stethoscope_signal.txt", "r"); 

  if (!file) {
    Serial.println("There was an error reading the content of the file");
    return;
  } 
  
//  Serial.println("reading");
  while(file.available()){
      int counter = 0;

      while(counter < MSG_BUFFER_SIZE - 1){
        if(file.available()){
          int ch = file.read();
            msg[counter] = ch;
        }
        counter += 1;
      }
      client.publish("/DS/Arduino", msg, 1);
//      Serial.println("Published: ");
//      Serial.println(msg);
      delay(100); // 200 delay is working fine without any data loss
  }
//  Serial.println("reading and publishing ends");
//  Serial.println(file.size());
  file.close();
}
