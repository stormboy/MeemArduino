MeemArduino
===========

This code publishes an Arduino's digital and analog inputs to an MQTT service, and subscribes to the MQTT service 
to update device outputs.

The topics used to send and receive messages represent Meem Facets (refer to Meemplex documentation).

This project is for an Arduino with an ethernet adapter.

Dependent Libraries
-------------------

###PubSubClient
http://knolleary.net/arduino-client-for-mqtt/

Modified PubSubClient to allow the host ip and port to be set after construction.  Also changed some 
char* to const char* to allow UUID array to be passed in as he client ID.

###TrueRandom
http://code.google.com/p/tinkerit/wiki/TrueRandom

For generating UUIDs

###Ethernet DHCP
http://gkaindl.com/software/arduino-ethernet/dhcp

For dynamically determining IP address.

###Ethernet Bonjour
http://gkaindl.com/software/arduino-ethernet/bonjour

For discovering a MQTT server.

Environment
-----------
Tested is a mosquitto server running on debian linux.  ZeroConf service registering is done via the avahi-daemon.

Create an avahi configuration file /etc/avahi/services/mosquitto.service

    <MTMarkdownOptions output='raw'>
    <![CDATA[
    <!DOCTYPE service-group SYSTEM "avahi-service.dtd">
    <service-group>
     <name replace-wildcards="yes">Mosquitto MQTT server on %h</name>
      <service>
       <type>_mqtt._tcp</type>
       <port>1883</port>
       <txt-record>info=A MQTT PubSub service! mqtt.org</txt-record>
      </service>
    </service-group>
    ]]>
    </MTMarkdownOptions>

