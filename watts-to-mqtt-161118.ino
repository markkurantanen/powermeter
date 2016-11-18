/*
  Power Meter Led counter
  by markku rantanen markku@kolumbus.fi
  Using TSL257 as photodiode connected to PIN 0
  Reading pulses triggered by attachinterrupt
*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define writepin 2
#define readpin 0
  
const char* ssid = "*******";
const char* password = "******";

const char* mqtt_server = "my.***.com";
const char* outTopic = "put/some/topic/here"; // this one tells to mqtt which sensor is up and connected
const char* powerTopic = "put/some/topic/meter-number"; // this one is used to report the pulses
const char* inTopic = "intopic/topic"; // this is for callback, for instance, reset the ESP etc

float sleeptime=20000; // 20 seconds sleeptime, not really sleeping...
float sleepseconds=0;

unsigned long sendPeriod = 20000; // 20 second interval for updating pulses to mqtt
long lastTime = 0;


// These next 5 lines needed for "blink"
volatile unsigned long counter = 0;
volatile unsigned long lastcounter = 0;
volatile unsigned long diffcounter = 0;
//volatile unsigned long pulses = 0;
volatile unsigned long  pulses = 0;

volatile unsigned long kwh = 0;
volatile unsigned long lastImpulse = 0;
volatile unsigned long diffImpulse = 0;
volatile long currentWatt = 0;
boolean inTrace = false;


WiFiClient power;
PubSubClient client(power);  //name of the client, used to check in function if connected or not

void setup() {
  // put your setup code here, to run once:

pinMode(0, INPUT);
  
   // You can open the Arduino IDE Serial Monitor window to see what the code is doing
  Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable
  
  // setup wifi
  setup_wifi();
  client.setServer(mqtt_server, 1883);        // connects to mqtt broker
  
  // Attach an interrupt handler on PIN 0
 attachInterrupt(2, blink, FALLING);
 // attachInterrupt(0, pinChanged, CHANGE);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("POWERMeter reader");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

// below topic would receive "command 1" then reset counter, should happen at midnight
//
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(0, HIGH);   // Turn the LED on and off to see it works and listens. Another is espreset to reset the chip
    delay(2000);
    digitalWrite(0, LOW);
    counter = 0;
 
  } 
}
void reconnect() {
  // Loop until we're reconnected
  Serial.println("Not connected");
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("power")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
//      client.publish("outTopic", "hello world");
      client.publish(outTopic, "My power-reader - connected");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {
  // put your main code here, to run repeatedly:

    if (!client.connected()) {
    reconnect();
  }
    client.loop();

if (counter > lastcounter) {
  diffcounter = counter-lastcounter;
  sleepseconds=sleeptime/1000;
  float pulses = diffcounter/sleepseconds;   // count pulses per loop time
  kwh = long(3600UL*pulses); // get kwh multiplied 3.6kwh * pulses per time
  //char* chardiffcounter = f2s(diffcounter,2);
  //char* charkwh = f2s(kwh,1);
  //char* charcounter = f2s(counter,0);
    long now = millis();
    if(now - lastTime > sendPeriod){ // send data to mqtt if sendPeriod is say 20 seconds
      lastTime = now;
      kwh=3*60*diffcounter;
      char* chardiffcounter = f2s(diffcounter,2);
      char* charkwh = f2s(kwh,1);
      char* charcounter = f2s(counter,0);
      client.publish(powerTopic,charkwh);   // publish kwh
      client.publish(powerTopic, chardiffcounter); // publish counter since last send period
      client.publish(powerTopic, charcounter); // use this one to send cumulative pulses, needs to reset the meter once per day to get real pulses per day
      lastcounter = counter;
    }
}
}


void blink() { // run this piece at every interrupt

  // Increment counter
  counter++;
  
}

/* float to string
 * f is the float to turn into a string
 * p is the precision (number of decimals)
 * return a string representation of the float.
 */
char *f2s(float f, int p){
  char * pBuff;                         // use to remember which part of the buffer to use for dtostrf
  const int iSize = 10;                 // number of buffers, one for each float before wrapping around
  static char sBuff[iSize][20];         // space for 20 characters including NULL terminator for each float
  static int iCount = 0;                // keep a tab of next place in sBuff to use
  pBuff = sBuff[iCount];                // use this buffer
  if(iCount >= iSize -1){               // check for wrap
    iCount = 0;                         // if wrapping start again and reset
  }
  else{
    iCount++;                           // advance the counter
  }
  return dtostrf(f, 0, p, pBuff);       // call the library function
}

