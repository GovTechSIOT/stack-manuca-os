/**
 * @defgroup decada_manager DECADA Manager
 * @{
 */

#include "decada_manager.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha1.h"
#include "mbed_trace.h"
#include "https_request.h"
#include "json.h"
#include "conversions.h"
#include "crypto_engine.h"
#include "device_uid.h"
#include "persist_store.h"
#include "subscription_callback.h"
#include "time_engine.h"

#undef TRACE_GROUP
#define TRACE_GROUP  "DecadaManager"

/**
 *  @brief      RESTful call to DECADA for issuing a client certificate from the certificate signing request.
 *  @details    The client certificate is required for client authentication over TLS.
 *  @author     Lau Lee Hong, Lee Tze Han
 *  @param      csr       Certificate Signing Request (PEM)
 *  @return     csr_sign_resp struct containing client certificate and serial number
 */
csr_sign_resp DecadaManager::SignCertificateSigningRequest(std::string csr)
{
    const std::string timestamp_ms = MsPaddingIntToString(RawRtcTimeNow());
    const std::string access_token = GetAccessToken();
    const std::string http_post_frame = "actionapply";
    
    Watchdog &watchdog = Watchdog::get_instance();
    watchdog.kick();

    Json::Value message_content;
    message_content["csr"] = csr;
    message_content["validDay"] = 365;
    message_content["issueAuthority"] = "ECC";
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string body = Json::writeString(builder, message_content);
    const char* body_sanitized = (char*)body.c_str();

    /* Sort parameters in ASCII order */
    const std::string parameters = http_post_frame + "deviceKey" + GetDeviceUid() + "orgId" + decada_ou_id_ + "productKey" + decada_product_key_+ body_sanitized;
    const std::string signing_params = access_token + parameters + timestamp_ms + decada_access_secret_;
    const std::string signature = ToLowerCase(CryptoEngine::GenericSHA256Generator(signing_params));

    const std::string request_uri = "/connect-service/v2.0/certificates?action=apply&orgId=" + decada_ou_id_
                                    + "&productKey=" + decada_product_key_
                                    + "&deviceKey=" + GetDeviceUid();
    HttpsRequest* request = new HttpsRequest(network_, ROOT_CA_PEM, HTTP_POST, (api_url_ + request_uri).c_str());
    request->set_header("Content-Type", "application/json;charset=UTF-8");
    request->set_header("apim-accesstoken", access_token);
    request->set_header("apim-signature", signature);
    request->set_header("apim-timestamp", timestamp_ms);

    HttpResponse* response = request->send(body_sanitized, strlen(body_sanitized));

    if (!response) 
    {
        tr_warn("Failed to sign CSR (status code %d)", response->get_status_code());
        delete request;

        return {"invalid", "invalid"};
    }
    else
    {
        std::string res_string = response->get_body_as_string();
        tr_debug("CSR Sign return: %s", res_string.c_str());

        Json::Reader reader;
        Json::Value root;
        Json::Value sub_root;
        reader.parse(res_string, root, false);
        sub_root = root.get("data", "invalid");

        std::string decada_cert = sub_root.get("cert", "invalid").asString();
        std::string decada_cert_serial_number = sub_root.get("certSN", "invalid").asString();

        delete request;

        return {decada_cert, decada_cert_serial_number};
    }
}

/**
 *  @brief      Ensures device has been created in DECADA.
 *  @details    The device secret is returned if the device has been created, otherwise attempts to create device through a RESTful call.
 *  @author     Lau Lee Hong, Lee Tze Han
 */
std::string DecadaManager::CheckDeviceCreation(void)
{
    std::string device_secret = GetDeviceSecret();
    
    /* Create this device in DECADA Cloud as a device entity */
    while (device_secret == "invalid")
    {
        device_secret = CreateDeviceInDecada("core-" + device_uuid);
        if (device_secret != "invalid")
        {
            break;
        }

        ThisThread::sleep_for(500ms);
    }

    tr_info("Device created in DECADA.");

    return device_secret;
}

