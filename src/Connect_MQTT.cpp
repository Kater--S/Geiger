
#include "Connect_MQTT.h"


bool hasConnection = false;
IPAddress ip;
#if USE_MQTTS
WiFiClientSecure wifiClient;
#else
WiFiClient wifiClient;
#endif

PubSubClient mqttClient(wifiClient);
int wifiConn = -1;

char topicStatus[TOPICSTRLEN];
char topicCommand[TOPICSTRLEN];
char topicAct[TOPICSTRLEN];
char topicThresh[TOPICSTRLEN];
char topicAlarm[TOPICSTRLEN];
#if RNG
char topicRNG[TOPICSTRLEN];
#endif
char msgStr[200];
int cycleNo = 0;



void setupTopics() {
  snprintf (topicStatus, TOPICSTRLEN, "%s/%s/%s/status", topicPrefix, clientName, clientLocation);
  snprintf (topicCommand, TOPICSTRLEN, "%s/%s/%s/command", topicPrefix, clientName, clientLocation);

  snprintf (topicAct, TOPICSTRLEN, "%s/%s/%s/activity", topicPrefix, clientName, clientLocation);
  snprintf (topicThresh, TOPICSTRLEN, "%s/%s/%s/threshold", topicPrefix, clientName, clientLocation);
  snprintf (topicAlarm, TOPICSTRLEN, "%s/%s/%s/alarm", topicPrefix, clientName, clientLocation);
#if RNG
  snprintf (topicRNG, TOPICSTRLEN, "%s/%s/%s/rngbytes", topicPrefix, clientName, clientLocation);
#endif
}


void publish_mqtt(const char* topic, const char* msg) {
  int result = mqttClient.publish(topic, msg);
  Serial.println(String("MQTT publish(\"") + topic + "\", \"" + msg + "\") -> " + (result ? "ok" : "error"));
}


void callback_mqtt(char* topic, byte* payload, unsigned int length) {
  Serial.print("*** Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  const int msgsize = 100;
  char msgstr[msgsize];
  int size = (length + 1) < msgsize ? length : msgsize - 1;
  memcpy(msgstr, payload, size);
  msgstr[size + 1] = '\0';
  Serial.println((String)"<" + msgstr + ">");
  String msg = String(msgstr);
  msg.toUpperCase();
  Serial.println((String)"<" + msg + ">");
  if (msg.startsWith("SET ")) {
    Serial.println("setting parameter");
  }
}


void mqtt_reconnect() {
  Serial.println("mqtt_reconnect()");
  // Loop until we're reconnected
  int trycount = 5;
  while (!mqttClient.connected() && trycount--) {
    Serial.print("Not connected, state = ");
    Serial.println(mqttClient.state());
    Serial.print("Attempting to connect to MQTT broker... ");
    // Attempt to connect
    char msg[100];
    // construct last will + testament message
    snprintf(msg, 100, "DOWN: %s @ %s", clientName, clientLocation);
    if (
#if USE_MQTTS
      mqttClient.connect(clientName, mqtt_username, mqtt_password, topicStatus, 2, 1, msg)
#else
      mqttClient.connect(clientName, NULL, NULL, topicStatus, 2, 1, msg)
#endif
    ) {
      Serial.println("connected");
      delay(1000);
      // Once connected, publish an announcement...
      snprintf(msg, 100, "UP: %s @ %s", clientName, clientLocation);
      publish_mqtt(topicStatus, msg);
      // ...tell our command topic...
      snprintf(msg, 100, "CMD: %s", topicCommand);
      publish_mqtt(topicStatus, msg);
      // ... and resubscribe
      int result = mqttClient.subscribe(topicCommand);
      Serial.println(String("MQTT subscribe(\"") + topicCommand + "\") -> " + (result ? "ok" : "error"));
      return;
    } else {
      Serial.print("failed, state = ");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  Serial.println("Problem establishing connection! Restarting system...");
  ESP.restart();  // or: restart();
}


void setup_mqtt()
{
  Serial.println("Connecting to Wi-Fi");
  wifiConn = WiFiSetup();
  if (wifiConn >= 0) {
    Serial.println(String("WiFi connected to ") + wifiConfigs[wifiConn].name);
    setupTopics();
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(callback_mqtt);
    hasConnection = true;
  } else {
    Serial.println("No WiFi connection!");
    hasConnection = false;
  }
}
/***
  Serial.println("Connecting to Wi-Fi");
  wifiConn = WiFiSetup();
  if (wifiConn >= 0) {
    Serial.println(String("WiFi connected to ") + wifiConfigs[wifiConn].name);
    setupTopics();
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(callback);
    Serial.println((String)"MQTT: state = " + mqttClient.state());
    hasConnection = true;
  } else {
    Serial.println("No WiFi connection!");
    hasConnection = false;
  }
***/

void loop_mqtt() {
  if (hasConnection) {
    if (!mqttClient.connected()) {
      Serial.println("no MQTT connection! - Trying to reconnect");
      mqtt_reconnect();
    }
    mqttClient.loop();
  }
}
