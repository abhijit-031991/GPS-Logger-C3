#define SLEEP_INTERVAL_SECONDS 1
#define GPS_EN 8

// EEPROM Struct // Contains Basics and setting info

struct meta{
    int gfrq;
    int gto;
    int hdop;
    uint16_t count;
    uint32_t wa;
    uint32_t ra;
    };

// Data Struct // Contains GPS data
struct locdata{
    uint16_t count;
    uint32_t datetime;
    uint16_t locktime;
    float lat;
    float lng;
    byte hdop;
    };

// EEPROM MetaData Address //

int eepromAddress = 1;

// Device ID //

const uint16_t tag = 10539;
const char* deviceName = "ArcTrack - 10539";

// Firmware Version //
const float firmwareVersion = 3;