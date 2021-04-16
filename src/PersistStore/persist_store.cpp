/**
 * @defgroup persist_store Persistent Storage
 * @{
 */

#include <string>
#include <sstream> // conversiondata substitute
#include "mbed-trace/mbed_trace.h"
#include "kvstore_global_api.h"
#include "persist_store.h"
#include "conversions.h"

#define TRACE_GROUP "PersistStore"

// Convenience typedef for key strings
typedef const char* const KeyName;

// List of keys used in the KVStore
namespace PersistKey
{
    KeyName DUMMY_INT =                     {"dummy_int"};
    KeyName DUMMY_STR =                     {"dummy_str"};

    KeyName SW_VER =                        {"sw_ver"};
    KeyName INIT_FLAG =                     {"init_flag"};
    KeyName TIME =                          {"time"};

    /* Network Configurations*/
    KeyName WIFI_SSID =                     {"wifi_ssid"};
    KeyName WIFI_PASS =                     {"wifi_pass"};

    /* Boot Manager */
    KeyName BOOTMANAGER_PASS_KEY =          {"bootmanager_pass_key"};
    KeyName BOOTMANAGER_PASS_SALT =         {"bootmanager_pass_salt"};

    /* Poll Rate */
    KeyName CYCLE_INTERVAL =                {"scheduler_cycle_interval"};    

    /* SSL Certificate Storage */
    KeyName CLIENT_CERTIFICATE =            {"client_certificate"};
    KeyName CLIENT_CERTIFICATE_SN =         {"client_certificate_sn"};
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 0)
    KeyName CLIENT_PRIVATE_KEY =            {"client_private_key"};
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
}

using namespace std;

// Forward declarations of helper functions
void WriteKey(KeyName key, const string& val);
void WriteKey(KeyName key, const int val);     // override for int
void WriteKey(KeyName key, const time_t val);  // override for time_t
string ReadKey(KeyName key);

////////////////////////////////////////////////////////////////////
//
//   Public functions for writing to persistent storage
//
////////////////////////////////////////////////////////////////////

/**
 *  @brief  Writes the PersistConfig struct to flash memory.
 *  @author Lee Tze Han
 *  @param  pconf PersistConfig to be stored
 */
void WriteConfig(const PersistConfig& pconf) 
{
    WriteKey(
        PersistKey::DUMMY_INT,
        pconf.dummy_int
    );
    
    WriteKey(
        PersistKey::DUMMY_STR,
        pconf.dummy_str
    );
}

/**
 *  @brief  Writes current system time (in time_t representation) to flash memory.
 *  @author Lee Tze Han
 *  @param  time time_t variable to be stored
 */
void WriteSystemTime(const time_t time)
{
    // TODO integrate with NTPClient
    WriteKey(
        PersistKey::TIME,
        time
    );
}

/**
 *  @brief  Writes current os version to flash memory.
 *  @author Lau Lee Hong
 *  @param  sw_ver Operating systems's version to be stored
 */
void WriteSwVer(const std::string sw_ver)
{
    WriteKey(
        PersistKey::SW_VER,
        sw_ver
    );
}

/**
 *  @brief  Writes flag if boot manager initialization is completed to flash memory.
 *  @author Lau Lee Hong
 *  @param  flag Initialization flag to be stored
 */
void WriteInitFlag(const std::string flag)
{
    WriteKey(
        PersistKey::INIT_FLAG,
        flag
    );
}

/**
 *  @brief  Writes WiFi SSID to flash memory.
 *  @author Lau Lee Hong
 *  @param  ssid WiFi SSID to be stored
 */
void WriteWifiSsid(const std::string ssid)
{
    WriteKey(
        PersistKey::WIFI_SSID,
        ssid
    );
}

/**
 *  @brief  Writes WiFi Pass to flash memory.
 *  @author Lau Lee Hong
 *  @param  pass WiFi Pass to be stored
 */
void WriteWifiPass(const std::string pass)
{
    WriteKey(
        PersistKey::WIFI_PASS,
        pass
    );
}

