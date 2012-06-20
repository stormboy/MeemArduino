
/**
 *
 * Web server exists for things on the network to read and set configuration.
 *
 * Configurable stuff to store in EEPROM:
 *  - MAC Address set.
 *  - Meem ID
 *  - MQTT server (should discover dynamically)
 
 * TODO subscribe to initial-content requests on this device's outbound facets. publish content when request made
 */

#include <SPI.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <TrueRandom.h>
#include <EthernetDHCP.h>
#include <EthernetBonjour.h>
#include <Meem.h>

using namespace Meem;

#define DEBUG 1

// a MAC address for the controller.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// default IP settings
IPAddress ip(192, 168, 0, 112);

// the MQTT
byte mqttHost[] = { 192, 168, 0, 17 };
int mqttPort = 1883;


enum io_type { digital, analog, loopback };

typedef struct {
  io_type ioType;   // digital or analog
  int pin;          // the IO pin
  int direction;    // input or output
  int lastValue;    // last value recorded for this pin
} IoDesc;

// facet descriptors
const int numFacets = 14;
const FacetDesc facets[] = {
  // inbound facets that write to device outputs
  { "binaryInput0", binary, INPUT },
  { "binaryInput1", binary, INPUT },
  { "binaryInput2", binary, INPUT },
  { "binaryInput3", binary, INPUT },
  { "linearInput0", linear, INPUT },
  { "linearInput1", linear, INPUT },
  
  // outbound facets that get their values from device inputs
  { "binaryOutput0", binary, OUTPUT },
  { "binaryOutput1", binary, OUTPUT },
  { "binaryOutput2", binary, OUTPUT },
  { "binaryOutput3", binary, OUTPUT },
  { "linearOutput0", linear, OUTPUT },
  { "linearOutput1", linear, OUTPUT },

  { "loopInput0", binary, INPUT },
  { "loopOutput0", binary, OUTPUT },
};

// The index of these IO ports matches the facet array
IoDesc ioPorts[] = {
  // output pins
  { digital, 0, OUTPUT },
  { digital, 1, OUTPUT },
  { digital, 2, OUTPUT },
  { digital, 3, OUTPUT },
  { analog,  A0, OUTPUT },
  { analog,  A1, OUTPUT },
  
  // input pins
  { digital, 4, INPUT },
  { digital, 5, INPUT },
  { digital, 6, INPUT },
  { digital, 7, INPUT },
  { analog,  A2, INPUT },
  { analog,  A3, INPUT },
  
  { loopback, 0, OUTPUT },    // sends message to INPUT 0
  { loopback, 0, INPUT },     // receives message from INPUY 0
};

