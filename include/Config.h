#pragma once

//////// CONFIG

#define TEST  0
#define USE_MQTTS 0
#define RNG   1

#define LOG_PERIOD 1000 //Logging period in milliseconds
#define MINUTE_PERIOD 60  // seconds
#define MQTT_PERIOD 5   // seconds

const int inputPin = 26;     // 13;
const int SDAPin =  5;       //  4 25 02
const int SCLPin =  4;       // 15 26 14

// OLED display setup
#define INVERT_OLED   1

/**#if INVERT_OLED
  #define FGCOLOR   BLACK
  #define BGCOLOR   WHITE
  #else
**/
#define FGCOLOR   WHITE
#define BGCOLOR   BLACK
/**#endif**/

// MQTT connection
#if USE_MQTTS
extern WiFiClientSecure wifiClient;
const char* const mqtt_server = "...";
const int mqtt_port = 8883;
#else
extern WiFiClient wifiClient;
const char* const mqtt_server = "10.1.8.20";
const int mqtt_port = 1883;
#endif

// MQTT Topics
#if TEST
const char* const topicPrefix = "temp/sensor";
#else
const char* const topicPrefix = "sensor";
#endif
const char* const clientName= "radio";
const char* const clientLocation  = "any";
