#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//this is where I hid my Api key and wifi credentials, so you can't see it :)
//you could comment this line and use the hard coded values below
#include "config.h"

/************************* Input/Output *********************************/
#define RELAY_UP 3
#define RELAY_DOWN 0
#define IN_UP 1
#define IN_DOWN 2

/************************* WiFi Access Point *********************************/

// #define WLAN_SSID       "ssid"
// #define WLAN_PASS       "password"

/************************* Adafruit.io Setup *********************************/

// #define AIO_SERVER      "io.adafruit.com"
// #define AIO_SERVERPORT  1883                   // use 8883 for SSL
// #define AIO_USERNAME    "username"
// #define AIO_KEY         "api_key..."

/************************* Variables *********************************/

//time from fully up to fully down, in ms
unsigned int fullTransitionTime = 27000; 
//where we want the shutter to be [in %], 0 means fully up, 100 means fully down
int targetPosition = 0;
//on boot we assume it's fully up
int currentPosition = 0;

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe updownshutter = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/updown");

Adafruit_MQTT_Publish current = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/current");

enum State
{
  IDLE,
  MOVING_UP,
  MOVING_DOWN
};

/************************* Functions declaration *********************************/
void MQTT_connect();
void MQTT_readSubs();
void shutterSetState(bool dir);
void checkInputs();
void manageTransitions();


void setup() {
  // Serial.begin(115200);
  // delay(10);

  pinMode(RELAY_DOWN,OUTPUT);
  pinMode(RELAY_UP,OUTPUT);
  pinMode(IN_UP,INPUT_PULLUP);
  pinMode(IN_DOWN,INPUT_PULLUP);

  // Connect to WiFi access point.
  // Serial.println(); Serial.println();
  // Serial.print("Connecting to ");
  // Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }
  // Serial.println();

  // Serial.println("WiFi connected");
  // Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&updownshutter);
}

void loop() {
  MQTT_connect();
  MQTT_readSubs();
  checkInputs();
  manageTransitions();
}


void MQTT_readSubs()
{
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(0))) {
    if (subscription == &updownshutter) {
      if(!strcmp((char*)updownshutter.lastread,"up"))
      {
        targetPosition = 0;
      }
      else if(!strcmp((char*)updownshutter.lastread,"down"))
      {
        targetPosition = 100;
      }
      else if(!strcmp((char*)updownshutter.lastread,"stop"))
      {
        targetPosition = currentPosition;
      }
    }

  }
}

//set movement state of the shutter, returns true if the state is other than the previous one 
bool shutterSetState(State state)
{
  static State lastState = State::IDLE;
  if(state==State::IDLE)
  {
    digitalWrite(RELAY_UP, LOW);
    digitalWrite(RELAY_DOWN, LOW);
  }
  else if(state==State::MOVING_UP)
  {
    digitalWrite(RELAY_UP, HIGH);
    digitalWrite(RELAY_DOWN, LOW);
  }
  else if(state==State::MOVING_DOWN)
  {
    digitalWrite(RELAY_DOWN, HIGH);
    digitalWrite(RELAY_UP, LOW);
  }
  bool stateChanged = lastState!=state;
  lastState = state;
  return stateChanged;
}

//chceck if input switches are pressed
void checkInputs()
{
  bool upPressed, downPressed;

  upPressed = !digitalRead(IN_UP);
  downPressed = !digitalRead(IN_DOWN);

  if(upPressed && !downPressed)
  {
    targetPosition = currentPosition-1;

    //if the program "thinks" the limit is obtained, but the user still presses/holds the switch,
    //it means the limit is not actually obtained and we should calibrate it allowing the user to switch/keep on the relay
    if(currentPosition==0)
    {
      currentPosition=1;
    }

  }
  if(!upPressed && downPressed)
  {
    targetPosition = currentPosition+1; 

    if(currentPosition==100)
    {
      currentPosition=99;
    }
  }
  targetPosition = constrain(targetPosition, 0,100);
}

void manageTransitions()
{
  static unsigned long lastT=0;
  int error = targetPosition - currentPosition;
  if(error>0)
  {
    //move down
    //if the state has changed, start measuring time
    if(shutterSetState(State::MOVING_DOWN))lastT = millis();
  }
  else if(error<0)
  {
    //move up
    if(shutterSetState(State::MOVING_UP))lastT = millis();
  }
  else
  {
    //stop
    if(shutterSetState(State::IDLE))current.publish(currentPosition);
  }

  //if the movement lasts for more than 1/100 of the fullTransition,
  //state that the current position has changed by 1
  if(millis()-lastT>=(fullTransitionTime/100U))
  {
      if(error>0)currentPosition++;
      else if(error<0)currentPosition--;
      lastT=millis();
  }

  currentPosition = constrain(currentPosition,0,100);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  //Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
      //  Serial.println(mqtt.connectErrorString(ret));
      //  Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  // Serial.println("MQTT Connected!");
}