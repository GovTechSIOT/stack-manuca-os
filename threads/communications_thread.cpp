/**
 * @defgroup communications_thread Communication Thread
 * @{
 */

#include <string>
#include "threads.h"
#include "global_params.h"
#include "watchdog.h"
#include "conversions.h"
#include "decada_manager.h"
#include "persist_store.h"
#include "time_engine.h"
#include "mbed_trace.h"

std::string device_secret, cert, decada_root_ca;
std::string const SENSOR_PUB_TOPIC = std::string("/sys/") + MBED_CONF_APP_DECADA_PRODUCT_KEY + "/" + device_uuid + "/thing/measurepoint/post";
std::string const DECADA_SERVICE_TOPIC = std::string("/sys/") + MBED_CONF_APP_DECADA_PRODUCT_KEY + "/" + device_uuid + "/thing/service/";
std::string const SENSOR_POLL_RATE_TOPIC = DECADA_SERVICE_TOPIC + "sensorpollrate";
std::unordered_set<std::string> subscription_topics = {SENSOR_POLL_RATE_TOPIC};

/* RTOS Sub-thread Initialization */
Thread thread_1_1(osPriorityNormal, OS_STACK_SIZE*3, NULL, "SubscriptionManagerThread");

/* [rtos: thread_1_1] SubscriptionManagerThread */
void subscription_manager_thread(mqtt_stack* stack) 
{
    #undef TRACE_GROUP
    #define TRACE_GROUP  "SubscriptionManagerThread"
    
    const uint32_t submgr_thread_sleep_ms = 10000;
    while (1)
    {   
        event_flags.wait_all(FLAG_MQTT_OK, osWaitForever, false);
        if (*(stack->mqtt_client_ptr) != NULL)
        {
            (*(stack->mqtt_client_ptr))->yield();
            if(!(*(stack->mqtt_client_ptr))->isConnected())
            {
                ReconnectMqttService(stack->network,*(stack->mqtt_network_ptr),*(stack->mqtt_client_ptr), device_secret, subscription_topics, decada_root_ca, cert, ReadSSLPrivateKey());
            }
        }
        ThisThread::sleep_for(submgr_thread_sleep_ms);
    }
}

/* [rtos: thread_1] CommunicationsControllerThread */
void communications_controller_thread(void) 
{
    #undef TRACE_GROUP
    #define TRACE_GROUP  "CommunicationsControllerThread"
    
    const uint32_t comms_thread_sleep_ms = 500;
    const uint32_t ntp_counter_max = 14400000;      // (comms_thread_sleep_ms)*(ntp_counter_max) = 4 hours 

    NetworkInterface* network = NULL;
    MQTTNetwork* mqtt_network = NULL;
    MQTTNetwork** mqtt_network_ptr = &mqtt_network;
    MQTT::Client<MQTTNetwork, Countdown>* mqtt_client = NULL;
    MQTT::Client<MQTTNetwork, Countdown>** mqtt_client_ptr  = &mqtt_client;

    std::string payload = "";

    bool is_network_connected= ConfigNetworkInterface(network);
    while (!is_network_connected)
    {
        printf("Network Connection Failed...Retrying...\r\n");
        is_network_connected = ConfigNetworkInterface(network);
    }
    
    /* Update RTC before first message is sent */
    NTPClient ntp(network);
    UpdateRtc(ntp);

    /* Get DECADA root CA cert */
    decada_root_ca = GetDecadaRootCA(network);
    tr_info("Root CA is retrieved from decada.");

    device_secret = CheckDeviceRegistrationStatus(network);
    bool first_registration_attempt = true;
    while (device_secret == "invalid")
    {
        if(!first_registration_attempt)
        {
            ThisThread::sleep_for(500);
        }
        tr_info("Registering Device to DECADA...");
        device_secret = RegisterDeviceToDecada(network, device_uuid);       // Change `device_uuid` to whatever you woud like the device to be labelled on DECADA Cloud dashboard

        first_registration_attempt = false;
    }
    tr_info("Device is registered with DECADA.");

    wd.Service(); 

    cert = ReadClientCertificate();
    if (cert == "" || cert == "invalid" || ReadSSLPrivateKey() == "")
    {
        tr_info("Requesting client certificate from DECADA...");
        cert = ApplyMqttCertificate(network, decada_root_ca);
        WriteClientCertificate(cert);
        tr_info("New client certificate is generated.");
    }
    else
    {
        tr_info("Using existing client certificate.");
    }

    /* Connect to MQTT */
    ConnectMqttNetwork(mqtt_network, network, decada_root_ca, cert, ReadSSLPrivateKey());
    ConnectMqttClient(mqtt_client, mqtt_network, device_secret);

    wd.Service();
    
    mqtt_stack stack;
    mqtt_stack* stack_ptr = &stack;
    stack_ptr->mqtt_client_ptr = mqtt_client_ptr;
    stack_ptr->mqtt_network_ptr = mqtt_network_ptr;
    stack_ptr->network = network;

    /* Signal other threads that MQTT is up */ 
    event_flags.set(FLAG_MQTT_OK);
    
    bool pub_ok = true;
    uint8_t ntp_counter = ntp_counter_max;
    bool inital_ntp_update = false;

    for(auto& sub_topic : subscription_topics)
    {
        MqttSubscribe(mqtt_client, sub_topic.c_str());
    }
    
    thread_1_1.start(callback(subscription_manager_thread, stack_ptr));

    while (1)
    {     
        /* Update HW RTC with NTP Cloud Time */
        if (ntp_counter == ntp_counter_max)
        {
            bool success = UpdateRtc(ntp);
            if (success || inital_ntp_update)
            {
                ntp_counter = 0;
                inital_ntp_update = true;
            }
        }
        else
        {
            ntp_counter++;
        }
        
        osEvent evt = comms_upstream_mail_box.get(1);
        if (evt.status == osEventMail) 
        {
            comms_upstream_mail_t *comms_upstream_mail = (comms_upstream_mail_t*) evt.value.p;
            payload = comms_upstream_mail->payload;
            free(comms_upstream_mail->payload);

            mqtt_mutex.lock();
            pub_ok = MqttPublish(mqtt_client, SENSOR_PUB_TOPIC.c_str(), payload);
            mqtt_mutex.unlock();

            comms_upstream_mail_box.free(comms_upstream_mail);
        }

        evt = service_response_mail_box.get(1);
        if (evt.status == osEventMail) 
        {
            service_response_mail_t *service_response_mail = (service_response_mail_t*) evt.value.p;
            payload = service_response_mail->response;
            free(service_response_mail->response);
            std::string service_id = service_response_mail->service_id;
            free(service_response_mail->service_id);
            std::string response_topic = DECADA_SERVICE_TOPIC + service_id + "_reply";
            mqtt_mutex.lock();
            pub_ok = MqttPublish(mqtt_client, response_topic.c_str(), payload);
            mqtt_mutex.unlock();

            service_response_mail_box.free(service_response_mail);
        }

        /* MQTT Reconnection: Attempt to reconnect. If fails, restart system. */
        if (pub_ok == false)
        {
            wd.Service();
            ReconnectMqttService(network,mqtt_network, mqtt_client, device_secret, subscription_topics, decada_root_ca, cert, ReadSSLPrivateKey());
        }
        
        wd.Service();
        ThisThread::sleep_for(comms_thread_sleep_ms);
    }
}

 /** @}*/