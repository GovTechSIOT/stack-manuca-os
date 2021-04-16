/**
 * @defgroup boot_manager Boot Manager
 * @{
 */

#include <chrono>
#include "mbedtls/pkcs5.h"
#include "boot_manager.h"
#include "conversions.h"
#include "device_uid.h"
#include "global_params.h"
#include "persist_store.h"

#define PBKDF2_DERIVED_KEY_LEN  (32)
#define PBKDF2_SALT_LEN         (16)
#define PBKDF2_ITERATIONS       (4000)

UnbufferedSerial pc(USBTX, USBRX, 115200);

const chrono::seconds boot_timeout = 5s;    
const std::string sdk_ver = "3.2.0";
const uint8_t max_login_attempts = 3;
const std::string poll_rate_ms = "10000";
const std::string uuid = GetDeviceUid();

char pbkdf2_pers_string[] = "BootManager";
BootManagerPass bootmanager_pass;

std::string wifi_ssid = ReadWifiSsid();
std::string wifi_pass = ReadWifiPass();

bool reformat_SD = false;
std::string user_input = "";

/**
 *  @brief  Prints the SDK Header.
 *  @author Lau Lee Hong, Ng Tze Yang
 */
void PrintHeader(void)
{
    printf("\r-----------------------------------------------------\r\n");
    printf("\r  ____   ____   _____         _       ____  _             _    \r\n");
    printf("\r / ___| / ___| |_   _|__  ___| |__   / ___|| |_ __ _  ___| | __\r\n");
    printf("\r \\___ \\| |  _    | |/ _ \\/ __| '_ \\  \\___ \\| __/ _` |/ __| |/ /\r\n");
    printf("\r  ___) | |_| |   | |  __/ (__| | | |  ___) | || (_| | (__|   < \r\n");
    printf("\r |____/ \\____|   |_|\\___|\\___|_| |_| |____/ \\__\\__,_|\\___|_|\\_\\\r\n");
    printf("\r\n");
    printf("sdk v%s \r\n\r\n", sdk_ver.c_str());
}

/**
 *  @brief  Prints the Bootmanager Menu
 *  @author Ng Tze Yang
 */
void PrintMenu(void)
{
    /* Censor Wifi Password */
    std::string censored_wifi_pass = ReadWifiPass();
    censored_wifi_pass.replace(1, censored_wifi_pass.length()-2, censored_wifi_pass.length()-2, '*');
    
    printf("\ruuid %s \r\n", uuid.c_str());
    printf("\r-----------------------------------------------------\r\n");
    printf("(1) WIFI SSID \t\t\t\t %s\r\n", ReadWifiSsid().c_str());
    printf("\r-----------------------------------------------------\r\n");
    printf("(2) WIFI Password: \t\t\t %s\r\n", censored_wifi_pass.c_str());
    printf("\r-----------------------------------------------------\r\n");
    printf("(3) Clear DECADA MQTT Certificate & Key\r\n");
    printf("\r-----------------------------------------------------\r\n");
    printf("(4) Change Boot Manager Password\r\n");
    printf("\r-----------------------------------------------------\r\n");
    printf("(-1) Reset All to Defaults\r\n");
    printf("\r-----------------------------------------------------\r\n");
    printf("(-2) Save & Quit Bootmanager\r\n");
    printf("\r-----------------------------------------------------\r\n");
}

/**
 *  @brief  Enter the Boot Manager.
 *  @author Lau Lee Hong, Lee Tze Han
 *  @return Whether to enter boot manager (1) or not (0)
 */
bool EnterBootManager(void)
{
    bool boot = false;
    bootmanager_pass = ReadBootManagerPass();

    /* First startup */
    if (ReadInitFlag() != "true" || bootmanager_pass.derived_key == "" || bootmanager_pass.salt == "")
    {
        return true;
    }
    
    Timer t;
    t.start();
    while (1)
    {
        /* Launch BootManager */
        if (pc.readable())
        {
            char c = getchar();
            boot = true;
            break;
        }
        /* Normal Operation */
        if (t.elapsed_time() > boot_timeout)
        {
            break;    
        }
    }
    t.stop();
    
    return boot;
}

/**
 *  @brief  Main bootloader logic. Configures DECADA Embedded Example based on user inputs
 *  @author Lau Lee Hong, Ng Tze Yang
 */
