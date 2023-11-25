#include <stdint.h>
#include <stdio.h>
#include "dsd.h"

#ifdef USE_MOSQUITTO
#include <mosquitto.h>
#endif

int mqtt_setup(dsd_opts * opts);
//void mqtt_send(dsd_opts * opts, uint8_t slot, uint8_t bytes[], uint8_t pdu_len);
void mqtt_send_position(dsd_opts * opts, char * msg, int msg_length);

