/*******************************************************************************************************
 * Copyright (c) 2020 Government Technology Agency of Singapore (GovTech)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 *
 * See the License for the specific language governing permissions and limitations under the License.
 *******************************************************************************************************/
#ifndef DECADA_MANAGER_H
#define DECADA_MANAGER_H

#include <string>
#include <unordered_set>
#include "global_params.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "crypto_engine.h"

/** DecadaManager class.
 *  @brief  MQTT instance that communicates with DECADA Cloud over TLS, and provisioned via dynamic activation.
 *
 *  Example:
 *  @code{.cpp}
 *  #include "mbed.h"
 *  #include "decada_manager.h"
 *
 *  int main() 
 *  {
 *      std::string pub_topic = "/publish/1"; 
 *      std::string sub_topic = "/subscribe/2";
 *      std::string pub_msg = "{hello-manuca}";
 *
 *      NetworkInterface* network = NULL;
 *      bool network_connected= ConfigNetworkInterface(network);
 *
 *      if (network_connected)
 *      {
 *          DecadaManager decada(network);
 *          decada.Connect();
 *          decada.Subscribe(sub_topic.c_str());
 *          decada.Publish(pub_topic.c_str(), pub_msg);
 *      }
 *  }
 *  @endcode
 */

/* List of trusted Root CA Certificates
 * For DecadaManager: Sectigo Root CA
 *
 * To add more root certificates, just concatenate them.
 */
const char ROOT_CA_PEM[] =  
    "-----BEGIN CERTIFICATE-----\n"
    "MIIGGTCCBAGgAwIBAgIQE31TnKp8MamkM3AZaIR6jTANBgkqhkiG9w0BAQwFADCB\n"
    "iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n"
    "cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n"
    "BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTgx\n"
    "MTAyMDAwMDAwWhcNMzAxMjMxMjM1OTU5WjCBlTELMAkGA1UEBhMCR0IxGzAZBgNV\n"
    "BAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UE\n"
    "ChMPU2VjdGlnbyBMaW1pdGVkMT0wOwYDVQQDEzRTZWN0aWdvIFJTQSBPcmdhbml6\n"
    "YXRpb24gVmFsaWRhdGlvbiBTZWN1cmUgU2VydmVyIENBMIIBIjANBgkqhkiG9w0B\n"
    "AQEFAAOCAQ8AMIIBCgKCAQEAnJMCRkVKUkiS/FeN+S3qU76zLNXYqKXsW2kDwB0Q\n"
    "9lkz3v4HSKjojHpnSvH1jcM3ZtAykffEnQRgxLVK4oOLp64m1F06XvjRFnG7ir1x\n"
    "on3IzqJgJLBSoDpFUd54k2xiYPHkVpy3O/c8Vdjf1XoxfDV/ElFw4Sy+BKzL+k/h\n"
    "fGVqwECn2XylY4QZ4ffK76q06Fha2ZnjJt+OErK43DOyNtoUHZZYQkBuCyKFHFEi\n"
    "rsTIBkVtkuZntxkj5Ng2a4XQf8dS48+wdQHgibSov4o2TqPgbOuEQc6lL0giE5dQ\n"
    "YkUeCaXMn2xXcEAG2yDoG9bzk4unMp63RBUJ16/9fAEc2wIDAQABo4IBbjCCAWow\n"
    "HwYDVR0jBBgwFoAUU3m/WqorSs9UgOHYm8Cd8rIDZsswHQYDVR0OBBYEFBfZ1iUn\n"
    "Z/kxwklD2TA2RIxsqU/rMA4GA1UdDwEB/wQEAwIBhjASBgNVHRMBAf8ECDAGAQH/\n"
    "AgEAMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAbBgNVHSAEFDASMAYG\n"
    "BFUdIAAwCAYGZ4EMAQICMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNl\n"
    "cnRydXN0LmNvbS9VU0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNy\n"
    "bDB2BggrBgEFBQcBAQRqMGgwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRy\n"
    "dXN0LmNvbS9VU0VSVHJ1c3RSU0FBZGRUcnVzdENBLmNydDAlBggrBgEFBQcwAYYZ\n"
    "aHR0cDovL29jc3AudXNlcnRydXN0LmNvbTANBgkqhkiG9w0BAQwFAAOCAgEAThNA\n"
    "lsnD5m5bwOO69Bfhrgkfyb/LDCUW8nNTs3Yat6tIBtbNAHwgRUNFbBZaGxNh10m6\n"
    "pAKkrOjOzi3JKnSj3N6uq9BoNviRrzwB93fVC8+Xq+uH5xWo+jBaYXEgscBDxLmP\n"
    "bYox6xU2JPti1Qucj+lmveZhUZeTth2HvbC1bP6mESkGYTQxMD0gJ3NR0N6Fg9N3\n"
    "OSBGltqnxloWJ4Wyz04PToxcvr44APhL+XJ71PJ616IphdAEutNCLFGIUi7RPSRn\n"
    "R+xVzBv0yjTqJsHe3cQhifa6ezIejpZehEU4z4CqN2mLYBd0FUiRnG3wTqN3yhsc\n"
    "SPr5z0noX0+FCuKPkBurcEya67emP7SsXaRfz+bYipaQ908mgWB2XQ8kd5GzKjGf\n"
    "FlqyXYwcKapInI5v03hAcNt37N3j0VcFcC3mSZiIBYRiBXBWdoY5TtMibx3+bfEO\n"
    "s2LEPMvAhblhHrrhFYBZlAyuBbuMf1a+HNJav5fyakywxnB2sJCNwQs2uRHY1ihc\n"
    "6k/+JLcYCpsM0MF8XPtpvcyiTcaQvKZN8rG61ppnW5YCUtCC+cQKXA0o4D/I+pWV\n"
    "idWkvklsQLI+qGu41SWyxP7x09fn1txDAXYw+zuLXfdKiXyaNb78yvBXAfCNP6CH\n"
    "MntHWpdLgtJmwsQt6j8k9Kf5qLnjatkYYaA7jBU=\n"
    "-----END CERTIFICATE-----\n";