/**
 *  @brief      Setup network connection to DECADA cloud.
 *  @details    This method will attempt to do dynamic provisioning on-the-fly with DECADA if device is not already provisioned.
 *  @author     Lau Lee Hong, Lee Tze Han
 *  @return     Successful(1)/unsuccessful(0) in establishing conenection to DECADA cloud
 */
bool DecadaManager::Connect(void)
{
    /* Store device secret used to communicate with API */
    device_secret_ = CheckDeviceCreation();

    /* Establish MQTT Connection */
    return ConnectMqttNetwork() && ConnectMqttClient();
}

/**
 *  @brief  Publish payload via MQTT.
 *  @author Lau Lee Hong
 *  @param  topic       MQTT publish topic
 *  @param  payload     Outgoing MQTT message
 *  @return Successful(1)/unsuccessful(0) mqtt publish
 */
bool DecadaManager::Publish(const char* topic, std::string payload)
{   
    stdio_mutex.lock();

    char* mqtt_payload = (char*) payload.c_str();

    /* Publish MQTT message */
    MQTT::Message message;
    message.retained = false;
    message.dup = false;
    message.payload = mqtt_payload;
    message.qos = MQTT::QOS0;
    message.payloadlen = (payload.length());
        
    int rc = mqtt_client_->publish(topic, message);

    stdio_mutex.unlock();
    
    if (rc != MQTT::SUCCESS)
    {
        /* Mosquitto broker is disconnected */ 
        tr_warn("rc from MQTT publish is %d", rc);

        return false;
    }
    else
    {
        tr_debug("MQTT Message published");
    }
    
    return true;
}

/**
 *  @brief  Subscribe to the MQTT topic.
 *  @author Lau Lee Hong
 *  @param  topic   MQTT topic
 *  @return Successful(1)/unsuccessful(0) mqtt subscription service
 */
bool DecadaManager::Subscribe(const char* topic)
{
    sub_topics_.insert(topic);

    int rc = mqtt_client_->subscribe(topic, MQTT::QOS1, SubscriptionMessageArrivalCallback);
    
    if (rc != MQTT::SUCCESS) 
    {
        tr_err("rc from MQTT subscribe is %d", rc);
        
        return false;
    }
    else
    {
        tr_info("MQTT subscription service online");
    }
        
    return true;  
}

/**
 *  @brief  Attempt to re-establish connection to DECADA. 
 *  @author Lau Lee Hong
 *  @return Successful(1)/unsuccessful(0)
 */
bool DecadaManager::Reconnect(void)
{
    return ReconnectMqttService();
}

/// TODO: Test with SE when server upgrade completes
/**
 *  @brief      Renew DECADA Client Certificate.
 *  @details    This method will invoke a software reset; New bootup will connect to DECADA using the renewed certificate.
 *  @author     Lau Lee Hong
 *  @return     true: success, false: failed
 */
bool DecadaManager::RenewCertificate(void)
{
    tr_debug("Renewing SSL Client Certificate");

    const csr_sign_resp sign_resp = RenewClientCertificate();

    if ((sign_resp.cert != "invalid") && (sign_resp.cert_sn != "invalid"))
    {
        WriteClientCertificate(sign_resp.cert);
        WriteClientCertificateSerialNumber(sign_resp.cert_sn);
        
        tr_warn("Client Certificate has been renewed; System will reset.");
        NVIC_SystemReset();

        return true;
    }
    else
    {
        return false;
    }
}

/**
 *  @brief  Returns mqtt stack pointers, to pass to another thread.
 *  @author Lau Lee Hong
 *  @return mqtt connectivity pointers
 */
mqtt_stack* DecadaManager::GetMqttStackPointer(void)
{
    mqtt_stack* stack_ptr = &stack_;

    stack_ptr->mqtt_client_ptr = mqtt_client_ptr_;
    stack_ptr->mqtt_network_ptr = mqtt_network_ptr_;
    stack_ptr->network = network_;
    
    return stack_ptr;
}

