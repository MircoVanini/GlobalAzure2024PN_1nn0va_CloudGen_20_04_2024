#include "Application.h"
#include "BaseMessage.h"

#include "AzureMacros.h"

using namespace std::placeholders;
using namespace std;

static esp_err_t mqttEventHandler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
        int i, r;

        case MQTT_EVENT_ERROR:
            Logger.Info("MQTT event MQTT_EVENT_ERROR");
            break;

        case MQTT_EVENT_CONNECTED:
            Logger.Info("MQTT event MQTT_EVENT_CONNECTED");
            r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
            if (r == -1)
            {
                Logger.Error("Could not subscribe for cloud-to-device messages.");
            }
            else
            {
                Logger.Info("Subscribed for cloud-to-device messages; message id:"  + String(r));
            }
            isConnected = true;
            break;

        case MQTT_EVENT_DISCONNECTED:
            Logger.Info("MQTT event MQTT_EVENT_DISCONNECTED");
            isConnected = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            Logger.Info("MQTT event MQTT_EVENT_SUBSCRIBED");
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            Logger.Info("MQTT event MQTT_EVENT_UNSUBSCRIBED");
            break;

        case MQTT_EVENT_PUBLISHED:
            Logger.Info("MQTT event MQTT_EVENT_PUBLISHED");
            break;

        case MQTT_EVENT_DATA:
            Logger.Info("MQTT event MQTT_EVENT_DATA");
            for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->topic_len; i++)
            {
                incoming_data[i] = event->topic[i]; 
            }
            incoming_data[i] = '\0';
            Logger.Info("Topic: " + String(incoming_data));

            for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->data_len; i++)
            {
                incoming_data[i] = event->data[i]; 
            }
            incoming_data[i] = '\0';
            Logger.Info("Data: " + String(incoming_data));
            break;

        case MQTT_EVENT_BEFORE_CONNECT:
            Logger.Info("MQTT event MQTT_EVENT_BEFORE_CONNECT");
            break;

        default:
            Logger.Error("MQTT event UNKNOWN");
            break;
    }

    return ESP_OK;
}

Application::Application()
{
    _pDistanceSensor = new UltraSonicDistanceSensor((byte)23, (byte)22, 50); 
    _pStepper        = new CheapStepper(27, 26, 25, 33);     
}

Application::~Application()
{
    if(_pDistanceSensor != NULL)
    {
        delete _pDistanceSensor;
        _pDistanceSensor = NULL;
    }
}

void Application::init(void)
{
    _proximityTask.set(100, TASK_FOREVER, std::bind(&Application::readProximityCallback, this));
    _queueTask.set(100, TASK_FOREVER, std::bind(&Application::queueCallback, this));
    _moveTask.set(10, TASK_ONCE, std::bind(&Application::moveCallback, this));

    _scheduler.init();
    _scheduler.addTask(_proximityTask);
    _scheduler.addTask(_queueTask);
    _scheduler.addTask(_moveTask);

    _proximityTask.enableDelayed(100);
    _queueTask.enableDelayed(100);

    _pStepper->setRpm(24);

    _deviceId      = Common::GetDeiceId();
    _openDebounce  = 0;
    _closeDebounce = 0;

    establishConnection();
}

void Application::loop() 
{  
    _scheduler.execute();
    _pStepper->run();    

    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }
    else if (sasToken.IsExpired())
    {
        Logger.Info("SAS token expired; reconnecting with a new one.");
        (void)esp_mqtt_client_destroy(mqtt_client);
        initializeMqttClient();
    }
}

void Application::readProximityCallback(void)
{
    try
    {
        float distance = _pDistanceSensor->measureDistanceCm();

        if (distance < 5.0 && distance > 1)
        {
            if (_openDebounce++ < DEBOUCE_OK)
                return;
            
            if (!_isDoorOpen)
            {
                sprintf(_buffer, "Open the doore -> distance: %f", distance);
                Logger.PrintLn(_buffer);

                BaseMessage msg;
                msg.setEvent(BaseMessage::eventType::openDoor, _deviceId, String(MAIN_DOOR));
                _eventQueue.push(msg.serialize());    

                _isDoorOpen    = true;
                _moveTask.restart();

                _openDebounce  = 0;
                _closeDebounce = 0;
            }
        }
        else
        {
            if (_closeDebounce++ < DEBOUCE_OK)
                return;

            if (_isDoorOpen)
            {
                Logger.PrintLn("Close the door");

                BaseMessage msg;
                msg.setEvent(BaseMessage::eventType::closeDoor, _deviceId, String(MAIN_DOOR));
                _eventQueue.push(msg.serialize());    

                _isDoorOpen   = false;
                _moveTask.restart();

                _openDebounce  = 0;
                _closeDebounce = 0;
            }
        }    
    }
    catch(const std::exception& e)
    {
        Logger.Error(e.what());
    }
}