void RunBootManager(void)
{
    if (ReadInitFlag() != "true")
    {
        SetDefaultConfig();
    }

    /* Prompt user to set password */
    if (bootmanager_pass.derived_key == "" || bootmanager_pass.salt == "")
    {
        ChangeBootManagerPass();
    }

    /* Boot Initialisation */
    WriteSwVer(sdk_ver);
    PrintHeader();
    BootManagerLogin();
    InitAfterLogin();

    /* Main Bootmanager Logic */
    while (true)
    {
        PrintMenu();
        user_input = GetUserInputString();
        /* Quit Bootmanager */
        if (user_input == "-2")
        {
            printf("End of configuration. DECADA Embedded Example will restart.\r\n");
            NVIC_SystemReset();
        }
        /* Reset Defaults */
        else if (user_input == "-1")
        {
            SetDefaultConfig();
        }
        else if (user_input == "1") 
        {
            printf("Choose new WIFI SSID:\r\n");
            WriteWifiSsid(GetUserInputString());
        }
        else if (user_input == "2") 
        {
            printf("Choose new WIFI Password:\r\n");
            WriteWifiPass(GetUserInputString(true));
        }
        else if (user_input == "3") 
        {
            ClearClientSslData();
            printf("DECADA MQTT Certificate & Key Cleared.\r\n");
        }
        else if (user_input == "4")
        {
            printf("Enter the old password:\r\n");
            
            user_input = GetUserInputString(true);
            if (CheckBootManagerPass(user_input))
            {
                ChangeBootManagerPass();
            }
            else {
                printf("Incorrect password.\r\n");
            }
        }
        else
        {
            printf("Invalid choice \"%s\". Try again...\r\n",user_input.c_str());
        }
    }
}

/**
 *  @brief  Initalisation tasks after successful login.
 *  @author Lau Lee Hong, Ng Tze Yang
 */
void InitAfterLogin(void)
{
    WriteCycleInterval(poll_rate_ms);
    WriteInitFlag("true");
}

/**
 *  @brief  Randdomly generates a salt
 *  @author Lee Tze Han
 *  @param  salt                Allocated buffer to export to
 *  @param  salt_len            Length of salt
 *  @return Success status
 */
bool GenerateSalt(unsigned char* salt, int salt_len)
{
    int rc;
    mbedtls_entropy_context entropy_ctx;
    mbedtls_ctr_drbg_context ctrdrbg_ctx;

    /* Initialize PRNG */
    mbedtls_entropy_init(&entropy_ctx);
    mbedtls_ctr_drbg_init(&ctrdrbg_ctx);

    rc = mbedtls_ctr_drbg_seed(
        &ctrdrbg_ctx, 
        mbedtls_entropy_func,
        &entropy_ctx, 
        (unsigned char*)pbkdf2_pers_string, 
        strlen(pbkdf2_pers_string)
    );
    if (rc != 0)
    {
        printf("\r\n");
        printf("mbedtls_ctr_drbg_seed returned -0x%04X - FAILED", -rc);
        printf("\r\n");

        return false;
    }

    /* Initialize salt */
    rc = mbedtls_ctr_drbg_random(&ctrdrbg_ctx, salt, salt_len);
    if (rc != 0)
    {
        printf("\r\n");
        printf("mbedtls_ctr_drbg_random returned -0x%04X - FAILED", -rc);
        printf("\r\n");

        return false;
    }

    return true;
}

/**
 *  @brief  Uses PBKDF2+SHA512 for key stretching 
 *  @author Lee Tze Han
 *  @param  pass                Password to derive key from
 *  @param  salt                Salt to use
 *  @param  salt_len            Length of salt
 *  @param  derived_key         Allocated buffer to export to
 *  @param  derived_key_len     Length of derived_key
 *  @return Success status
 *  @note   derived_key_len must be less than the output size of the PRF used (<= 32 bytes for SHA-256)
 */
bool GetDerivedKeyFromPass(std::string pass, const unsigned char* salt, int salt_len, unsigned char* derived_key, int derived_key_len)
{
    int rc;
    mbedtls_md_context_t md_ctx;
    const mbedtls_md_info_t* md_info;

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    rc = mbedtls_md_setup(&md_ctx, md_info, 1);
    if (rc != 0)
    {
        printf("\r\n");
        printf("mbedtls_md_setup returned -0x%04X - FAILED", -rc);
        printf("\r\n");

        return false;
    }

    rc = mbedtls_pkcs5_pbkdf2_hmac(
        &md_ctx, 
        (const unsigned char*)pass.c_str(), 
        pass.length(), 
        salt, 
        salt_len, 
        PBKDF2_ITERATIONS, 
        derived_key_len,
        derived_key
    );

    mbedtls_md_free(&md_ctx);
    
    return true;
}

/**
 *  @brief  Prompt for new password and save to flash if valid.
 *  @author Lee Tze Han
 */
