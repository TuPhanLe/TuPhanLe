
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp32/rom/rtc.h"
#include "esp32/rom/crc.h"
#include "sys/time.h"
#include "sdkconfig.h"
#include <time.h>

#include "app_main.h"
#include "util.h"
#include "bluetooth.h"
#include "mqtt.h"
#include "spiffs.h"
#include "rtc_pcf8563.h"

#define TAG_rtc "RTC"

#if 1 /* GLobal varible */

    /* Data MQTT CLOUD */
    int led = 0;
	int alarmLed = 0;
	int startHour = 0;
	int startMinute = 0;
	int endHour = 0;
	int endMinute = 0;

    unsigned char glFlagTimer = 0;
    unsigned char datachange = 0;

    /* test alarm*/
    unsigned char first_Initialization = 1;
    unsigned char test = 1;
    unsigned char suspend = 0;
    TaskHandle_t xHandle;
    
    /* Time for alarm */
	struct timeAlarm {
		unsigned char Hours;
		unsigned char Minutes;
    };
    struct timeAlarm timeNow;
    time_t now;
    struct tm timeinfo;

    // Data upload 

    /* timer loop 10ms */
    
	unsigned char statusLed = 0;
	unsigned char statusAlarm = 0;
	unsigned char statusWifi = 0;
    char wifi;
    #define onLed 1
    #define offLed 0
    
#endif

#if 1
    /* setting RTC */
    void settupRTC(void);

	/* Set power Pump */
	void setPowerLed(unsigned char mode);
		
	/* Check data */
	void queryData();
		
	/* Alarm Pump */
	void setTimeRTC(int hour, int minute);
	void getTime();
	unsigned char alarmRTC(int hour, int minute);

    void publishMQTT(char *topic,char *data[]);

    void updateCloudMQTT(void);
    void clearData();

    void operate();

    void setup(void);
#endif






/*void timer_callback(void *param)
{
    glFlagtimer10ms = 1;
}*/

///////////////////////////////////////////////////////////////
/* Check Data */
void queryData()
{
	/* off led */
	if(led == 0 && alarmLed == 0){
		setPowerLed(offLed);
		statusLed = 0;
		statusAlarm = 0;
		//glResetTimerFlag = 0;
		//clearData();
  	}
	else if(led == 0 && alarmLed == 1 && statusAlarm == 0){
    	setPowerLed(onLed);
		statusLed = 1;
		glFlagTimer = 0;

		// Start alarm
		setTimeRTC(startHour,startMinute);
	  	statusAlarm = 1;
        printf(" startAlarm \r\n ");	
	}
	else if(led == 1 && alarmLed == 0 && statusAlarm == 0){
		setPowerLed(onLed);
		statusLed = 1;
		//glResetTimerFlag = 0;
		
		// Limit time Led = 20 minute 
		//setLimitedTime();
		//setTime(sHour,sMinute);
		//statusAlarm = 1;
		//glLimitedTimeFlag = 1;
	}
	/*else if(led == 1 && alarmLed == 1 && statusAlarm == 0){
		setPowerLed(onLed);
		statusLed = 1;
		//glResetTimerFlag = 0;

		// Start alarm
		setTimeRTC(startHour,startMinute);
		statusAlarm = 1;
	}*/
		
}
////////////////////////////////////////////////////////////

/* set Power Pump */
void setPowerLed(unsigned char mode)
{
	gpio_set_level(23,mode);
}

