/*
  Geiger.cpp
  This code interacts with the Alibaba RadiationD-v1.1 (CAJOE) Geiger counter board
  and reports readings in CPM (Counts Per Minute).
  via MQTT

  Author: \sk
  Based on work by Andreas Spiess (https://github.com/SensorsIot/Geiger-Counter-RadiationD-v1.1-CAJOE-/)
  Based on initial work of Mark A. Heckler (@MkHeck, mark.heckler@gmail.com)
  License: MIT License
  Please use freely with attribution. Thank you!

  Board: e.g. ESP32 -> Wemos LOLIN32
  Libraries needed:
      PubSubClient@2.8
      ESP8266_and_ESP32_OLED_driver_for_SSD1306_displays@4.3.0

  MQTT topics used for publishing:
    sensor/radio/any/     (prefix parts areconfigurable)
      status          status text
      activity        current activity (cpm)
      threshold       current threshold (cpm)
      alarm           current alarm status (1 or 0)
      rngbytes        rng bytes generated (if configured)
      command         command channel (subscribed by node; currently unused)

  Version history:

  (ToDo: make alarm thresholds configurable via MQTT input)

  V2.4  RNG feature
  V2.3  support inverse display (to compensate for wearing out pixels)
  V2.2  redo MQTT connection
  V2.1  use gliding average as threshold
  V2.0  switched from Thingspeak/IFTTT to MQTT delivery
  V1    original work by A. Spiess
*/

#define VERSION "2.4"


//////// INCLUDES

#include "Config.h"

#include "Connect_MQTT.h"
//#include "WiFiConfig.h"

#include <SSD1306Wire.h>
#include "icons.h"
//#include <credentials.h> // or define mySSID and myPASSWORD and THINGSPEAK_API_KEY

#ifndef CREDENTIALS

// MQTT client
#include <PubSubClient.h>



#endif

//////// MISC

#define MIN(A,B)  ((A<B) ? A : B)
#define MAX(A,B)  ((A>B) ? A : B)

//////// FORWARDS

void displayInit();
void displayString(String dispString, int x, int y);
void displayInt(int dispInt, int x, int y);

//////// VARIABLES

char acts[20];

SSD1306Wire  display(0x3c, SDAPin, SCLPin);

unsigned long previousMillis;                            // Time measurement
unsigned long previous_cpm = -1;                         // previous measurement
unsigned long cpm[60];                                   // CPM measurements
int second = 0;                                          // rolling index for measurements
bool valid = false;                                      // false during ramping up phase
const int histSize = 24 * 60;
unsigned long history[histSize];                         // CPM measurements history
int histUsed = 0;
int histIndex = 0;
const float thresholdFactor = 4;
int threshold = 0;
const int LINELEN = 256;
char  line[LINELEN];

volatile unsigned long counts = 0;                       // Tube events

#if RNG
const int RNG_OUTPUTSIZE = 64;
volatile unsigned long rng_start;
volatile unsigned long rng_delta1, rng_delta2;
volatile int rng_phase = 0;
volatile bool rng_toggle = false;
volatile int rng_bitnum = 0;
volatile unsigned char rng_byte = 0;
volatile int rng_outputindex = 0;
volatile unsigned char rng_output[RNG_OUTPUTSIZE];
volatile bool rng_chunk_done = false;
#endif

//////// INTERRUPT ROUTINE

void ISR_impulse() { // Captures count of events from Geiger counter board
  counts++;
#if RNG
  if (rng_chunk_done)
    return;
  unsigned long t_now = micros();
  switch (rng_phase) {
    case 0:   rng_start = t_now;
      break;
    case 1:   rng_delta1 = t_now - rng_start;
      break;
    case 2:   rng_start = t_now;
      break;
    case 3:   rng_delta2 = t_now - rng_start;
      if (rng_delta1 != rng_delta2) {
        unsigned char rng_bit = (rng_delta1 < rng_delta2) ^ rng_toggle;
        rng_toggle = !rng_toggle;
        rng_byte = (rng_byte << 1) | rng_bit;
        rng_bitnum++;
        if (rng_bitnum > 7) {
          rng_output[rng_outputindex++] = rng_byte;
          rng_bitnum = 0;
          rng_byte = 0;
          if (rng_outputindex > RNG_OUTPUTSIZE) {
            rng_outputindex = 0;
            rng_chunk_done = true;
          }
        }
      }
      break;
  }
  rng_phase = (rng_phase + 1) % 4;
#endif
}