/**
 *  @brief      Returns an access token for subsequent REST calls usage using appKey and appSecret.
 *  @details    The token expires 2 hours after creation, thus it is not recommended to be stored/cached in software.
 *  @author     Lau Lee Hong
 *  @return     DECADA REST API access token
 */
std::string DecadaManager::GetAccessToken(void)
{   
    const std::string timestamp_ms = MsPaddingIntToString(RawRtcTimeNow());
    const std::string signing_params = decada_access_key_ + timestamp_ms + decada_access_secret_;
    const std::string signature = ToLowerCase(CryptoEngine::GenericSHA256Generator(signing_params));

    Json::Value message_content;
    message_content["appKey"] = decada_access_key_;
    message_content["encryption"] = signature;
    message_content["timestamp"] = timestamp_ms;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string body = Json::writeString(builder, message_content);
    const char* body_sanitized = (char*)body.c_str();

    const std::string request_uri = "/apim-token-service/v2.0/token/get";
    HttpsRequest* request = new HttpsRequest(network_, ROOT_CA_PEM, HTTP_POST, (api_url_ + request_uri).c_str());
    request->set_header("Content-Type", "application/json;charset=UTF-8");
 
    HttpResponse* response = request->send(body_sanitized, strlen(body_sanitized));

    if (!response) 
    {
        tr_warn("GetAccessToken failed (status code %d)", response->get_status_code());
        delete request;

        return "invalid";  
    }
    else
    {
        std::string res_string = response->get_body_as_string();
        Json::Reader reader;
        Json::Value root;
        Json::Value sub_root;
        reader.parse(res_string, root, false);
        sub_root = root.get("data", "invalid");
        std::string access_token = sub_root.get("accessToken", "invalid").asString();

        delete request;

        return access_token;
    }
}

/**
 *  @brief  RESTful call to get device secret from DECADA.
 *  @author Lau Lee Hong
 *  @return C++ string containing response code from the server
 */
std::string DecadaManager::GetDeviceSecret(void)
{
    const std::string timestamp_ms = MsPaddingIntToString(RawRtcTimeNow());
    const std::string access_token = GetAccessToken();
    const std::string http_get_frame = "actionget";

    /* Sort parameters in ASCII order */ 
    const std::string parameters = http_get_frame + "deviceKey" + GetDeviceUid() + "orgId" + decada_ou_id_ + "productKey" + decada_product_key_;
    const std::string signing_params = access_token + parameters + timestamp_ms + decada_access_secret_;
    const std::string signature = ToLowerCase(CryptoEngine::GenericSHA256Generator(signing_params));

    const std::string request_uri = "/connect-service/v2.1/devices?action=get&orgId=" + decada_ou_id_
                                    + "&productKey=" + decada_product_key_
                                    + "&deviceKey=" + GetDeviceUid();

    HttpsRequest* request = new HttpsRequest(network_, ROOT_CA_PEM, HTTP_GET, (api_url_ + request_uri).c_str());
    request->set_header("apim-accesstoken", access_token);
    request->set_header("apim-signature", signature);
    request->set_header("apim-timestamp", timestamp_ms);

    HttpResponse* response = request->send();

    if (!response) 
    {
        tr_warn("GetDeviceSecret failed (status code %d)", response->get_status_code());
        delete request;

        return "invalid"; 
    }
    else
    {
        std::string res_string = response->get_body_as_string();
        tr_debug("device secret: %s", res_string.c_str());

        Json::Reader reader;
        Json::Value root;
        Json::Value sub_root;
        reader.parse(res_string, root, false);
        sub_root = root.get("data", "invalid");
        std::string device_secret = sub_root.get("deviceSecret", "invalid").asString();

        delete request;

        return device_secret;
    }
 }

/**
 *  @brief  RESTful call to create "register" this device under the product.
 *  @author Lau Lee Hong
 *  @param  default_name  User defined name for their device
 *  @return C++ string containing of random generated characters
 */
