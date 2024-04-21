
#ifndef __AZUREMACROS_H__
#define __AZUREMACROS_H__

// C99 libraries
#include <cstdlib>
#include <string.h>
#include <time.h>

// Libraries for MQTT client and WiFi connection
#include <WiFi.h>
#include <mqtt_client.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

#include "AzIoTSasToken.h"
#include "SerialLogger.h"

// Wifi
#define IOT_CONFIG_WIFI_SSID     "PutYourSSIDHere"
#define IOT_CONFIG_WIFI_PASSWORD "PutYourPasswordHere"

// Azure IoT
#define IOT_CONFIG_IOTHUB_FQDN  "PutYourIoTHubNameHere.azure-devices.net"
#define IOT_CONFIG_DEVICE_ID    "PutYourDeviceIDHere"
#define IOT_CONFIG_DEVICE_KEY   "PutYourDeviceKeyHere"

// Publish 1 message every 2 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 2000

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'. 
#define AZURE_SDK_CLIENT_USER_AGENT "c/" AZ_SDK_VERSION_STRING "(ard;esp32)"

// Utility macros and defines
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov", "ntp1.inrim.it"
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0
#define SAS_TOKEN_DURATION_IN_MINUTES 60
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF   1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

// Translate iot_configs.h defines into variables used by the sample
static const char* ssid            = IOT_CONFIG_WIFI_SSID;
static const char* password        = IOT_CONFIG_WIFI_PASSWORD;
static const char* host            = IOT_CONFIG_IOTHUB_FQDN;
static const char* mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id       = IOT_CONFIG_DEVICE_ID;
static const int mqtt_port         = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;

// Memory allocated for the sample's variables and structures.
static esp_mqtt_client_handle_t mqtt_client;
static az_iot_hub_client        client;
static volatile bool            isConnected;

static char                 mqtt_client_id[128];
static char                 mqtt_username[128];
static char                 mqtt_password[200];
static uint8_t              sas_signature_buffer[256];
static unsigned long        next_telemetry_send_time_ms = 0;
static char                 telemetry_topic[128];
static uint8_t              telemetry_payload[100];
static uint32_t             telemetry_send_count = 0;

#define INCOMING_DATA_BUFFER_SIZE 128
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];

// Auxiliary functions

static AzIoTSasToken sasToken(
    &client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));



#endif // __AZUREMACROS_H__