void ChangeBootManagerPass(void)
{
    while (true)
    {
        printf("\r\n");
        printf("Choose new Boot Manager password (at least 6 char. long):\r\n");
        std::string pw = GetUserInputString(true);

        if (pw.length() < 6) {
            printf("\r\n");
            printf("Please enter a valid password.\r\n");
            continue;
        }

        printf("\r\n");
        printf("Re-enter the new password:\r\n");
        std::string pw2 = GetUserInputString(true);

        if (pw != pw2)
        {
            printf("\r\n");
            printf("Entered passwords do not match!\r\n");
            continue;
        }

        /* Key stretching */
        unsigned char salt[PBKDF2_SALT_LEN];
        unsigned char derived_key[PBKDF2_DERIVED_KEY_LEN];

        if (!GenerateSalt(salt, sizeof(salt)))
        {
            printf("\r\n");
            printf("Failed to generate salt\r\n");
            printf("\r\n");
            
            return;
        }

        if (!GetDerivedKeyFromPass(pw, salt, sizeof(salt), derived_key, sizeof(derived_key)))
        {
            printf("\r\n");
            printf("Failed to derive key from password");
            printf("\r\n");

            return;
        }

        /* Store in flash memory */
        BootManagerPass pass;
        pass.derived_key = CharToHex(derived_key, sizeof(derived_key));
        pass.salt = CharToHex(salt, sizeof(salt));
        WriteBootManagerPass(pass);

        bootmanager_pass = pass;

        printf("\r\n");
        printf("Successfully set password!\r\n");
        break;
    }
}

/**
 *  @brief  Check password entered by user.
 *  @author Lee Tze Han
 *  @param  pass    Password to be checked
 *  @return If password is correct
 */
bool CheckBootManagerPass(std::string pass)
{
    unsigned char user_key[PBKDF2_DERIVED_KEY_LEN];
    unsigned char stored_key[PBKDF2_DERIVED_KEY_LEN];
    unsigned char salt[PBKDF2_SALT_LEN];

    HexToChar(bootmanager_pass.derived_key, stored_key);
    HexToChar(bootmanager_pass.salt, salt);

    GetDerivedKeyFromPass(pass, salt, sizeof(salt), user_key, sizeof(user_key));
    
    bool ok = true;
    for (int i = 0; i < sizeof(user_key); i++)
    {
        if (user_key[i] != stored_key[i])
        {
            ok = false;
            break;
        }
    }

    return ok;
}

/**
 *  @brief  Login function to authenticate user.
 *  @author Lau Lee Hong, Ng Tze Yang
 */
void BootManagerLogin(void) 
{
    for (int i=max_login_attempts; i>0; i--)
    {
        printf("Enter password (%d attempts left): \r\n", i);
        user_input = GetUserInputString(true);
        if (CheckBootManagerPass(user_input))
        {
            printf("Login successful.\r\n");
            printf("\r\n");
            return;
        }
        printf("Wrong password.\r\n");
        printf("\r\n");
    }

    printf("OS Locked\r\n");
    while(1);
}

/**
 *  @brief  Obtains user inputs and returns it as a std::string.
 *  @author Lau Lee Hong, Ng Tze Yang
 *  @return std::string containing user input
 */
std::string GetUserInputString(bool is_hidden)
{
    std::string input = "";
    while (true)
    {
        char c = getchar();
        /* Do not show passwords typed on screen */
        if (c == '\b') {
            printf("\b \b");
        }
        else {
            if (is_hidden && c != '\r' && c != '\n') {
                printf("*");
            }
            else {
                printf("%c", c);
            }
        }

        /* User presses Enter */
        if (c == '\r' || c == '\n')
        {
            return input;
        }
        /* User presses Backspace */
        else if (c == '\b')
        {
            input = input.substr(0, input.size()-1);
        }
        else
        {
            input.push_back(c);
        }
    }
}

/**
 *  @brief  Sets configuration parameters to default values
 *  @author Ng Tze Yang, Lau Lee Hong
 */
void SetDefaultConfig(void)
{
    WriteWifiSsid("WIFI_SSID");
    WriteWifiPass("WIFI_PW");
    ClearClientSslData();
}

/**
 *  @brief  Clears client data used for SSL sessions
 *  @author Lee Tze Han
 */
void ClearClientSslData(void)
{
    WriteClientCertificate("");
    WriteClientCertificateSerialNumber("");
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 0)
    WriteClientPrivateKey("");
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
}

/**
 *  @brief  Reset the wireless module
 *  @author Goh Kok Boon
 */
void WirelessModuleReset(void)
{
    DigitalOut wifireset(PF_11);
    wifireset = 0;
    ThisThread::sleep_for(1s);
    wifireset = 1;
    ThisThread::sleep_for(1s);
}
/** @}*/