std::string DecadaManager::CreateDeviceInDecada(const std::string default_name)
{   
    const std::string timestamp_ms = MsPaddingIntToString(RawRtcTimeNow());
    const std::string access_token = GetAccessToken();
    const std::string http_post_frame = "actioncreate";

    Json::Value international_name;

    Json::Value device_name;
    device_name["defaultValue"] = default_name;
    device_name["i18nValue"] = international_name;

    Json::Value message_content;
    message_content["productKey"] = decada_product_key_;
    message_content["timezone"] = "+08:00";
    message_content["deviceName"] = device_name;
    message_content["deviceKey"] = GetDeviceUid();
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string body = Json::writeString(builder, message_content);
    const char* body_sanitized = (char*)body.c_str();

    /* Sort parameters in ASCII order */
    const std::string parameters = http_post_frame + "orgId" + decada_ou_id_ + body_sanitized;
    const std::string signing_params = access_token + parameters + timestamp_ms + decada_access_secret_;
    const std::string signature = ToLowerCase(CryptoEngine::GenericSHA256Generator(signing_params));

    const std::string request_uri = "/connect-service/v2.1/devices?action=create&orgId=" + decada_ou_id_;
    HttpsRequest* request = new HttpsRequest(network_, ROOT_CA_PEM, HTTP_POST, (api_url_ + request_uri).c_str());
    request->set_header("Content-Type", "application/json;charset=UTF-8");
    request->set_header("apim-accesstoken", access_token);
    request->set_header("apim-signature", signature);
    request->set_header("apim-timestamp", timestamp_ms);
 
    HttpResponse* response = request->send(body_sanitized, strlen(body_sanitized));

    if (!response) 
    {
        tr_warn("CreateDeviceInDecada request failed (status code %d)", response->get_status_code());
        delete request;

        return "invalid";  
    }
    else
    {
        std::string res_string = response->get_body_as_string();
        tr_debug("create device: %s", res_string.c_str());
        Json::Reader reader;
        Json::Value root;
        Json::Value sub_root;
        reader.parse(res_string, root, false);
        sub_root = root.get("data", "invalid");
        std::string device_secret = sub_root.get("deviceSecret", "invalid").asString();

        delete request;

        return device_secret;
    }
}

/**
 *  @brief      RESTful call to renew the existing SSL client certificate with a new certificate.
 *  @details    The client certificate is a key parameter required for establishing a MQTT connection with decada.
 *  @author     Lau Lee Hong
 *  @return     csr_sign_resp struct containing refreshed client certificate and serial number
 */
csr_sign_resp DecadaManager::RenewClientCertificate(void)
{  
    const std::string timestamp_ms = MsPaddingIntToString(RawRtcTimeNow());
    const std::string access_token = GetAccessToken();
    const std::string http_post_frame = "actionrenew";
    
    Watchdog &watchdog = Watchdog::get_instance();
    watchdog.kick();

    Json::Value message_content;
    message_content["certSn"] = StringToInt(ReadClientCertificateSerialNumber());
    message_content["validDay"] = 365;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string body = Json::writeString(builder, message_content);
    const char* body_sanitized = (char*)body.c_str();

    /* Sort parameters in ASCII order */
    const std::string parameters = http_post_frame + "deviceKey" + GetDeviceUid() + "orgId" + decada_ou_id_ + "productKey" + decada_product_key_+ body_sanitized;
    const std::string signing_params = access_token + parameters + timestamp_ms + decada_access_secret_;
    const std::string signature = ToLowerCase(CryptoEngine::GenericSHA256Generator(signing_params));

    const std::string request_uri = "/connect-service/v2.0/certificates?action=renew&orgId=" + decada_ou_id_
                                    + "&productKey=" + decada_product_key_
                                    + "&deviceKey=" + GetDeviceUid();
    HttpsRequest* request = new HttpsRequest(network_, ROOT_CA_PEM, HTTP_POST, (api_url_ + request_uri).c_str());
    request->set_header("Content-Type", "application/json;charset=UTF-8");
    request->set_header("apim-accesstoken", access_token);
    request->set_header("apim-signature", signature);
    request->set_header("apim-timestamp", timestamp_ms);

    HttpResponse* response = request->send(body_sanitized, strlen(body_sanitized));

    if (!response) 
    {
        tr_warn("RenewClientCertificate request failed (status code %d)", response->get_status_code());
        delete request;

        return {"invalid", "invalid"};
    }
    else
    {
        std::string res_string = response->get_body_as_string();
        tr_debug("renew client cert: %s", res_string.c_str());
        Json::Reader reader;
        Json::Value root;
        Json::Value sub_root;
        reader.parse(res_string, root, false);
        sub_root = root.get("data", "invalid");
        std::string renewed_decada_cert = sub_root.get("cert", "invalid").asString();
        std::string renewed_decada_cert_serial_number = sub_root.get("certSN", "invalid").asString();

        delete request;

        return {renewed_decada_cert, renewed_decada_cert_serial_number};
    }
}