void Application::moveCallback(void)
{
    try
    {
        Logger.PrintLn(">>> moveCallback");

        int steps = 2048  - abs(_pStepper->getStepsLeft());
        sprintf(_buffer, "steps: %ld", steps);
        Logger.PrintLn(_buffer);

        _pStepper->stop();
        _pStepper->newMove(_isDoorOpen, steps);

        Logger.PrintLn("<< moveCallback");
    }
    catch(const std::exception& e)
    {
        Logger.Error(e.what());
    }
}

void Application::queueCallback(void)
{
    try
    {    
        if (_eventQueue.size() == 0)
            return;

        Logger.PrintLn(">>> queueCallback");

        while (_eventQueue.size() > 0)
        {
            auto item = _eventQueue.front();
            _eventQueue.pop();
            
            Serial.write(item.c_str());
            Serial.write("\n");

            sendTelemetry(item);
        }

        Logger.PrintLn("<<< queueCallback");
    }
    catch(const std::exception& e)
    {
        Logger.Error(e.what());
    }
}

void Application::establishConnection(void)
{
    try
    {
        connectToWiFi();
        initializeTime();
        initializeIoTHubClient();
        (void)initializeMqttClient();
    }
    catch(const std::exception& e)
    {
        Logger.Error(e.what());
    }    
}

void Application::connectToWiFi(void)
{
    Logger.Info("Connecting to WIFI SSID " + String(ssid));

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Logger.Print(".");
    }

    Logger.PrintLn("");
    Logger.Info("WiFi connected, IP address: " + WiFi.localIP().toString());
}

void Application::initializeTime(void)
{
    Logger.Info("Setting time using SNTP");

    configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
    time_t now = time(NULL);

    while (now < UNIX_TIME_NOV_13_2017)
    {
        delay(500);
        Logger.Print(".");
        now = time(nullptr);
    }
    
    Logger.PrintLn("");
    Logger.Info("Time initialized!");
}

void Application::initializeIoTHubClient(void)
{
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

    if (az_result_failed(az_iot_hub_client_init(&client,
                                                az_span_create((uint8_t*)host, strlen(host)),
                                                az_span_create((uint8_t*)device_id, strlen(device_id)),
                                                &options)))
    {
        Logger.Error("Failed initializing Azure IoT Hub client");
        return;
    }

    size_t client_id_length;
    if (az_result_failed(az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
    {
        Logger.Error("Failed getting client id");
        return;
    }

    if (az_result_failed(az_iot_hub_client_get_user_name(&client, mqtt_username, sizeofarray(mqtt_username), NULL)))
    {
        Logger.Error("Failed to get MQTT clientId, return code");
        return;
    }

    Logger.Info("Client ID: " + String(mqtt_client_id));
    Logger.Info("Username: " + String(mqtt_username));
}

int Application::initializeMqttClient(void)
{
    if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
    {
        Logger.Error("Failed generating SAS token");
        return 1;
    }

    esp_mqtt_client_config_t mqtt_config;
    memset(&mqtt_config, 0, sizeof(mqtt_config));

    mqtt_config.uri                    = mqtt_broker_uri;
    mqtt_config.port                   = mqtt_port;
    mqtt_config.client_id              = mqtt_client_id;
    mqtt_config.username               = mqtt_username;
    mqtt_config.password               = (const char*)az_span_ptr(sasToken.Get());
    mqtt_config.keepalive              = 30;
    mqtt_config.disable_clean_session  = 0;
    mqtt_config.disable_auto_reconnect = false;
    mqtt_config.event_handle           = mqttEventHandler;
    mqtt_config.user_context           = NULL;
    mqtt_config.cert_pem               = (const char*)ca_pem;

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL)
    {
        Logger.Error("Failed creating mqtt client");
        return 1;
    }

    esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

    if (start_result != ESP_OK)
    {
        Logger.Error("Could not start mqtt client; error code:" + start_result);
        return 1;
    }
    else
    {
        Logger.Info("MQTT client started");
        return 0;
    }
}

void Application::sendTelemetry(String sentense)
{
    Logger.Info("Sending telemetry ...");

    if (!isConnected)
    {
        Logger.Info("Skip sending telemetry, the connection is down !");
        return;
    }

    // The topic could be obtained just once during setup,
    // however if properties are used the topic need to be generated again to reflect the
    // current values of the properties.
    if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(&client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
    {
        Logger.Error("Failed az_iot_hub_client_telemetry_get_publish_topic");
        return;
    }

    az_span telemetry = az_span_create_from_str((char *)(sentense.c_str()));

    if (esp_mqtt_client_publish(mqtt_client, 
                                telemetry_topic, 
                                (const char*)az_span_ptr(telemetry),
                                az_span_size(telemetry),
                                MQTT_QOS1,
                                DO_NOT_RETAIN_MSG) == 0)
    {
        Logger.Error("Failed publishing");
    }
    else
    {
        Logger.Info("Message published successfully");
    }
}