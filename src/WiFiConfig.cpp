
#include "WiFiConfig.h"

int _currentConfig = -1;

int currentConfig()
{
  return _currentConfig;
}

bool useWiFiConfig(WiFiConfig config)
{
  Serial.print(String("trying to connect to ") + config.name + " ");
  //WiFi.disconnect();
  //WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  if (config.useDHCP) {
    IPAddress nullAddr = IPAddress(0, 0, 0, 0);
    //WiFi.config(nullAddr, nullAddr, nullAddr);
  } else {
    WiFi.config(config.ownIP, config.gwIP, config.subnetIP);
  }
  WiFi.begin(config.ssid, config.password);
  int tries = 0;
  const int maxTries = 20;
  while (tries < maxTries) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(String(" - connected to ") + config.name + ".");
      Serial.println((String)"local IP is " + WiFi.localIP().toString());
      return true;
    }
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println(String(" - no connection to ") + config.name + ".");
  return false;
}

void saveWiFiConfig(int idx)
{
  Serial.println("save new config info");
  EEPROM.write(0, 'W');
  EEPROM.write(1, 'F');
  EEPROM.write(2, 'C');
  EEPROM.write(3, idx);
  EEPROM.commit();
}

int WiFiSetup()
{
  int confNo = -1;

  // try last used setup
  EEPROM.begin(512);
  if (EEPROM.read(0) == 'W' && EEPROM.read(1) == 'F' && EEPROM.read(2) == 'C') {   // valid WiFiConfig
    Serial.print("found last WiFi config: ");
    confNo = EEPROM.read(3);
    Serial.println(confNo);
    if (useWiFiConfig(wifiConfigs[confNo])) {
      _currentConfig = confNo;
      return _currentConfig;
    }
  }

  // try known setups from scan
  Serial.println("Scanning WiFi");
  int numSSIDs = WiFi.scanNetworks();
  Serial.println(String("...found ") + numSSIDs + " networks:");
  for (int i = 0; i < numSSIDs; i++) {
    Serial.println(String("  ") + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ")");
  }
  for (int i = 0; i < numSSIDs; i++) {
    for (int j = 0; j < dim(wifiConfigs); j++) {
      if (WiFi.SSID(i) == String(wifiConfigs[j].ssid)) {
        if (useWiFiConfig(wifiConfigs[j])) {
          saveWiFiConfig(j);
          _currentConfig = confNo;
          return _currentConfig;
        }
      }
    }
  }

  // try all given setups
  for (int i = 0; i < dim(wifiConfigs); i++) {
    Serial.println(String("Check WiFi config no.") + i);
    if (useWiFiConfig(wifiConfigs[i])) {
      Serial.println("successful");
      if (i != confNo) { // save new configuration
        saveWiFiConfig(i);
      }
      _currentConfig = confNo;
      return _currentConfig;
    }
  } // for
  _currentConfig = -1;
  return _currentConfig;
}
