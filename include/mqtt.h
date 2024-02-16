#include <stdint.h>
#include <stdio.h>
#include "dsd.h"

#ifdef USE_MOSQUITTO
#include <mosquitto.h>
#endif

int mqtt_setup(dsd_opts * opts);
void mqtt_send_position(dsd_opts * opts, char * msg, int msg_length);