/**
 *  @brief  Connect to MQTT network (refer to decada_manager.h to set a different hostname/serverport).
 *  @author Lau Lee Hong, Goh Kok Boon, Lee Tze Han
 *  @return Success of opening socket through MQTTNetwork
 *  @note There is no way to query if there is an MQTT network behind the socket;
 *        this only checks if the socket is successfully opened.
 */
bool DecadaManager::ConnectMqttNetwork(void)
{
    if (!network_)
    {
        tr_error("NetworkInterface NULL");   
    }

    mqtt_network_ = new MQTTNetwork(network_);

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
    int rc = mqtt_network_->connect(broker_ip_.c_str(), mqtt_server_port_, ROOT_CA_PEM,
                                    ReadClientCertificate().c_str(), pk_ctx_);
#else
    int rc = mqtt_network_->connect(broker_ip_.c_str(), mqtt_server_port_, ROOT_CA_PEM,
                                    ReadClientCertificate().c_str(), ReadClientPrivateKey().c_str());
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT

    if (rc != 0)
    {
        tr_err("Failed to set-up socket (rc = %d)", rc);

        return false;
    }
    else
    {
        tr_info("Opened socket on %s:%d", broker_ip_.c_str(), mqtt_server_port_);
    }
    
    return true;
}

/**
 *  @brief  Connect to MQTT broker (eg. Mosquitto).
 *  @author Lau Lee Hong
 *  @return Success of connection to MQTT broker
 */
bool DecadaManager::ConnectMqttClient(void)
{
    if (!mqtt_network_)
    {
        tr_error("MQTTNetwork NULL");   
    }
    const std::string decada_device_key = device_uuid;
    const std::string decada_product_key = MBED_CONF_APP_DECADA_PRODUCT_KEY;
    int rtc_time_ms = RawRtcTimeNow();
    std::string time_now_milli_sec = MsPaddingIntToString(rtc_time_ms);

    std::string sha256_input = "clientId" + decada_device_key + "deviceKey" + decada_device_key + "productKey" + decada_product_key +
     "timestamp" + time_now_milli_sec + device_secret_;

    std::string sha256_output = CryptoEngine::GenericSHA256Generator(sha256_input);

    std::string mqtt_decada_client_id = decada_device_key + "|securemode=2,signmethod=sha256,timestamp=" + time_now_milli_sec + "|";
    std::string mqtt_decada_username = decada_device_key + "&" + decada_product_key;
    std::string mqtt_decada_password = ToLowerCase(sha256_output);
    
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char*)mqtt_decada_client_id.c_str();
    data.username.cstring = (char*)mqtt_decada_username.c_str();
    data.password.cstring = (char*)mqtt_decada_password.c_str();
    data.keepAliveInterval = 3600;      // keep tcp connection open for 60mins

    mqtt_client_ = new MQTT::Client<MQTTNetwork, Countdown>(*mqtt_network_);
    
    int rc = mqtt_client_->connect(data);
    if (rc != MQTT::SUCCESS)
    {
        tr_err("rc from MQTT connect is %d", rc);
     
        return false;
    }
    else
    {
        tr_info("MQTT client successfully connected to broker");
    }
    
    return true;
}

