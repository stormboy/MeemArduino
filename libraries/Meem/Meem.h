/**
 * Meem.h - A Meem class.
 * Warren Bloomer
 * http://sugarcoding.net/
 */

#ifndef Meem_h
#define Meem_h

#include <SPI.h>
#include <PubSubClient.h>

//#define MEEM_DEBUG 1


#define EPROM_OFFSET_MEEMID 0
#define EPROM_OFFSET_NAME 16

namespace Meem {

    enum facet_type { binary, linear };
    typedef struct {
        char* name;
        facet_type facetType;
        int direction;
    } FacetDesc;

    #define STR_BUFFER_LENGTH 128
	
    
	static char meem_reg_topic[] = "meem";

	/**
	 * A MQTT Meem
	 */
	class MqttMeem {
		static char charBuffer[STR_BUFFER_LENGTH];
 
	public:
		
		/**
		 * constructor
		 */
		MqttMeem(char* meemUUID, const FacetDesc* facets, int numFacets, void (*inboundFacetCallback)(int /*facetIndex*/, const char* /*payload*/));
        
		void setMeemUUID(const char* uuid) {
                this->meemUUID = meemUUID;
		}
        
		/**
		 * connect to an MQTT server
		 */
		boolean connect( byte ip[], uint16_t port);
        
		/**
		 * Write message on an outbound facet off this device.
         * This published a message to the MQTT sevice with a topic representing the
         * outbound facet.
		 */
		boolean sendToOutboundFacet(int facetIndex, const char* payload);
        
        /**
         * Process incoming messages from the subscription.
         */
		boolean loop();
        
        /**
         * Disconnect from MQTT service
         */
		void disconnect();
        
		/**
         * a handler for receiving MQTT subscription events
         */
		static void inboundMessageCallback(char* topic, uint8_t* payload, unsigned int length);
        
	private:

		static MqttMeem* singleton;
	
		// string representation of the meem's UIUD
		char* meemUUID;			// string representation of meem uuid
        
		int numFacets;
		
		const FacetDesc* facets;
		
		 // generate a new meem uuid
		void newMeemid() {}
	
		// load configuration from EEPROM
		void loadConfig() {}
		
		void handleInboundMessage(char* topic, char* message, unsigned int length);
		
		// a function to send the facet message on
		void (*inboundFacetCallback)(int /*facetIndex*/, const char* /*message*/);
        
		// a function to send confi messages on
		void (*inboundConfig)(char* /* config message*/);

	};

	
} // Meem namespace

#endif	// Meem_h