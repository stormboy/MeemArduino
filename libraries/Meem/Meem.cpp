/**
 * Meem.cpp - A Meem for MQTT.
 * Warren Bloomer
 * http://sugarcoding.net/
 */

#include "Meem.h"
#include <Arduino.h>
#include <SPI.h>
#include <PubSub.h>
//#include <string.h>

namespace Meem {

	/*
		const char HexChars[] = {
		  '0', '1', '2', '3', '4', '5',
		  '6', '7', '8', '9', 'a', 'b',
		  'c', 'd', 'e', 'f' 
		};
		
		void uuidToString(uint8_t uuid[16], char uuidString[36]) {
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
	
	// static initialiser
	MqttMeem* MqttMeem::singleton = NULL;

	// the MQTT client object
	PubSubClient mqttClient((byte[]){ 192, 168, 0, 21 }, 1883, MqttMeem::inboundMessageCallback);
	
	
	/**
	 * initialise the Meem object
	 */
	MqttMeem::MqttMeem(const char* meemUUID, const FacetDesc* facets, int numFacets, void (*inboundFacetCallback)(int /*facetIndex*/, char* /*payload*/)) {
        //memset(charBuffer, '\0', STR_BUFFER_LENGTH);
		this->meemUUID = meemUUID;
        
        this->numFacets = numFacets;
	   this->facets = facets;
	   this->inboundFacetCallback = inboundFacetCallback;
	   
        this->singleton = this;
	}
		
	
	/**
	 * connect to MQTT server
	 */
	boolean MqttMeem::connect(byte ip[], uint16_t port) {
        
#ifdef MEEM_DEBUG
	   Serial.println("connecting to MQTT service");
#endif
        
       if (mqttClient.connected()) {
		 mqttClient.disconnect();
	   }
	   mqttClient.setServer(ip, port);
	
	#ifdef MEEM_DEBUG
	  //Serial.println("connecting MQTT client ");
	#endif
        
	   boolean connected = mqttClient.connect(meemUUID);
	   if (connected) {
		  //Serial.print("connected to MQTT service for ");
		  //Serial.println(meemUUID);

			// subscribe to inbound facets	
			strcpy(charBuffer, meem_reg_topic);
			strcat(charBuffer, "/");
			strcat(charBuffer, this->meemUUID);
			strcat(charBuffer, "/in/#");
		  
		  //Serial.print("subscribing to ");
		  //Serial.println(charBuffer);
	
			if ( mqttClient.subscribe(charBuffer) == false) {
#ifdef MEEM_DEBUG
				  Serial.print("subscription failed");
#endif
			}

		  // add to meem registry
		  strcpy(charBuffer, "(added ");
		  strcat(charBuffer, this->meemUUID);
		  strcat(charBuffer, ")");
		  
		  mqttClient.publish(meem_reg_topic, charBuffer);

		  // set lifecycle state

			strcpy(charBuffer, meem_reg_topic);
			strcat(charBuffer, "/");
			strcat(charBuffer, this->meemUUID);
			strcat(charBuffer, "/lifecycle");
		  
		  mqttClient.publish(charBuffer, "(state ready)");
	  }
	
	   return connected;
	}
	
	/**
	 * The main loop
	 */
	boolean MqttMeem::loop() {
	  return mqttClient.loop();
	}
	
	/**
	 * Send a message to an outbound facet
	 */
	boolean MqttMeem::sendToOutboundFacet(int facetIndex, char* payload) {
        FacetDesc facetDesc = facets[facetIndex];
        
        strcpy(charBuffer, meem_reg_topic);
        strcat(charBuffer, meemUUID);
        strcat(charBuffer, "/");
        strcat(charBuffer, facetDesc.name);
	   
	#ifdef MEEM_DEBUG
	  Serial.print("sending message ");
	  Serial.print(charBuffer);
	  Serial.print(": ");
	  Serial.println(payload);
	#endif
	
	   return mqttClient.publish(charBuffer,(uint8_t*)payload,strlen(payload),false);
	}
	

	
	/**
	 *
	 */
	void MqttMeem::disconnect() {
        if (mqttClient.connected()) {
           mqttClient.disconnect();   
        }
	}

	/**
	 * handle message arrived
	 */
	void MqttMeem::inboundMessageCallback(char* topic, uint8_t* payload, unsigned int length) {
        memset(charBuffer, NULL, STR_BUFFER_LENGTH);
        memcpy(charBuffer, payload, length);

	#ifdef MEEM_DEBUG
//	  Serial.print("message received at ");
//	  Serial.print(topic);
//	  Serial.print(": ");
//	  Serial.println(charBuffer);
	#endif
	
	  // TODO check if configuration reqest
	  int isConfigurationFacet = false;
	  if (isConfigurationFacet) {
		// TODO do config update
		// 	    singleton->configure();
	  }
	  else {
		if (MqttMeem::singleton && MqttMeem::singleton->inboundFacetCallback) {
            MqttMeem::singleton->handleInboundMessage(topic, charBuffer, length);
		}
		else {
		  //Serial.print("cannot sent incoming message; singleton not set");
		}
	  }
	
	}
    
    void MqttMeem::handleInboundMessage(char* topic, char* message, unsigned int length)  {
        //  locate facet index
        int facetIndex = -1;
        for (int i=0; i<this->numFacets; i++) {
            FacetDesc facet = this->facets[i];
            int n = strlen(topic) - strlen(facet.name);
            if (strcmp(topic+n, facet.name) == 0) {
                facetIndex = i;
                break;
            }
        }
        
        this->inboundFacetCallback(facetIndex, message);
    }

}