/* Expected response from signing CSR */
typedef struct {
    // Certificate from CA signing CSR
    std::string cert;
    // Serial number of issued certificate
    std::string cert_sn;
} csr_sign_resp;

class DecadaManager : public CryptoEngine
{
    public:
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
        DecadaManager(NetworkInterface*& net, SecureElement* se)
            : network_(net), CryptoEngine(se) 
        {
#else
        DecadaManager(NetworkInterface*& net)
            : network_(net)
        {
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
        if (csr_ != "")
            {
                /* Previous client certificate did not exist or was invalidated by CryptoEngine */
                csr_sign_resp sign_resp = SignCertificateSigningRequest(csr_);

                if (sign_resp.cert != "invalid" && sign_resp.cert_sn != "invalid")
                {
                    WriteClientCertificate(sign_resp.cert);
                    WriteClientCertificateSerialNumber(sign_resp.cert_sn);
                }
            }
        }
        
        /* Used by CryptoEngine to sign CSR */
        csr_sign_resp SignCertificateSigningRequest(std::string csr);

        std::string CheckDeviceCreation(void);

        /* Publish & Subscribe */
        bool Connect(void);
        bool Publish(const char* topic, std::string payload);
        bool Subscribe(const char* topic);
        bool Reconnect(void);
        bool RenewCertificate(void);
        mqtt_stack* GetMqttStackPointer(void);

    private:
        /* DECADA Provisioning */
        std::string GetAccessToken(void);
        std::string GetDeviceSecret(void);
        std::string CreateDeviceInDecada(const std::string default_name);
        csr_sign_resp RenewClientCertificate(void);
        
        /* Network Connection */
        bool ConnectMqttNetwork(void);
        bool ConnectMqttClient(void);

        /* Network Disconnection */
        void DisconnectMqttNetwork(void);
        void DisconnectMqttClient(void);

        /* Reconnection */
        bool ReconnectMqttNetwork(void);
        bool ReconnectMqttClient(void);
        bool ReconnectMqttService(void);

        const std::string decada_product_key_ = MBED_CONF_APP_DECADA_PRODUCT_KEY;
        const std::string decada_access_key_ = MBED_CONF_APP_DECADA_ACCESS_KEY;
        const std::string decada_access_secret_ = MBED_CONF_APP_DECADA_ACCESS_SECRET;
        const std::string decada_ou_id_ = MBED_CONF_APP_DECADA_OU_ID;
        const std::string api_url_ = MBED_CONF_APP_DECADA_API_URL;
        const std::string broker_ip_ = "mqtt.decada.gov.sg";      
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)      
        const int mqtt_server_port_ = 18886;
#else
        const int mqtt_server_port_ = 18885;
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
        
        std::string device_secret_;
        NetworkInterface* network_ = NULL;
        MQTTNetwork* mqtt_network_ = NULL;
        MQTTNetwork** mqtt_network_ptr_ = &mqtt_network_;
        MQTT::Client<MQTTNetwork, Countdown>* mqtt_client_ = NULL;
        MQTT::Client<MQTTNetwork, Countdown>** mqtt_client_ptr_  = &mqtt_client_;
        mqtt_stack stack_;

        std::unordered_set<std::string> sub_topics_;
};

#endif  // DECADA_MANAGER_H