/**
 *  @brief  Disconnect the MQTT network.
 *  @author Lau Lee Hong
 *  @param  mqtt_network Reference to MQTT Network pointer
 */
void DecadaManager::DisconnectMqttNetwork(void)
{
    int rc = mqtt_network_->disconnect();
    if (rc != 0)
    {
        tr_warn("Failed to disconnect from MQTT network. (rc = %d)", rc);   
    }

    delete mqtt_network_;
    mqtt_network_ = NULL;
    
    return;        
}

/**
 *  @brief  Disconnect the MQTT client.
 *  @author Lee Tze Han
 */
void DecadaManager::DisconnectMqttClient(void)
{
    for (auto& sub_topic : sub_topics_)
    {
        int rc = mqtt_client_->unsubscribe(sub_topic.c_str());
        if (rc != 0)
        {
            tr_warn("Failed to unsubscribe to MQTT (rc = %d)", rc);   
        }
        
        rc = mqtt_client_->setMessageHandler(sub_topic.c_str(), 0);
        if (rc != 0)
        {
            tr_warn("Failed to set message handler (rc = %d)", rc); 
        }
    }

    if (mqtt_client_->isConnected())
    {
        int rc = mqtt_client_->disconnect();
        if (rc != 0)
        {
            tr_warn("Failed to disconnect MQTT client (rc = %d)", rc);   
        }
    }

    delete mqtt_client_;
    mqtt_client_ = NULL;

    return;        
}

/**
 *  @brief  Reconnect MQTT network on DECADA.
 *  @author Lau Lee Hong, Ng Tze Yang
 *  @return Successful(1)/unsuccessful(0) MQTT Network Reconnection
 */
bool DecadaManager::ReconnectMqttNetwork(void)
{   
    bool rc = ConnectMqttNetwork();
    if (rc)
    {
        tr_info("Re-established connectivity with MQTT network");
    }
    else
    {
        tr_err("Could not establish connectivity with MQTT network");
    }
    
    return rc; 
}

/**
 *  @brief  Reconnect MQTT client on DECADA.
 *  @author Lau Lee Hong, Lee Tze Han
 *  @return Successful(1)/unsuccessful(0) MQTT Client Reconnection
 */
bool DecadaManager::ReconnectMqttClient(void)
{
    bool connected = ConnectMqttClient();
    bool subscribed = true;
    for (auto& sub_topic : sub_topics_)
    {
        if (!Subscribe(sub_topic.c_str()))
        {
            subscribed = false;
            tr_warn("Failed to subscribe to %s", sub_topic.c_str());
        }
    }
    
    if (connected && subscribed)
    {
        tr_info("Re-established connectivity with MQTT client");

        return true;
    }
    else
    {
        tr_warn("Could not re-establish connectivity with MQTT client");

        return false;
    }
}

/**
 *  @brief  Disconnect and Reconnect MQTT network & client on DECADA.
 *  @author Ng Tze Yang, Lau Lee Hong
 *  @return Successful(1)/unsuccessful(0) MQTT Client Reconnection
 */
bool DecadaManager::ReconnectMqttService(void)
{   
    const uint8_t max_mqtt_failed_reconnections = 5;
    static uint8_t mqtt_failed_reconnections = 0;

    mqtt_mutex.lock();
    DisconnectMqttNetwork();
    DisconnectMqttClient();
    bool network_is_connected = ReconnectMqttNetwork();
    bool client_is_connected = ReconnectMqttClient();
    mqtt_mutex.unlock();
        
    while (!(network_is_connected && client_is_connected))
    {
        network_is_connected = ReconnectMqttNetwork();
        client_is_connected = ReconnectMqttClient();

        mqtt_failed_reconnections++;
        if (mqtt_failed_reconnections == max_mqtt_failed_reconnections)
        {
            NVIC_SystemReset();
        }
    }
    mqtt_failed_reconnections = 0;

    return true;
}

 /** @}*/ 