/**
 *  @brief  Writes the derived key and salt for the boot manager password to flash memory.
 *  @author Lee Tze Han
 *  @param  pass    BootManagerPass struct
 */
void WriteBootManagerPass(const BootManagerPass& pass)
{
    WriteKey(
        PersistKey::BOOTMANAGER_PASS_KEY,
        pass.derived_key
    );
    WriteKey(
        PersistKey::BOOTMANAGER_PASS_SALT,
        pass.salt
    );
}

/**
 *  @brief  Writes scheduler cycle interval(seconds) to flash memory.
 *  @author Goh Kok Boon
 *  @param  interval value of the cycle interval
 */
void WriteCycleInterval(const std::string interval)
{
    WriteKey(
        PersistKey::CYCLE_INTERVAL,
        interval
    );
}

/**
 *  @brief  Writes client certificate to flash memory.
 *  @author Goh Kok Boon
 *  @param  cert value of the certificate received from decada
 */
void WriteClientCertificate(const std::string cert)
{
    WriteKey(
        PersistKey::CLIENT_CERTIFICATE,
        cert
    );
}

/**
 *  @brief  Writes client certificate serial number to flash memory.
 *  @author Lau Lee Hong
 *  @param  cert_sn certificate serial number received from decada
 */
void WriteClientCertificateSerialNumber(const std::string cert_sn)
{
    WriteKey(
        PersistKey::CLIENT_CERTIFICATE_SN,
        cert_sn
    );
}

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 0)
/**
 *  @brief  Writes client private key (PEM) to flash memory.
 *  @author Lee Tze Han
 *  @param  private_key client private key in PEM format
 */
void WriteClientPrivateKey(const std::string private_key)
{
    WriteKey(
        PersistKey::CLIENT_PRIVATE_KEY,
        private_key
    );
}
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT

////////////////////////////////////////////////////////////////////
//
//   Public functions for reading from persistent storage
//
////////////////////////////////////////////////////////////////////

/**
 *  @brief  Reads a previously stored PersistConfig from flash memory.
 *  @author Lee Tze Han
 *  @return PersistConfig struct from persistent storage
 */
PersistConfig ReadConfig(void)
{
    PersistConfig pconf;
    
    pconf.dummy_int = StringToInt(ReadKey(PersistKey::DUMMY_INT));
    pconf.dummy_str = ReadKey(PersistKey::DUMMY_STR);
    
    return pconf;   
}


/**
 *  @brief  Reads a previously stored time_t from flash memory.
 *  @author Lee Tze Han
 *  @return Last system time time_t in persistent storage
 */
time_t ReadSystemTime(void)
{
    std::string time_str = ReadKey(PersistKey::TIME);
    return StringToTime(time_str);
}

/**
 *  @brief  Reads a previously stored Tresidder OS version from flash memory.
 *  @author Lau Lee hong
 *  @return Last Tresidder OS version in persistent storage
 */
std::string ReadSwVer(void)
{
    std::string sw_ver = ReadKey(PersistKey::SW_VER);
    return sw_ver;
}

/**
 *  @brief  Reads a previously stored init flag from flash memory.
 *  @author Lau Lee hong
 *  @return Last init flag in persistent storage
 */
std::string ReadInitFlag(void)
{
    std::string init_flag = ReadKey(PersistKey::INIT_FLAG);
    return init_flag;
}

/**
 *  @brief  Reads a previously stored WiFi SSID from flash memory.
 *  @author Lau Lee hong
 *  @return Last WiFi SSID in persistent storage
 */
std::string ReadWifiSsid(void)
{
    std::string wifi_ssid = ReadKey(PersistKey::WIFI_SSID);
    return wifi_ssid;
}

/**
 *  @brief  Reads a previously stored WiFi SSID from flash memory.
 *  @author Lau Lee Hong
 *  @return Last WiFi pass in persistent storage
 */
std::string ReadWifiPass(void)
{
    string wifi_pass = ReadKey(PersistKey::WIFI_PASS);
    return wifi_pass;
}