// the UUID string for this device
char meemUUID[] = {'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 
  '-', 'x', 'x', 'x', 'x', 
  '-', 'x', 'x', 'x', 'x', 
  '-', 'x', 'x', 'x', 'x', 
  '-', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', '\0' };

char strBuffer[128];

/*

const char HexChars[] = {
  '0', '1', '2', '3', '4', '5',
  '6', '7', '8', '9', 'a', 'b',
  'c', 'd', 'e', 'f' 
};

void uuidToString(uint8_t uuid[16], char uuidString[37]) {
  for (int i=0, j=0; i<16; i++) {
    switch(i) {
      case 4:
      case 6:
      case 8:
      case 10:
        uuidString[j++] = '-';
      default:
        uint8_t num = uuid[i];
        uuidString[j++] = HexChars[(num >> 4)];
        uuidString[j++] = HexChars[(num & 0x0f)];
    }
  }
}

void createUUID(char* uuidString) {
  uint8_t uuid[16];
  TrueRandom.uuid(uuid);
  uuidToString(uuid, uuidString);
}
*/

/**
 * Fact callback
 */
void inboundFacetCallback(int facetIndex, char* payload) {
  int len = strlen(payload);

  //Serial.print((int)facetIndex);
  
  if (facetIndex > 0) {
#ifdef DEBUG
    Serial.print(facets[facetIndex].name);
    Serial.println(payload);
#endif 
    IoDesc io = ioPorts[facetIndex];
    if (io.ioType == digital) {
      // get value from payload
      char str[8]; memset(str, NULL, 8);
//      sscanf(payload, "(value %s)", &str);        //!!!! sscanf adds 2k
      int value = (strncmp("true", str, 4) == 0) ? HIGH : LOW;
      digitalWrite(io.pin, value);
    }
    else if (io.ioType == analog) {
      // 10 bit - from 0 to 1023
      int value = HIGH;
//      sscanf(payload, "(value %d)", &value);
      analogWrite(io.pin, value);
    }
  }
}

void serviceFound(const char* type, MDNSServiceProtocol proto,
                  const char* name, const byte ipAddr[4],
                  unsigned short port,
                  const char* txtContent)
{
  if (NULL == name) {
	Serial.print("Finished type ");
	Serial.println(type);
  } else {
    Serial.print("Found: '");
    Serial.print(name);
    /*
    Serial.print("' at ");
    //Serial.print(ip_to_str(ipAddr));
    Serial.print(", port ");
    Serial.print(port);
    Serial.println(" (TCP)");

    if (txtContent) {
      Serial.print("\ttxt record: ");
      
      char buf[256];
      char len = *txtContent++, i=0;;
      while (len) {
        i = 0;
        while (len--)
          buf[i++] = *txtContent++;
        buf[i] = '\0';
        Serial.print(buf);
        len = *txtContent++;
        
        if (len)
          Serial.print(", ");
        else
          Serial.println();
      }
    }
    */
  }
}

// Initialize the Ethernet server library with the IP address and port
//EthernetServer server(80);


/**
 * TODO read settings from EEPROM
 */
void readSettings() {
  //  read meemUUID. if not exists then generate
  
  //createUUID(meemUUID);
//  Serial.print("uuid: ");
//  Serial.println(meemUUID);
}



/**
 * Process web server request, if one exists
 */
 /*
void handleWebServerRequests() {
    // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline character) and the line is blank, the http request has ended, so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // TODO check whether a POST or GET
          
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain; charset=utf-8");
          client.println("Connnection: close");
          client.println();
          
          // send device status
          client.print("((configuration (meemUUID ");
          client.print(meemUUID);
          client.println("))");
          client.println("(values (");
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {                 // output the value of each analog input pin
            int sensorReading = analogRead(analogChannel);
            client.print("  (linearInput");
            client.print(analogChannel);
            client.print(" (value ");
            client.print(sensorReading);
            client.println("))");     
          }
          client.println(")))");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
}
*/

MqttMeem meem(meemUUID, facets, numFacets, inboundFacetCallback);

/**
 * Check device inputs for changes and if any exist, publish to proper MQTT topic.
 */
void checkDeviceInputs() {
  
  for (int i=0; i<numFacets; i++) {
    IoDesc io = ioPorts[i];
    if (io.direction == INPUT) {  // only insterested in inputs
      int value;
      switch (io.ioType) {
      case digital:
        value = digitalRead(io.pin);
        if (value != io.lastValue) {
            const char *v = value ? "true" : "false";
            sprintf(strBuffer, "(value %s)", v);
            meem.sendToOutboundFacet(i, strBuffer);
            ioPorts[i].lastValue = value;
        }
        break;
      case analog:
        value = analogRead(io.pin);
        if (value != io.lastValue) {
          sprintf(strBuffer, "(value %i)", value);
          ioPorts[i].lastValue = value;
        }
        break;
      }
    }
  }
}


/**
 * Setup the device
 */
void setup() {
  /*
  memset(meemUUID, '\0', 37);
  */
 
   // Open serial communications and wait for port to open:
  Serial.begin(9600);
  
  // read settings from EEPROM
  readSettings();

#ifdef DEBUG
  Serial.print("starting device...");
#endif

  // set the pin directions  
  for (int i=0; i<numFacets; i++) {
    IoDesc port = ioPorts[i];
    if (port.direction != loopback) {
      pinMode(port.pin, port.direction);
    }
  }

#ifdef DEBUG
  Serial.print("geting ip address: ");
#endif

  if (EthernetDHCP.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);        // initialize the ethernet device with default settings
  }
//  Ethernet.begin(mac, ip);        // initialize the ethernet device with default settings

  // TODO maybe broadcast this device on network.
  
#ifdef DEBUG
  Serial.print("got IP address: ");
  Serial.println(Ethernet.localIP());
#endif

  EthernetBonjour.begin();
  EthernetBonjour.setServiceFoundCallback(serviceFound);
  EthernetBonjour.startDiscoveringService("_mqtt", MDNSServiceTCP, 5000);
  
//  server.begin();

  meem.setMeemUUID(meemUUID);
  meem.connect(mqttHost, mqttPort); 
}


/**
 * Main device loop.
 */
void loop() {
  
  EthernetBonjour.run();
  
//  handleWebServerRequests();

  checkDeviceInputs();

  meem.loop();
}


