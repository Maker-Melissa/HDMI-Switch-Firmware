/* ---------- DO NOT EDIT ANYTHING IN THIS FILE UNLESS YOU KNOW WHAT YOU ARE DOING---------- */
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <Servo.h> 
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <MQTTClient.h>           //https://github.com/256dpi/arduino-mqtt
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/* HDMI Pin Settings */
#define HDMI1_PIN 16
#define HDMI2_PIN 14
#define HDMI3_PIN 12
#define HDMI4_PIN 13
#define BUTTON_PIN 3
#define POWER_PIN 5

//define your default values here, if there are different values in config.json, they are overwritten.
char host[34];
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_topic[50];
char mqtt_isAuthentication[7] = "FALSE";
char mqtt_username[40];
char mqtt_password[40];
char update_username[40];
char update_password[40];
char update_path[34] = "/firmware";

//Unique device ID 
const char* mqttDeviceID;

//Form Custom SSID
String ssidAP = "HDMISwitch" + String(ESP.getChipId());

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () 
{
  shouldSaveConfig = true;
}

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void connect();

void setup() 
{
  pinMode(BUTTON_PIN, OUTPUT);
  digitalWrite(BUTTON_PIN, HIGH);

  pinMode(HDMI1_PIN, INPUT);
  pinMode(HDMI2_PIN, INPUT);
  pinMode(HDMI3_PIN, INPUT);
  pinMode(HDMI4_PIN, INPUT);
  pinMode(POWER_PIN, INPUT);

  if (SPIFFS.begin()) 
  {
    if (SPIFFS.exists("/config.json")) 
    {
      //file exists, reading and loading
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) 
      {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) 
        {
          strcpy(host, json["host"]);
          strcpy(update_username, json["update_username"]);
          strcpy(update_password, json["update_password"]);
          strcpy(mqtt_isAuthentication, json["mqtt_isAuthentication"]);
          strcpy(mqtt_username, json["mqtt_username"]);
          strcpy(mqtt_password, json["mqtt_password"]);
          strcpy(update_path, json["update_path"]);
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);
        } 
        else 
        {
        }
      }
    }
  } 
  else 
  {
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_text0("<p>Select your wifi network and type in your password, if you do not see your wifi then scroll down to the bottom and press scan to check again.</p>");
  WiFiManagerParameter custom_text1("<h1>Hostname/MQTT ID</h1>");
  WiFiManagerParameter custom_text2("<p>Enter a name for this device which will be used for the hostname on your network and identify the device from MQTT.</p>");
  WiFiManagerParameter custom_host("name", "Device Name", host, 32);
  
  WiFiManagerParameter custom_text3("<h1>MQTT</h1>");
  WiFiManagerParameter custom_text4("<p>Enter the details of your MQTT server and then enter the topic for which the device listens to MQTT commands from. If your server requires authentication then set it to True and enter your server credentials otherwise leave it at false and keep the fields blank.</p>");
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server IP", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Server Port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_topic("topic", "MQTT Topic", mqtt_topic, 50);
  WiFiManagerParameter custom_mqtt_isAuthentication("isAuthentication", "MQTT Authentication?", mqtt_isAuthentication, 7);
  WiFiManagerParameter custom_mqtt_username("userMQTT", "Username For MQTT Account", mqtt_username, 40);
  WiFiManagerParameter custom_mqtt_password("passwordMQTT", "Password For MQTT Account", mqtt_password, 40);

  WiFiManagerParameter custom_text5("<h1>Web Updater</h1>");
  WiFiManagerParameter custom_text6("<p>The web updater allows you to update the firmware of the device via a web browser by going to its ip address or hostname /firmware ex. 192.168.0.5/firmware you can change the update path below. The update page is protected so enter a username and password you would like to use to access it. </p>");
  WiFiManagerParameter custom_update_username("user", "Username For Web Updater", update_username, 40);
  WiFiManagerParameter custom_update_password("password", "Password For Web Updater", update_password, 40);
  WiFiManagerParameter custom_device_path("path", "Updater Path", update_path, 32);
  WiFiManagerParameter custom_text8("<p>*To reset device settings restart the device and quickly place the jumper on the board, wait 10 seconds and remove the jumper again.*</p>");
  WiFiManagerParameter custom_text9("");

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.setCustomHeadElement("<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} input#passwordMQTT{-webkit-text-security:disc !important;} input#password{-webkit-text-security:disc !important;} body{text-align: center;font-family:oswald;} button{border:0;background-color:#313131;color:white;line-height:2.4rem;font-size:1.2rem;text-transform: uppercase;width:100%;font-family:oswald;} .q{float: right;width: 65px;text-align: right;} body{background-color: #575757;}h1 {color: white; font-family: oswald;}p {color: white; font-family: open+sans;}a {color: #78C5EF; text-align: center;line-height:2.4rem;font-size:1.2rem;font-family:oswald;}</style>");
  wifiManager.addParameter(&custom_text0);
  wifiManager.addParameter(&custom_text1);
  wifiManager.addParameter(&custom_text2);
  wifiManager.addParameter(&custom_host);

  wifiManager.addParameter(&custom_text3);
  wifiManager.addParameter(&custom_text4);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.addParameter(&custom_mqtt_isAuthentication);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);

  wifiManager.addParameter(&custom_text5);
  wifiManager.addParameter(&custom_text6);
  wifiManager.addParameter(&custom_update_username);
  wifiManager.addParameter(&custom_update_password);
  wifiManager.addParameter(&custom_device_path);
  wifiManager.addParameter(&custom_text8);
  
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(ssidAP.c_str())) 
  {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //read updated parameters
  strcpy(host, custom_host.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
  strcpy(mqtt_isAuthentication, custom_mqtt_isAuthentication.getValue());
  strcpy(mqtt_username, custom_mqtt_username.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());
  strcpy(update_username, custom_update_username.getValue());
  strcpy(update_password, custom_update_password.getValue());
  strcpy(update_path, custom_device_path.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) 
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["host"] = host;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;
    json["mqtt_isAuthentication"] = mqtt_isAuthentication;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;
    json["update_username"] = update_username;
    json["update_password"] = update_password;
    json["update_path"] = update_path;
 
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) 
    {
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  delay(5000);
  
  if(digitalRead(0) == LOW || String(host).length() == 0 || String(mqtt_server).length() == 0 || String(mqtt_topic).length() == 0 || String(update_username).length() == 0 || String(update_password).length() == 0)
  {
    wifiManager.resetSettings();
    ESP.reset();
  }
  
  mqttDeviceID = host;
  client.begin(mqtt_server, atoi(mqtt_port), net);
  client.onMessage(messageReceived);

  connect();

  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

  httpServer.on("/", [](){
    if(!httpServer.authenticate(update_username, update_password))
      return httpServer.requestAuthentication();
    httpServer.send(200, "text/plain", "Hostname: " + String(host) + "\nMQTT Server: " + String(mqtt_server) + "\nMQTT Port: " + String(mqtt_port) + "\nMQTT Authentication: " + String(mqtt_isAuthentication) + "\nMQTT Command Topic: " + String(mqtt_topic) + "\nMQTT Status Topic: " + String(mqtt_topic) + "/state" + "\nTo update firmware go to: http://"+ String(host) + ".local" + String(update_path) + "\n*To reset device settings restart the device and quickly place the jumper on the board, wait 10 seconds and remove the jumper again.*");
  });
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);

}