//////// SETUP

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println("*******************************");
  Serial.println("*                             *");
  Serial.println("*  Radioactivity Sensor Node  *");
  Serial.println("*                             *");
  Serial.println("*  " VERSION "  kater                 *");
  Serial.println("*                             *");
  Serial.println("*******************************");
  Serial.println("");
  displayInit();
  displayString("Welcome", 64, 15);
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 128
#endif

  display.drawXbm((DISPLAY_WIDTH - RADIO_W) / 2, 0, RADIO_W, RADIO_H, (const unsigned char*)radio_bits);
  display.display();

  setup_mqtt();

  display.clear();
  displayString("Ramping up", 64, 15);
  pinMode(inputPin, INPUT);                                                // Set pin for capturing Tube events
  interrupts();                                                            // Enable interrupts
  attachInterrupt(digitalPinToInterrupt(inputPin), ISR_impulse, FALLING);  // Define interrupt on falling edge
  for (int i = 0; i < 60; i++) cpm[i] = 0;
  for (int i = 0; i < histSize; i++) history[i] = 0;
  Serial.println("ramping up");
  //sleep(1000);
}

//////// MAIN LOOP

void loop() {

  unsigned long currentMillis = millis();

  loop_mqtt();

#if RNG
  if (rng_chunk_done) {
    for (int idx = 0; idx < RNG_OUTPUTSIZE; idx++) {
      sprintf(line + 2 * idx, "%02x", rng_output[idx]);
    }
    Serial.println((String)"RNG" + RNG_OUTPUTSIZE + ": " + line);

    publish_mqtt(topicRNG, line);
    rng_chunk_done = false;
  }
#endif

  if (currentMillis - previousMillis > LOG_PERIOD) {
    //Serial.println((String)"tick...  counts = " + counts);
    unsigned long delta = counts;
    previousMillis = currentMillis;
    for (int i = 0; i < 60; i++)
      cpm[i] += delta;
    unsigned long current_cpm = cpm[second];
    cpm[second] = 0;
    second = (second + 1) % 60;

    if (((second % MQTT_PERIOD) == 0) && valid) {
      //snprintf(acts, 20, "%d", current_cpm);
      dtostrf(current_cpm, 0, 0, acts);
      if (hasConnection) {
        publish_mqtt(topicAct, acts);
      }
    }

    if (!second) {
      if (!valid) {
        Serial.println("init done");
        valid = true;
      }
      history[histIndex] = current_cpm;
      histIndex = (histIndex + 1) % histSize;
      histUsed++;
      int histSum = 0;
      int sampleSize = MIN(histSize, histUsed);
      for (int i = 0; i < sampleSize; i++) histSum += history[i];
      Serial.println(String("Mean cpm ") + histSum / sampleSize);
      threshold = histSum / sampleSize * thresholdFactor;
      Serial.println(String("Threshold set to ") + threshold);
      if (hasConnection) {
        //snprintf(acts, 20, "%d", threshold);
        dtostrf(threshold, 0, 0, acts);
        publish_mqtt(topicThresh, acts);
        publish_mqtt(topicAlarm, (current_cpm > threshold) ? "1" : "0");
      }
    }

    snprintf(line, LINELEN, "sec #%02d: %5ld", second, current_cpm);
    Serial.println(line);
    counts = 0;
    if (valid) {
      if (current_cpm != previous_cpm) {
        display.clear();
        if (current_cpm > threshold) {
#if INVERT_OLED
          display.normalDisplay();
#else
          display.invertDisplay();
#endif
          display.drawXbm(0, 0, RADIO_W, RADIO_H, (const unsigned char*)radio_bits);
          display.setTextAlignment(TEXT_ALIGN_CENTER);
          display.setFont(ArialMT_Plain_24);
          display.drawString(96, 30 , String(current_cpm));
          display.display();
        } else {
#if INVERT_OLED
          display.invertDisplay();
#else
          display.normalDisplay();
#endif
          display.setTextAlignment(TEXT_ALIGN_CENTER);
          displayString("Radioaktivität", 64, 0);
          displayInt(current_cpm, 64, 30);
#if INVERT_OLED
          // additional display for better readability on worn out OLED screen
          //display.setTextAlignment(TEXT_ALIGN_LEFT);
          displayInt(current_cpm, 24 + random(80), 12);
#endif
        }
        previous_cpm = current_cpm;
      } else {
        // no change => do nothing
      }
    } else {
      display.clear();
      //displayString("<starting>", 64, 0);
      display.drawProgressBar(4, 16, 120, 5, second * 100 / 60);
      displayInt(current_cpm, 64, 30);
    }
  }
}

