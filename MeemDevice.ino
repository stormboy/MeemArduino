
/**
 *
 * A device meem that interacts with the world via MQTT.  It depends on an IP adapter.  It has been tested with an ethernet shield.
 *
 * Configurable stuff to store in EEPROM:
 *  - Meem UUID
 *  - MAC Address set.
 *  - MQTT server (should discover dynamically)
 *
 * feedback outbound facets for when inputs facets controlling devices output successfully change the output's value.
 *
 * TODO reconnect to mqtt server if connection is lost. (retry at an interval)
 * TODO subscribe to initial-content requests on this device's outbound facets. publish content when request made
 * TODO remove need for sscanf and printf
 */

#include <SPI.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <PubSubClient.h>

//#ifdef DHCP
  #include <EthernetDHCP.h>
//#endif
//#ifdef ZEROCONF_REGISTER | ZEROCONF_DISCOVER
  #include <EthernetBonjour.h>
//#endif

#ifdef GENERATE_UUID
  #include <TrueRandom.h>
#endif

#include <Meem.h>
#include "MeemDevice.h"

using namespace Meem;

// facet descriptors
const int numFacets = 18;
const int feedbackOffset = 12;  // offset for the feedback facets for inputs
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

  // feedback for the input facets
  { "binaryFeedback0", binary, OUTPUT },
  { "binaryFeedback1", binary, OUTPUT },
  { "binaryFeedback2", binary, OUTPUT },
  { "binaryFeedback3", binary, OUTPUT },
  { "linearFeedback0", linear, OUTPUT },
  { "linearFeedback1", linear, OUTPUT },
};

// The index of these IO ports matches the facet array
IoDesc ioPorts[] = {
  // output pins
  { digital, 6, OUTPUT },
  { digital, 7, OUTPUT },
  { digital, 8, OUTPUT },
  { digital, 9, OUTPUT },
  { analog,  A0, OUTPUT },
  { analog,  A1, OUTPUT },
  
  // input pins
  { digital, 0, INPUT },
  { digital, 2, INPUT },
  { digital, 3, INPUT },
  { digital, 4, INPUT },
  { analog,  A2, INPUT },
  { analog,  A3, INPUT },
  
  { loopback, 0, INPUT },    
  { loopback, 0, INPUT },    
  { loopback, 0, INPUT },  
  { loopback, 0, INPUT },    
  { loopback, 0, INPUT },   
  { loopback, 0, INPUT },
};

// the UUID string for this device
char meemUUID[] = {'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 
  '-', 'x', 'x', 'x', 'x', 
  '-', 'x', 'x', 'x', 'x', 
  '-', 'x', 'x', 'x', 'x', 
  '-', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', '\0' };

// character buffer
#define CHAR_BUF_SIZE 128
char charBuffer[CHAR_BUF_SIZE];

// initialise the ethernet client library
//EthernetClient client;

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
 * declare the function
 */
void inboundFacetCallback(int facetIndex, const char* payload);


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
 * Facet callback.  This is called when a message is being received from the MQTT sever for an
 * inbound Facet.
 */
void inboundFacetCallback(int facetIndex, const char* payload) {
  int len = strlen(payload);

  if (facetIndex >= 0) {
#ifdef DEBUG
    Serial.print(facets[facetIndex].name); Serial.println(payload);
#endif 
    IoDesc port = ioPorts[facetIndex];
    if (port.ioType == digital) {
      // get value from payload
      memset(charBuffer, NULL, CHAR_BUF_SIZE);
      sscanf(payload, "(value %s)", &charBuffer);        //!!!! sscanf adds 2k
      int value = (strncmp("true", charBuffer, 4) == 0) ? HIGH : LOW;
      digitalWrite(port.pin, value);
      
      // send feedback on status of update
      strcpy(charBuffer, payload);
      meem.sendToOutboundFacet(facetIndex+feedbackOffset, charBuffer);
#ifdef DEBUG
      Serial.print("pin "); Serial.print(port.pin); Serial.print(" value: "); Serial.println(value);
#endif
    }
    else if (port.ioType == analog) {
      // 10 bit - from 0 to 1023
      int value = HIGH;
      sscanf(payload, "(value %d)", &value);
      analogWrite(port.pin, value);
      // send feedback on status of update
      memset(charBuffer, NULL, CHAR_BUF_SIZE);
      strcpy(charBuffer, payload);
      meem.sendToOutboundFacet(facetIndex+feedbackOffset, charBuffer);
#ifdef DEBUG
      Serial.print("pin "); Serial.print(port.pin); Serial.print(" value: "); Serial.println(value);
#endif
    }
  }
}

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
            sprintf(charBuffer, "(value %s)", v);
            meem.sendToOutboundFacet(i, charBuffer);
            ioPorts[i].lastValue = value;
        }
        break;
      case analog:
        value = analogRead(port.pin);
        if ( abs(value - port.lastValue) > 100) {
          sprintf(charBuffer, "(value %i)", value);
          meem.sendToOutboundFacet(i, charBuffer);
          ioPorts[i].lastValue = value;
        }
        break;
      }
    }
  }
}


/**
 * SETUP
 *
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
      
      // TODO initialise the inputs??
      if (port.direction = INPUT) {
        if (port.ioType == digital) {
          digitalWrite(port.pin, HIGH);
        }
        else if (port.ioType == analog) {
          analogWrite(port.pin, HIGH);
        }
      }
    }
  }

#ifdef DEBUG
  Serial.print("geting ip address: ");
#endif

#ifdef DHCP
  //if (Ethernet.begin(mac) == 0) {
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
 * LOOP
 *
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
}


