//
//  Util.cpp         izinto Utilities
//
//  2019-04-16 izinto\sk
//

#include "Util.h"
#include <EEPROM.h>
#include <Wire.h>
//#include <esp_deep_sleep.h>   -- obsolete header file

int UTIL_DEBUG = 0;

String buf2String(uint8_t* buf, int len)
{
  String ret = String("");
  for (int i = 0; i < len; i++) {
    uint8_t b = buf[i];
    if (b < 0x10)
      ret = ret + "0";
    ret = ret + String(b, HEX);
    if (i < (len - 1))
      ret = ret + " ";
  }
  return ret;
}

//***************************************************************************************************
// dump memory for debugging output
void memdmp(const char* header, const uint8_t* p, uint16_t len)
{
  if (!UTIL_DEBUG) return;

  uint16_t i;                                                        // Loop control
  Serial.print(header);                                           // Show header
  for (i = 0; i < len; i++) {
    if ((i & 0x1F) == 0) {                                     // Continue on next line?
      if (i > 0)  {                                                 // Yes, continuation line?
        Serial.printf("\n");                                      // Yes, print it
      }
      // Serial.printf("%04X: ", i);                                 // Print index
    }
    Serial.printf("%02X ", *p++);                                 // Print one data byte
  }
  Serial.println();
}

#ifdef OBSOLETE
//***************************************************************************************************
// deepsleep for a given time
void deepsleep(long int sleeptime, bool disable_pd)
{

  /*
    First we configure the wake up source
    We set our ESP32 to wake up every <sleeptime> seconds
  */
  esp_sleep_enable_timer_wakeup(sleeptime);
  //if (UTIL_DEBUG > 0) Serial.println("setup ESP32 to sleep for " + String(sleeptime / 1000000) + " Seconds");

  /*
    Next we decide what all peripherals to shut down/keep on
    By default, ESP32 will automatically power down the peripherals
    not needed by the wakeup source, but if you want to be a poweruser
    this is for you. Read in detail at the API docs
    http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
    Left the line commented as an example of how to configure peripherals.
    The line below turns off all RTC peripherals in deep sleep.
  */
  if (disable_pd) {
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    // do NOT turn off SLOW_MEM since that is used for storage
    //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    if (UTIL_DEBUG > 0) Serial.println("configured all RTC Peripherals to be powered down in sleep");
  }
  /*
    Now that we have setup a wake cause and if needed setup the
    peripherals state in deep sleep, we can now start going to
    deep sleep.
    In the case that no wake up sources were provided but deep
    sleep was started, it will sleep forever unless hardware
    reset occurs.
  */
  if (UTIL_DEBUG > 0) Serial.println("going to sleep now. --");
  Serial.flush();
  esp_deep_sleep_start();
}

//***************************************************************************************************
// print the reason by which ESP32 has been awaken from sleep
esp_sleep_wakeup_cause_t print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (UTIL_DEBUG) {
    switch (wakeup_reason)
    {
      case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
      case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
      case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
      case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
      default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
    }
  }
  return wakeup_reason;
}
#endif

int Storage::begin()
{
  return 1;
}

//// RTC storage functions
//
RTC_DATA_ATTR uint8_t rtcdata[64];              // data in RTC ram stays valid during deep sleep

void RTCStorage::store(uint8_t* data, int size)
{
  if (UTIL_DEBUG > 0) Serial.print(String("rtcmemStore ") + size + " bytes: ");
  for (int offset = 0; offset < size; offset++) {
    uint8_t b = data[offset];
    rtcdata[offset] = b;
    if (UTIL_DEBUG > 0) Serial.printf("%02X ", b);
  }
  if (UTIL_DEBUG > 0) Serial.println();
}

void RTCStorage::retrieve(uint8_t* data, int size)
{
  if (UTIL_DEBUG > 0) Serial.print(String("rtcmemRetrieve ") + size + " bytes: ");
  for (int offset = 0; offset < size; offset++) {
    uint8_t b = rtcdata[offset];
    data[offset] = b;
    if (UTIL_DEBUG > 0) Serial.printf("%02X ", b);
  }
  if (UTIL_DEBUG > 0) Serial.println();
}

//// EEPROM storage functions
//
int EEPROMStorage::begin() {
  int ret = EEPROM.begin(512);
  if (!ret) {                           // Init EEPROM, 512 byte should be sufficient
    if (UTIL_DEBUG > 0) Serial.println("Error initializing EEPROM storage");
  }
  return ret;
}

void EEPROMStorage::store(uint8_t* data, int size)
{
  if (UTIL_DEBUG > 0) Serial.print(String("e2promStore ") + size + " bytes: ");
  for (int offset = 0; offset < size; offset++)
  {
    uint8_t b = data[offset];
    EEPROM.write(offset, b);                       // Write to EEPROM
    if (UTIL_DEBUG > 0) Serial.printf("%02X ", b);
  }
  if (UTIL_DEBUG > 0) Serial.println();
  EEPROM.commit();
}

void EEPROMStorage::retrieve(uint8_t* data, int size)
{
  if (UTIL_DEBUG > 0) Serial.print(String("e2promRetrieve ") + size + " bytes: ");
  for (int offset = 0; offset < size; offset++)
  {
    uint8_t b = EEPROM.read(offset);
    data[offset] = b;
    if (UTIL_DEBUG > 0) Serial.printf("%02X ", b);
  }
  if (UTIL_DEBUG > 0) Serial.println();
}

//////////////////////////// DEBUG HELPERS //////////////////////////////

bool i2c_hasDevice(byte address)
{
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

int i2c_scan(bool verbose /* = true */)
{
  if (!UTIL_DEBUG) return -1;
  byte error, address;
  int nDevices;

  Serial.println("Scanning I²C bus...");

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I²C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      if (verbose) {
        Serial.print(" = ");
        Serial.print(i2cname(address));
      }
      Serial.println(",");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("!");
    }
    /*else
      {
      Serial.print("No device at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(",");
      }*/
  }
  if (nDevices == 0) {
    Serial.println("No I²C devices found\n");
  } else {
    Serial.print(nDevices);
    Serial.println(" found - done.\n");
  }
  return nDevices;
}

const char* i2cname(int address) {
  switch (address) {
    case 0x29:  return "TCS 34735: color sensor"; break;
    case 0x38:  return "VEML 6070: UV sensor"; break;
    case 0x39:  return "VEML 6070: UV sensor"; break;
    case 0x3C:  return "SSD 1306: OLED"; break;
    case 0x3F:  return "PCF/LCD"; break;
    case 0x44:  return "SHT 31: temp/hum sensor"; break;
    case 0x5A:  return "MLX 90614: IR temp sensor"; break;
    case 0x60:  return "SI 1145: IR sensor"; break;
    case 0x61:  return "SCD 30: CO2 sensor"; break;
    case 0x69:  return "SPS 30: particle matter sensor"; break;
    case 0x76:  return "BME/BMP 280: pressure/temp/[hum] sensor"; break;
    case 0x77:  return "BME/BMP 280: pressure/temp/[hum] sensor"; break;
    default:    ;
  }
  return "<unknown/other>";
}
