/**
 * Meem.cpp - A Meem for MQTT.
 * Warren Bloomer
 * http://sugarcoding.net/
 */

#include "Meem.h"
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

namespace Meem {
	
	// static initialiser
	MqttMeem* MqttMeem::singleton = NULL;
	
	char MqttMeem::charBuffer[STR_BUFFER_LENGTH];
    
        EthernetClient ethClient;
	// the MQTT client object
	PubSubClient* mqttClient; //((byte[]){ 192 }, 1883, MqttMeem::inboundMessageCallback, ethClient);	

	/**
	 * initialise the Meem object
	 */
	MqttMeem::MqttMeem(char* meemUUID, const FacetDesc* facets, int numFacets, void (*inboundFacetCallback)(int, const char*)) {
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
        
        if (mqttClient != NULL && mqttClient->connected()) {
            mqttClient->disconnect();
        }
        mqttClient = new PubSubClient(ip, port, MqttMeem::inboundMessageCallback, ethClient);
        //mqttClient.setServer(ip, port);
        
        boolean connected = mqttClient->connect(meemUUID);
        if (connected) {
#ifdef MEEM_DEBUG
            Serial.print("connected to MQTT service for ");
            Serial.println(meemUUID);
#endif
			// subscribe to inbound facets	
			strcpy(MqttMeem::charBuffer, meem_reg_topic);
			strcat(MqttMeem::charBuffer, "/");
			strcat(MqttMeem::charBuffer, this->meemUUID);
			strcat(MqttMeem::charBuffer, "/in/#");
            
#ifdef MEEM_DEBUG
            Serial.print("subscribing to ");
            Serial.println(MqttMeem::charBuffer);
#endif	
			if ( mqttClient->subscribe(MqttMeem::charBuffer) == false) {
#ifdef MEEM_DEBUG
                Serial.print("subscription failed");
#endif
			}
            
            // add to meem registry
            strcpy(MqttMeem::charBuffer, "(add ");
            strcat(MqttMeem::charBuffer, this->meemUUID);
            strcat(MqttMeem::charBuffer, ")");
            
            mqttClient->publish(meem_reg_topic, MqttMeem::charBuffer);
            
            
            // send facets
//			strcpy(charBuffer, meem_reg_topic);
//			strcat(charBuffer, "/");
//			strcat(charBuffer, this->meemUUID);
//            
//            char* tmp = (char*) malloc (numFacets*60);
//            strcpy(tmp, "(facets (");
//            for (int i=0; i<numFacets; i++) {
//                FacetDesc facet = facets[i];
//                sprintf(tmp, "(%s %s %s) ", facet.name, facet.facetType, facet.direction);
//            }
//            strcat(tmp, "))");
//            mqttClient->publish(charBuffer, tmp);
//            free(tmp);
            
            // set lifecycle state
			strcpy(MqttMeem::charBuffer, meem_reg_topic);
			strcat(MqttMeem::charBuffer, "/");
			strcat(MqttMeem::charBuffer, this->meemUUID);
			strcat(MqttMeem::charBuffer, "/lifecycle");
            
            mqttClient->publish(charBuffer, "(state ready)");
        }
        
        return connected;
	}
	
	/**
	 * The main loop
	 */
	boolean MqttMeem::loop() {
        return mqttClient->loop();
	}
	
	/**
	 * Send a message to an outbound facet
	 */
	boolean MqttMeem::sendToOutboundFacet(int facetIndex, const char* payload) {
        FacetDesc facetDesc = facets[facetIndex];
        
        strcpy(MqttMeem::charBuffer, meem_reg_topic);
        strcat(MqttMeem::charBuffer, "/");
        strcat(MqttMeem::charBuffer, meemUUID);
        strcat(MqttMeem::charBuffer, "/out/");
        strcat(MqttMeem::charBuffer, facetDesc.name);
        
#ifdef MEEM_DEBUG
        Serial.print("sending message ");
        Serial.print(charBuffer);
        Serial.print(": ");
        Serial.println(payload);
#endif
        
        return mqttClient->publish(MqttMeem::charBuffer,(uint8_t*)payload,strlen(payload),false);
	}
	
	/**
	 * Disconnect from the MQTT server.
	 */
	void MqttMeem::disconnect() {
        if (mqttClient->connected()) {
            mqttClient->disconnect();   
        }
	}
    
	/**
	 * handle message arrived from MQTT
	 */
	void MqttMeem::inboundMessageCallback(char* topic, uint8_t* payload, unsigned int length) {
		memset(MqttMeem::charBuffer, NULL, STR_BUFFER_LENGTH);
		memcpy(MqttMeem::charBuffer, payload, length);
        
#ifdef MEEM_DEBUG
        //	  Serial.print("message received at ");
        //	  Serial.print(topic);
        //	  Serial.print(": ");
        //	  Serial.println(charBuffer);
#endif
        
        if (MqttMeem::singleton && MqttMeem::singleton->inboundFacetCallback) {
            MqttMeem::singleton->handleInboundMessage(topic, MqttMeem::charBuffer, length);
        }
        
	}
    
    void MqttMeem::handleInboundMessage(char* topic, char* message, unsigned int length)  {
        // TODO check if configuration reqest
        int isConfigurationFacet = false;
        if (isConfigurationFacet) {
            // TODO do config update
            // this->configure(message);
        }
        else {
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
    
}