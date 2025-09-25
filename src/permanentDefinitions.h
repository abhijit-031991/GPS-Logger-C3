#define SLEEP_INTERVAL_SECONDS 60
#define GPS_EN 8
#define VSNS_PIN 2


#define ADC_MAX_VALUE    4095        // 12-bit ADC resolution
#define VREF             1100        // mV, internal reference voltage (default ~1100 mV, but can vary)
#define WIFI_TIMEOUT_MS  300000 


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

struct testGPS{
  bool gps;
  double lat;
  double lng;
};    

// EEPROM MetaData Address //

int eepromAddress = 1;

// Device ID //

const uint16_t tag = 10540;
const char* deviceName = "ArcTrack - 10540";

// Firmware Version //
const float firmwareVersion = 3;