void displayInit() {
  Serial.println("init OLED display");
  Wire.begin(SDAPin, SCLPin);
  display.init();
  display.flipScreenVertically();
  display.clear();
#if INVERT_OLED
  display.invertDisplay();
#else
  display.normalDisplay();
#endif
  display.setColor(FGCOLOR);
  display.setFont(ArialMT_Plain_24);
}

void displayInt(int dispInt, int x, int y) {
  display.setColor(FGCOLOR);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(x, y, String(dispInt));
  display.display();
}

void displayString(String dispString, int x, int y) {
  display.setColor(FGCOLOR);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(x, y, dispString);
  display.display();
}

/***
  void publishMQTT(const char* topic, const char* msg) {
  int result = mqttClient.publish(topic, msg);
  Serial.println(String("MQTT publish(\"") + topic + "\", \"" + msg + "\") -> " + (result ? "ok" : "error"));
  }

  void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  #if 0
    // Switch on the LED if '1' was received as first character
    if ((char)payload[0] >= '1' && (char)payload[0] <= '4') {
      digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
      digitalWrite(D7, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is acive low on the ESP-01)
    } else {
      digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
      digitalWrite(D7, HIGH);  // Turn the LED off by making the voltage HIGH
    }
  #endif
  }

  #define TOPIC_OUT "fhem/announce"
  #define TOPIC_IN  "commands"

  void mqtt_reconnect() {
  Serial.println("mqtt_reconnect()");
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Not connected, state = ");
    Serial.println(mqttClient.state());
    Serial.print("Attempting to connect to MQTT broker... ");
    // Attempt to connect
    char msg[100];
    // construct last will + testament message
    snprintf(msg, 100, "DOWN: %s @ %s", clientName, clientLocation);
  #if USE_MQTTS
    if (mqttClient.connect(clientName, mqtt_username, mqtt_password, TOPIC_OUT, 2, 1, msg))
  #else
    if (mqttClient.connect(clientName, NULL, NULL, TOPIC_OUT, 2, 1, msg))
  #endif
    {
      Serial.println("connected");
      delay(1000);
      // Once connected, publish an announcement...
      snprintf(msg, 100, "UP: %s @ %s", clientName, clientLocation);
      int ret = mqttClient.publish(TOPIC_OUT, msg);
      Serial.print("mqttClient.publish(");
      Serial.print(TOPIC_OUT);
      Serial.print(", \"");
      Serial.print(msg);
      Serial.print("\") -> ");
      Serial.println(ret);
      // ... and resubscribe
      mqttClient.subscribe(TOPIC_IN);
    } else {
      Serial.print("failed, state = ");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  }


  void setupTopics() {
  snprintf (topicAct, 100, "%s/%s/activity", topicPath, clientLocation);
  snprintf (topicThresh, 100, "%s/%s/threshold", topicPath, clientLocation);
  snprintf (topicAlarm, 100, "%s/%s/alarm", topicPath, clientLocation);
  }
***/
