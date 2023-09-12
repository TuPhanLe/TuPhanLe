#ifndef _MQTT
#define _MQTT


static void mqtt_app_start(void);

void publishMQTT(char *topic,char *data[]);
void mqtt_init();
#endif