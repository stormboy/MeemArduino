
/**
 *
 * A device meem that interacts with the world via MQTT.  It depends on an IP adapter.  It has been tested with an ethernet shield.
 *
 * Configurable stuff to store in EEPROM:
 *  - Meem UUID
 *  - MAC Address set.
 *  - MQTT server (should discover dynamically)
 *
 * TODO feedback outbound facets for when inputs facets controlling devices output successfully change the output's value.
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
#include "MeemDevice.h"

using namespace Meem;

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
  { digital, 9, OUTPUT },
  { digital, 12, OUTPUT },
  { digital, 11, OUTPUT },
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

#ifdef GENERATE_UUID
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
#endif

/**
 * Facet callback.  This is called when a message is being received from the MQTT sever for an
 * inbound Facet.
 */
void inboundFacetCallback(int facetIndex, char* payload) {
  int len = strlen(payload);

  if (facetIndex > 0) {
#ifdef DEBUG
    Serial.print(facets[facetIndex].name); Serial.println(payload);
#endif 
    IoDesc port = ioPorts[facetIndex];
    if (port.ioType == digital) {
      // get value from payload
      char str[8]; memset(str, NULL, 8);
      sscanf(payload, "(value %s)", &str);        //!!!! sscanf adds 2k
      int value = (strncmp("true", str, 4) == 0) ? HIGH : LOW;
      digitalWrite(port.pin, value);
      // TODO send feedback on status of update
#ifdef DEBUG
      Serial.print("pin "); Serial.print(port.pin); Serial.print(" value: "); Serial.println(value);
#endif
    }
    else if (port.ioType == analog) {
      // 10 bit - from 0 to 1023
      int value = HIGH;
      sscanf(payload, "(value %d)", &value);
      analogWrite(port.pin, value);
      // TODO send feedback on status of update
#ifdef DEBUG
      Serial.print("pin "); Serial.print(port.pin); Serial.print(" value: "); Serial.println(value);
#endif
    }
  }
}

/**
 * Instantiate the MQTT Meem object
 */
MqttMeem meem(meemUUID, facets, numFacets, inboundFacetCallback);


#ifdef ZEROCONF_DISCOVER

/**
 * This function is called when an MQTT service has been discovered.
 */
void serviceFound(const char* type, MDNSServiceProtocol proto,
                  const char* name, const byte ipAddr[4],
                  unsigned short port,
                  const char* txtContent)
{
  if (NULL == name) {
#ifdef DEBUG
	Serial.print("Finished type ");
	Serial.println(type);
	Serial.print(ipAddr[0]);
#endif
  } 
  else {
#ifdef DEBUG
    Serial.print("Found: '");
    Serial.print(name);
#endif

    // (re)connect
    meem.connect(ipAddr, port);
    
    // TODO store address in EEPROM
    
  }
}
#endif // ZEROCONF_DISCOVER

#ifdef WEBSERVER
// Initialize the Ethernet server library with the IP address and port
EthernetServer server(80);
#endif

/**
 * TODO read settings from EEPROM
 */
void readSettings() {
  //  TODO read meemUUID. if not exists then generate
#ifdef GENERATE_UUID
  createUUID(meemUUID);
#endif // GENERATE_UUID
}


#ifdef WEBSERVER
/**
 * Process web server request, if one exists
 */
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
#endif // WEBSERVER

/**
 * Check device inputs for changes and if any exist, publish to proper MQTT topic.
 */
void checkDeviceInputs() {
  
  for (int i=0; i<numFacets; i++) {
    IoDesc port = ioPorts[i];
    if (port.direction == INPUT) {  // only insterested in inputs
      int value;
      switch (port.ioType) {
      case digital:
        value = digitalRead(port.pin);
        if (value != port.lastValue) {
            const char *v = value ? "true" : "false";
            sprintf(strBuffer, "(value %s)", v);
            meem.sendToOutboundFacet(i, strBuffer);
            ioPorts[i].lastValue = value;
        }
        break;
      case analog:
        value = analogRead(port.pin);
        if ( abs(value - port.lastValue) > 100) {
          sprintf(strBuffer, "(value %i)", value);
          meem.sendToOutboundFacet(i, strBuffer);
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
 
 #ifdef DEBUG
   // Open serial communications and wait for port to open:
  Serial.begin(9600);
#endif

  // read settings from EEPROM
  readSettings();

#ifdef DEBUG
  Serial.print("starting device...");
#endif

  // set the pin directions  
  for (int i=0; i<numFacets; i++) {
    IoDesc port = ioPorts[i];
    if (port.ioType != loopback) {
#ifdef DEBUG
      Serial.print("pin "); Serial.print(port.pin); Serial.print(" direction: "); Serial.println(port.direction);
#endif
      pinMode(port.pin, port.direction);
    }
  }

#ifdef DEBUG
  Serial.print("geting ip address: ");
#endif

#ifdef DHCP
  if (EthernetDHCP.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);        // initialize the ethernet device with default settings
  }
#else
  Ethernet.begin(mac, ip);        // initialize the ethernet device with default settings
#endif // DHCP

  // TODO maybe broadcast this device on network.
  
#ifdef DEBUG
  Serial.print("got IP address: ");
  Serial.println(Ethernet.localIP());
#endif

#ifdef ZEROCONF_DISCOVER | ZEROCONF_REGISTER
  EthernetBonjour.begin();
#endif

#ifdef ZEROCONF_DISCOVER
  EthernetBonjour.setServiceFoundCallback(serviceFound);
  EthernetBonjour.startDiscoveringService("_mqtt", MDNSServiceTCP, 5000);
#endif

#ifdef ZEROCONF_REGISTER
  // register service
  EthernetBonjour.addServiceRecord("meemduino._http", 80, MDNSServiceTCP);
#endif

//  server.begin();

  meem.setMeemUUID(meemUUID);
  meem.connect(mqttHost, mqttPort); 
}


/**
 * Main device loop.
 */
void loop() {

#ifdef ZEROCONF_DISCOVER | ZEROCONF_REGISTER
  EthernetBonjour.run();
#endif

#ifdef WEBSERVER
  handleWebServerRequests();
#endif

  checkDeviceInputs();

  meem.loop();
  
//  int lightLevel = analogRead(A2);
//  Serial.println(lightLevel, DEC);
//  delay(200);
}


