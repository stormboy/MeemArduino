
// enable writing to serial
//#define DEBUG 1

//#define WEBSERVER 1

// enable if the device is to register itself
//#define ZEROCONF_REGISTER 1

// enable if the device is to discover an MQTT service
//#define ZEROCONF_DISCOVER 1

// to enable DHCP
#define DHCP 1

// to generate a UUID
//#define GENERATE_UUID 1

enum io_type { digital, analog, loopback };

typedef struct {
  io_type ioType;   // digital or analog
  int pin;          // the IO pin
  int direction;    // input or output
  int lastValue;    // last value recorded for this pin
} IoDesc;



// a MAC address for the controller.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// default IP settings. If DHCP is enabled, this will be automatically determined by a DHCP service onthe network
IPAddress ip(192, 168, 0, 112);

// the MQTT. If discovery is enabled, this will be updated with what MQTT service is discovered on the network via zeroconf.
byte mqttHost[] = { 192, 168, 0, 17 };

int mqttPort = 1883;

