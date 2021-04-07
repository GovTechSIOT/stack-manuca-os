/**
 * @defgroup communications_network Communications Network
 * @{
 */
 
#include "communications_network.h"
#include "mbed.h"
#include "platform.h"
#include "mbed_trace.h"
#include "persist_store.h"
#include "trace_manager.h"
#include "trace_macro.h"

#ifdef USE_WIFI
#include "ESP32Interface.h"
#else
#include "EthernetInterface.h"
#endif  // USE_WIFI

#define TRACE_GROUP  "CommunicationsNetwork"

#if (defined(USE_WIFI) || defined(MBED_TEST_MODE))
std::string const WIFI_SSID = ReadWifiSsid();
std::string const WIFI_PASSWORD = ReadWifiPass();
#endif

/**
 *  @brief  Configure network interface (WiFi / Ethernet)
 *  @author Lee Tze Han
 *  @param network Reference to NetworkInterface pointer
 *  @return Success of NetworkInterface connection
 */
bool ConfigNetworkInterface(NetworkInterface*& network)
{
    int rc;
    
    #ifdef USE_WIFI
    const int esp32_serial_baud_rate = 115200;
    ESP32Interface* netif = new ESP32Interface(MBED_CONF_APP_WIFI_EN, NC, MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX, false, NC, NC, esp32_serial_baud_rate);
    rc = netif->connect(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str(), MBED_CONF_APP_WIFI_SECURITY);
    #else
    EthernetInterface* netif = new EthernetInterface();
    rc = netif->connect();
    #endif  // USE_WIFI
    
    network = netif;
    
    if (rc != 0)
    {
        /* Unable to configure NetworkInterface */
        tr_err("Failed to configure NetworkInterface (rc = %d)", rc);
        
        return false;
    }
    else
    {
        tr_info("NetworkInterface successfully configured");
    }
    
    return true;
}

/** @}*/