void controlRTC(void *pvParameter)
{
    while(1){
        if(statusAlarm == 1){
            //printf("alarming\r\n");
            statusAlarm = alarmRTC(endHour,endMinute);

            if(statusAlarm == 0){
                // turn Off Led
                setPowerLed(offLed);
                statusLed = 0;
                //glLimitedTimeFlag = 0;
                
                /* update led state */
                updateCloudMQTT();
                clearData();
                printf(" finnish alarm ");
                suspend = 1;
		    }
		    // continute alarm-------->
        }
        else{
            printf("No alarming");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

#if 0
void setupRTC(void)
{
    esp_err_t ret;
	struct tm default_time = {
        .tm_year = 2023 - 1900,
        .tm_mon = 3, // January
        .tm_mday = 10,
        .tm_hour = ....,
        .tm_min = .....,
        .tm_sec = 0
    };
    time_t default_time_secs = mktime(&default_time);
    struct timeval tv = {
        .tv_sec = default_time_secs,
        .tv_usec = 0
    };
    printf("Setting default time to %ld\n", default_time_secs);
    settimeofday(&tv, NULL);

    // Initialize the RTC GPIO
    printf("Initializing RTC GPIO\n");
    ret = rtc_gpio_init(GPIO_NUM_26);
    if (ret != ESP_OK) {
        printf("Error initializing RTC GPIO: %d\n", ret);
        //return;
    }
    ret = rtc_gpio_set_direction(GPIO_NUM_26, RTC_GPIO_MODE_INPUT_ONLY);
    if (ret != ESP_OK) {
        printf("Error setting RTC GPIO direction: %d\n", ret);
        //return;
    }
}
#endif
////////////////////////////////////////////////////////////
/* Alarm Pump */
void setTimeRTC(int hour, int minute)
{   
    esp_err_t ret;
	struct tm default_time = {
        .tm_year = 2023 - 1900,
        .tm_mon = 3, // January
        .tm_mday = 10,
        .tm_hour = hour,
        .tm_min = minute,
        .tm_sec = 0
    };
    time_t default_time_secs = mktime(&default_time);
    struct timeval tv = {
        .tv_sec = default_time_secs,
        .tv_usec = 0
    };
    //printf("Setting default time to %ld\n", default_time_secs);
    settimeofday(&tv, NULL);

    // Initialize the RTC GPIO
    //printf("Initializing RTC GPIO\n");
    ret = rtc_gpio_init(GPIO_NUM_26);
    if (ret != ESP_OK) {
        printf("Error initializing RTC GPIO: %d\n", ret);
        //return;
    }
    ret = rtc_gpio_set_direction(GPIO_NUM_26, RTC_GPIO_MODE_INPUT_ONLY);
    if (ret != ESP_OK) {
        printf("Error setting RTC GPIO direction: %d\n", ret);
        //return;
    }
    printf("set alarm success \r\n");
}

void getTime()
{
    time(&now);
    localtime_r(&now, &timeinfo);
	// get time now 
	timeNow.Hours = timeinfo.tm_hour;
	timeNow.Minutes = timeinfo.tm_min;
}

unsigned char alarmRTC(int hour, int minute)
{
	unsigned char flag;

	//Read Time from RTC
	getTime();
    printf("%d %d \r\n", timeNow.Hours, timeNow.Minutes); 

	if(timeNow.Hours == hour){
		if(timeNow.Minutes < minute){
			flag = 1;     // continnue alarm
		}
		else{
			flag = 0;     // stop alarm
		}
	}
	else{
		flag = 1;         // continnue alarm
	}
	return flag;
}
////////////////////////////////////////////////////////////


void mainTask(void *pvParameter)
{
    while(1){
        /* update connect Esp to Wifi state */
        // if(statusWifi == 0){
        //     publishMQTT("statusWifi","0");
        //     statusWifi = 1;
        // }
        // else{
        //     publishMQTT("statusWifi","1");
        //     statusWifi = 0;
        // }

        /* incoming data from MQTT cloud */
        if(datachange == 6){

            /* Process Data */
            queryData();

            if(statusAlarm == 1 && glFlagTimer == 0){
                /* start task alarm */
                if(first_Initialization == 1){
                    xTaskCreate(&controlRTC, "controlRTC", 2048, NULL, 4, &xHandle);
                    printf("create task control \r\n");
                    first_Initialization = 0;
                }
                else{
                    vTaskResume(xHandle);
                    printf("Resume\r\n");
                }
                glFlagTimer = 1;
            }
            datachange = 0;
            updateCloudMQTT();
        }
        
        /* suspend when no need RTC */
        if(suspend == 1){
            suspend = 0;
            vTaskSuspend(xHandle);
            printf("suspend done\r\n");
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void updateCloudMQTT(void)
{
    /* Updata state led */
    // if(statusLed == 1){
    //     publishMQTT("statusLed","1");
    // }
    // else{
    //     publishMQTT("statusLed","0");
    // }
}

void clearData(void)
{
    led = 0;
    alarmLed = 0;
    startHour = 0;
    startMinute = 0;
    endHour = 0;
    endMinute = 0;
    datachange = 0;
}

void setup(void)
{
    /* Set gpio output for control relay */
    gpio_pad_select_gpio(23);
    gpio_set_direction(23,GPIO_MODE_OUTPUT);
   
    clearData();
}

void operate(void)
{
    char file_name[30] = "wifi.txt";
    char file_name1[30] = "schedule.txt";
    char write_buffer[100] = "SSID: windpink PASSWORD: 11111111 \r\n";
    char write_buffer1[100] = "SSID: windPink PASSWORD: 11111111 \r\n";
    char write_buffer2[100] = "SSID: Windpink PASSWORD: 11111111 \r\n";
    char write_buffer3[100] = "SSID: Windpink PASSWORD: 11111111 \r\n";
    char read_buffer[100];

    //save_file_flash(file_name, sizeof(file_name), write_buffer, strlen(write_buffer), WRITE_ONLY);

    memset(read_buffer,NULL,sizeof(read_buffer));
    read_file_flash(file_name, sizeof(file_name), read_buffer, sizeof(read_buffer), READ_ONLY);
    //printf("%s",read_buffer);
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    

    init_spiffs();
    rtc_pcf();
    //operate();

    //setup();
    //ble_handle();
    //mqtt_init();
    
    
    
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */

    // char string[] = "0001000100010001000100";
    // uint8_t data_to_hex[11];
    // String_to_Hex(string,strlen(string),data_to_hex);
    // for (uint8_t i = 0; i < 11; i++)
    // {
    //     printf("test util lib: %d ",data_to_hex[i]);
    // }
    // printf("\r\n");
    
    //spiffs();
    //xTaskCreate(&mainTask, "mainTask", 2048, NULL, 5, NULL);

}