void connect() 
{
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
  }

  //If authentication true then connect with username and password
  if(String(mqtt_isAuthentication).equalsIgnoreCase("TRUE"))
  {
    while (!client.connect(mqttDeviceID, mqtt_username, mqtt_password)) 
    {
      delay(1000);
    }
  }
  else
  {
    while (!client.connect(mqttDeviceID)) 
    {
      delay(1000);
    }
  }

  client.subscribe(mqtt_topic);
  //client.publish(String(mqtt_topic) + "/info", String("IP Address: " + WiFi.localIP()));
  client.publish(String(mqtt_topic) + "/state", String(getCurrentPort()));
}

void loop() 
{
  client.loop();
  delay(10);

  if(!client.connected()) 
  {
    //connect();
    client.disconnect();
    connect();
  }

  httpServer.handleClient();
}

void messageReceived(String &topic, String &payload) 
{
  String msgString = payload;
  if (msgString == "1") {
    selectPort(1);
  } else if (msgString == "2") {
    selectPort(2);
  } else if (msgString == "3") {
    selectPort(3);
  } else if (msgString == "4") {
    selectPort(4);
  } else if (msgString == "100") {
    cyclePorts();
  } else if (msgString == "NEXT") {
    incrementPort();
  } else if (msgString == "PREVIOUS") {
    decrementPort();
  } else if (msgString == "ON") {
    turnOn();
  } else if (msgString == "OFF") {
    turnOff();
  } else if (msgString == "POWER") {
    powerToggle();
  }

  client.publish(String(mqtt_topic) + "/state", String(getCurrentPort()));
}

int getCurrentPort()
{
  // Check to see which input is low
  // Return the one that's low
  
  if(digitalRead(HDMI1_PIN) == LOW) {
    return 1;
  }
  if(digitalRead(HDMI2_PIN) == LOW) {
    return 2;
  }
  if(digitalRead(HDMI3_PIN) == LOW) {
    return 3;
  }
  if(digitalRead(HDMI4_PIN) == LOW) {
    return 4;
  }
}

bool isPowerOn()
{
  if(digitalRead(POWER_PIN) == HIGH) {
    return true;
  }

  return false;
}

void incrementPort()
{
  // set buttonPin Low
  digitalWrite(BUTTON_PIN, LOW);
  // wait a bit
  delay(50);
  // set buttonPin High
  digitalWrite(BUTTON_PIN, HIGH);
  // wait a bit more
  delay(250);
}

void decrementPort()
{
  // Increment 3 times
  for (int i = 0; i < 3; i++) {
    incrementPort();
  }
}

void cyclePorts()
{
  // Increment 4 times
  for (int i = 0; i < 4; i++) {
    incrementPort();
  }
}

void buttonShortHold()
{
  // set buttonPin Low
  digitalWrite(BUTTON_PIN, LOW);
  // wait a half second
  delay(500);
  // set buttonPin High
  digitalWrite(BUTTON_PIN, HIGH);
  delay(250);
}

void buttonLongHold()
{
  // set buttonPin Low
  digitalWrite(BUTTON_PIN, LOW);
  // wait over 1 second
  delay(1500);
  // set buttonPin High
  digitalWrite(BUTTON_PIN, HIGH);
  delay(250);
}

void powerToggle()
{
  if (isPowerOn()) {
    turnOff();
  } else {
    turnOn();
  }
}

void turnOn()
{
  if (isPowerOn()) return;
  buttonShortHold();
}

void turnOff()
{
  if (!isPowerOn()) return;
  buttonLongHold();
}

void selectPort(int port)
{
  // Make sure it's on
  turnOn();

  // Get the current port and figure out how many to advance
  int steps = port - getCurrentPort();
  if (steps < 0) {
    steps += 4;
  }

  // increment that number
  for (int i = 0; i < steps; i++) {
    incrementPort();
  }
}