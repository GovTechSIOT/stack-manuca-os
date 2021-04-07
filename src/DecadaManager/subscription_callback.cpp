/**
 * @defgroup subscription_callback Subscription Callback
 * @{
 */

#include "subscription_callback.h"
#include "mbed_trace.h"
#include "json.h"
#include "global_params.h"
#include "conversions.h"
#include "persist_store.h"
#include "trace_manager.h"
#include "trace_macro.h"

#undef TRACE_GROUP
#define TRACE_GROUP  "SubscriptionCallback"

/**
 *  @brief  Callback when a message has arrived from the broker.
 *  @author Lau Lee Hong, Yap Zi Qi
 *  @param  md  Address of message data
 */
void SubscriptionMessageArrivalCallback(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    std::string incoming_payload = (char*)message.payload;
    
    incoming_payload = incoming_payload.substr(0, message.payloadlen);

    Json::Reader reader;
    Json::Value root;
    reader.parse(incoming_payload, root, false);

    std::vector<std::pair<std::string, std::string>> param_list;
    std::string msg_id = root.get("id", "invalid").asString();

    std::string endpoint_id = root.get("method", "invalid").asString();
    std::string const identifier = "thing.service.";
    std::size_t found = endpoint_id.find(identifier);
    if (found !=std::string::npos)
    {
        endpoint_id.erase(found, found+identifier.length());     // remove first 14 characters "thing.service." in method
    }

    Json::Value params = root.get("params", "invalid");
    if (params != "invalid")
    {
        Json::Value::Members param_keys = params.getMemberNames();
        for (int i=0; i<param_keys.size(); i++)
        {
            std::string name = param_keys[i];
            std::string value = root["params"][name].asString();
            param_list.push_back(make_pair(name, value));
        }
    }

    for (auto& it : param_list)
    {
        tr_info("service identifier: %s, message_id: %s, param: %s, value: %s", endpoint_id.c_str(), msg_id.c_str(), it.first.c_str(), it.second.c_str());

        mqtt_arrived_mail_t *mqtt_arrived_mail = mqtt_arrived_mail_box.try_calloc();
        while (mqtt_arrived_mail == NULL)
        {
            mqtt_arrived_mail = mqtt_arrived_mail_box.try_calloc();
            tr_warn("Memory full. NULL pointer allocated");
            ThisThread::sleep_for(500ms);
        }

        mqtt_arrived_mail->endpoint_id = StringToChar(endpoint_id);
        mqtt_arrived_mail->msg_id = StringToChar(msg_id);
        mqtt_arrived_mail->param = StringToChar(it.first);
        mqtt_arrived_mail->value = StringToChar(it.second);
        mqtt_arrived_mail_box.put(mqtt_arrived_mail);
    }

    return;
}

 /** @}*/