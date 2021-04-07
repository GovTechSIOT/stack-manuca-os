/**
 * @defgroup communications_thread Communication Thread
 * @{
 */

#include <string>
#include <unordered_set>
#include "rtos.h"
#include "threads.h"
#include "mbed_trace.h"
#include "rtos.h"
#include "global_params.h"
#include "conversions.h"
#include "communications_network.h"
#include "decada_manager.h"
#include "persist_store.h"
#include "se_trustx.h"
#include "time_engine.h"

std::string const SENSOR_PUB_TOPIC = std::string("/sys/") + MBED_CONF_APP_DECADA_PRODUCT_KEY + "/" + device_uuid + "/thing/measurepoint/post";
std::string const DECADA_SERVICE_TOPIC = std::string("/sys/") + MBED_CONF_APP_DECADA_PRODUCT_KEY + "/" + device_uuid + "/thing/service/";
std::string const SENSOR_POLL_RATE_TOPIC = DECADA_SERVICE_TOPIC + "sensorpollrate";
std::unordered_set<std::string> subscription_topics = {SENSOR_POLL_RATE_TOPIC};

/* RTOS Sub-thread Initialization */
Thread thread_1_1(osPriorityNormal, OS_STACK_SIZE*3, NULL, "SubscriptionManagerThread");

/* [rtos: thread_1_1] SubscriptionManagerThread */
void subscription_manager_thread(DecadaManager* decada_ptr)
{
    #undef TRACE_GROUP
    #define TRACE_GROUP  "SubscriptionManagerThread"
    
    const chrono::milliseconds submgr_thread_sleep_ms = 1000ms;
    mqtt_stack* stack = decada_ptr->GetMqttStackPointer();

    while (1)
    {   
        event_flags.wait_all(FLAG_MQTT_OK, osWaitForever, false);
        if (*(stack->mqtt_client_ptr) != NULL)
        {
            (*(stack->mqtt_client_ptr))->yield();
            if(!(*(stack->mqtt_client_ptr))->isConnected())
            {
                decada_ptr->Reconnect();
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

    const chrono::milliseconds comms_thread_sleep_ms = 500ms;
    const uint32_t ntp_counter_max = 14400000;      // (comms_thread_sleep_ms)*(ntp_counter_max) = 4 hours 
    Watchdog &watchdog = Watchdog::get_instance();

    NetworkInterface* network = NULL;
    std::string payload = "";

    bool is_network_connected= ConfigNetworkInterface(network);
    while (!is_network_connected)
    {
        tr_info("Network Connection Failed...Retrying...");
        is_network_connected = ConfigNetworkInterface(network);
    }
    watchdog.kick();

    /* Update RTC before first message is sent */
    NTPClient ntp(network);
    UpdateRtc(ntp);
    watchdog.kick();

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
    /* Setup cryptographic utilities */
    TrustX trustx;
    while (!trustx.isReady())
    {
        ThisThread::sleep_for(100ms);
    }

    DecadaManager decada(network, &trustx);
#else
    DecadaManager decada(network);
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
    decada.Connect();
    watchdog.kick();

    /* Signal other threads that MQTT is up */ 
    event_flags.set(FLAG_MQTT_OK);
    
    uint32_t ntp_counter = ntp_counter_max;
    bool pub_ok = true;
    bool inital_ntp_update = false;

    for(auto& subscription_topic : subscription_topics)
    {
        decada.Subscribe(subscription_topic.c_str());
    }

    DecadaManager* decada_ptr = &decada; 
    thread_1_1.start(callback(subscription_manager_thread, decada_ptr));

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
        
        comms_upstream_mail_t *comms_upstream_mail = comms_upstream_mail_box.try_get_for(1ms);
        if (comms_upstream_mail) 
        {
            payload = comms_upstream_mail->payload;
            free(comms_upstream_mail->payload);

            mqtt_mutex.lock();
            pub_ok = decada.Publish(SENSOR_PUB_TOPIC.c_str(), payload);
            mqtt_mutex.unlock();

            comms_upstream_mail_box.free(comms_upstream_mail);
        }

        service_response_mail_t *service_response_mail = service_response_mail_box.try_get_for(1ms);
        if (service_response_mail) 
        {
            payload = service_response_mail->response;
            free(service_response_mail->response);
            std::string service_id = service_response_mail->service_id;
            free(service_response_mail->service_id);
            std::string response_topic = DECADA_SERVICE_TOPIC + service_id + "_reply";
            mqtt_mutex.lock();
            pub_ok = decada.Publish(response_topic.c_str(), payload);
            mqtt_mutex.unlock();

            service_response_mail_box.free(service_response_mail);
        }

        /* MQTT Reconnection: Attempt to reconnect. If fails, restart system. */
        if (pub_ok == false)
        {
            watchdog.kick();
            decada.Reconnect();
        }
        
        watchdog.kick();
        ThisThread::sleep_for(comms_thread_sleep_ms);
    }
}

 /** @}*/