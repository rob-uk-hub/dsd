#include "mqtt.h"

#ifdef USE_MOSQUITTO
#include <mqtt_protocol.h>

struct mosquitto * mosq_mqtt;

void mqtt_disconnect()
{
    mosquitto_disconnect(mosq_mqtt);
}

int mqtt_connect(dsd_opts * opts)
{
    int rc;

    mqtt_disconnect();

    if(sizeof (opts->mqtt_username) > 0)
    {
        mosquitto_username_pw_set(mosq_mqtt, opts->mqtt_username, opts->mqtt_password);
    } else {
        mosquitto_username_pw_set(mosq_mqtt, NULL, NULL);
    }

    int port = opts->mqtt_broker_port == 0 ? 1883 : opts->mqtt_broker_port;
    const int keep_alive = 15;
    rc = mosquitto_connect(mosq_mqtt, opts->mqtt_broker_address, port, keep_alive);
    if(rc != 0){
        fprintf(stderr, "Client could not connect to broker! Error Code: %d\n", rc);
        mqtt_disconnect();
        return 1;
    }
    fprintf(stderr, "Connected to the MQTT broker, data will be sent to it.\n");

    rc = mosquitto_loop_start(mosq_mqtt);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}

    return 0;
}

int mqtt_setup(dsd_opts * opts)
{
    if(opts->mqtt_broker_address[0] == 0) 
    {
        fprintf(stderr, "MQTT Disabled\n");

        return 0;
    }

    fprintf(stderr, "Connecting to MQTT\n");

    mosquitto_lib_init();

    mosq_mqtt = mosquitto_new(NULL, true, NULL);
    //mosquitto_int_option(mosq_mqtt, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

    return mqtt_connect(opts);
}

void mqtt_distroy()
{
    mosquitto_destroy(mosq_mqtt);

    mosquitto_lib_cleanup();
}

void mqtt_send(dsd_opts * opts, char* topic, char* payload, int length)
{
    if(opts->mqtt_broker_address[0] == 0)  return;

    // TODO - should a unqiue topic be used for each source together with the retain option?
    fprintf(stderr, "\nSending %d bytes to MQTT topic %s...\n", length, topic);
    int rc = mosquitto_publish(mosq_mqtt, NULL, topic, length, payload, 1, false); 
    if(rc == MOSQ_ERR_NO_CONN || rc == MOSQ_ERR_ERRNO)
    {
        mqtt_connect(opts);

        fprintf(stderr, "Resending due to a MQTT connection error %d\n", rc);
        rc = mosquitto_publish(mosq_mqtt, NULL, topic, length, payload, 1, false); 
    } 

    if (rc != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "Error sending message to MQTT, result = %d\n", rc);
        mqtt_connect(opts);
    }
}

void mqtt_send_position(dsd_opts * opts, char * msg, int msg_length) {
    if(opts->mqtt_broker_address[0] == 0)  return;

    char *topic = opts->mqtt_position_topic;
    if(topic == 0) return;

    mqtt_send(opts, topic, msg, msg_length);
}

#else

int mqtt_setup(dsd_opts * opts)
{
    return 0;
}

void mqtt_send_position(dsd_opts * opts, char * msg, int msg_length)
{
}

#endif