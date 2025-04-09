#pragma once


//////// NETWORK

// WiFi
#include "WiFiConfig.h"

// MQTT client
#include <PubSubClient.h>


#include "Config.h"

extern bool hasConnection /* = false*/;
extern IPAddress ip;
extern PubSubClient mqttClient;
extern int wifiConn /* = -1 */;

const int TOPICSTRLEN = 50;

extern char topicStatus[TOPICSTRLEN];
extern char topicCommand[TOPICSTRLEN];
extern char topicAct[TOPICSTRLEN];
extern char topicThresh[TOPICSTRLEN];
extern char topicAlarm[TOPICSTRLEN];
#if RNG
extern char topicRNG[TOPICSTRLEN];
#endif
extern char msgStr[200];
extern int cycleNo /* = 0 */;

void setupTopics();
void publish_mqtt(const char* topic, const char* msg);
void callback_mqtt(char* topic, byte* payload, unsigned int length);
void mqtt_reconnect();
void setup_mqtt();
void loop_mqtt();