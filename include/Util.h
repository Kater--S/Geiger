//
//  Util.h         izinto Utilities
//
//  2019-04-16 izinto\sk
//

#pragma once

#include <Arduino.h>

extern int UTIL_DEBUG;

typedef uint8_t            bit_t;
typedef uint8_t            u1_t;
typedef int8_t             s1_t;
typedef uint16_t           u2_t;
typedef int16_t            s2_t;
typedef uint32_t           u4_t;
typedef int32_t            s4_t;
typedef unsigned int       uint;
typedef const char* str_t;


#define LEN(x) (sizeof(x) / sizeof((x)[0]))

String buf2String(uint8_t* buf, int len);

// dump memory for debugging output
void memdmp(const char* header, const uint8_t* p, uint16_t len);

// deepsleep for a given time (in µs)
void deepsleep(long int sleeptime, bool disable_pd = true);

// print the reason by which ESP32 has been awaken from sleep
esp_sleep_wakeup_cause_t print_wakeup_reason();

// I²C
bool i2c_hasDevice(byte address);
int i2c_scan(bool verbose = true);
const char* i2cname(int address);


class Storage
{
  public:
    virtual int begin();
    virtual void store(uint8_t* data, int size) = 0;
    virtual void retrieve(uint8_t* data, int size) = 0;
};

class RTCStorage: Storage
{
  public:
    void store(uint8_t* data, int size);
    void retrieve(uint8_t* data, int size);
};

class EEPROMStorage: Storage
{
  public:
    int begin();
    void store(uint8_t* data, int size);
    void retrieve(uint8_t* data, int size);
};