/**
 *  @brief  Reads the derived key and salt for the boot manager password from flash memory.
 *  @author Lee Tze Han
 *  @return BootManagerPass struct
 */
BootManagerPass ReadBootManagerPass(void)
{
    BootManagerPass pass;

    pass.derived_key = ReadKey(PersistKey::BOOTMANAGER_PASS_KEY);
    pass.salt = ReadKey(PersistKey::BOOTMANAGER_PASS_SALT);

    return pass;
}

/**
 *  @brief  Reads the current system scheduler cycle interval from flash memory.
 *  @author Goh Kok Boon
 *  @return Seconds for scheduler cycle interval
 */
std::string ReadCycleInterval(void)
{
    string cycle_interval = ReadKey(PersistKey::CYCLE_INTERVAL);
    return cycle_interval;
}

/**
 *  @brief  Reads the client certificate from flash memory.
 *  @author Goh Kok Boon
 *  @return client certificate in PEM format
 */
std::string ReadClientCertificate(void)
{
    std::string client_cert = ReadKey(PersistKey::CLIENT_CERTIFICATE);
    return client_cert;
}

/**
 *  @brief  Reads the client certificate serial number from flash memory.
 *  @author Lau Lee Hong
 *  @return client certificate serial number
 */
std::string ReadClientCertificateSerialNumber(void)
{
    std::string client_cert_serial_number = ReadKey(PersistKey::CLIENT_CERTIFICATE_SN);
    return client_cert_serial_number;
}

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 0)
/**
 *  @brief  Reads the client private key (PEM) from flash memory.
 *  @author Lee Tze Han
 *  @return client private key in PEM format
 */
std::string ReadClientPrivateKey(void)
{
    std::string client_private_key = ReadKey(PersistKey::CLIENT_PRIVATE_KEY);
    return client_private_key;
}
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT

////////////////////////////////////////////////////////////////////
//
//   Helper functions for interfacing with global KVStore API
//
////////////////////////////////////////////////////////////////////


/**
 *  @brief  Writes a key-value pair to flash memory.
 *  @author Lee Tze Han
 *  @param  key Key string
 *  @param  val String value to be stored under key
 */
void WriteKey(KeyName key, const string& val)
{
    tr_debug("Writing key \"%s\" with value \"%s\"", key, val.c_str());
    
    const char* val_cstr = val.c_str();
    
    int rc = kv_set(key, val_cstr, strlen(val_cstr), 0);
    if (rc != 0)
    {
        tr_warn("Failed to set key (returned %d)", MBED_GET_ERROR_CODE(rc));
    }
    
    tr_debug("Write OK");
}


/**
 *  @brief  Integer overload for WriteKey.
 *  @author Lee Tze Han
 *  @param  key Key string
 *  @param  val Integer value to be stored under key
 */
void WriteKey(KeyName key, const int val)
{
    WriteKey(key, IntToString(val));
}

/**
 *  @brief  time_t overload for WriteKey.
 *  @author Lee Tze Han
 *  @param  key Key string
 *  @param  val time_t value to be stored under key
 */
void WriteKey(KeyName key, const time_t val)
{
    WriteKey(key, TimeToString(val));    
}

/**
 *  @brief  Get value of key-value pair from flash memory.
 *  @author Lee Tze Han
 *  @param  key Key string
 *  @return val C++ string stored under key
 */
string ReadKey(KeyName key)
{
    tr_debug("Reading key \"%s\"", key);
    
    kv_info_t kv_info;
    int rc = kv_get_info(key, &kv_info);
    if (rc != 0)
    {
        tr_warn("Failed to get key info (returned %d)", MBED_GET_ERROR_CODE(rc));
        return string();
    }
    
    // buffer is one char larger to accomodate null terminator
    char* buffer[kv_info.size + 1];
    memset(buffer, 0, kv_info.size + 1);
    
    rc = kv_get(key, buffer, kv_info.size, NULL);
    if (rc != 0)
    {
        tr_warn("Failed to read key (returned %d)", MBED_GET_ERROR_CODE(rc));
        return string();
    }
    
    return string((const char*) buffer);
}

/** @}*/