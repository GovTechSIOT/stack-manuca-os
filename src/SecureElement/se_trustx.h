#ifndef SE_TRUSTX_H
#define SE_TRUSTX_H

#include "mbed.h"
#include "mbed_trace.h"
#include "optiga/optiga_util.h"
#define PAL_OS_HAS_EVENT_INIT
#include "optiga/pal/pal_os_event.h"
#include "secure_element.h"

#undef TRACE_GROUP
#define TRACE_GROUP  "TrustX"

/** TrustX class.
 *  @brief  Derived class of SecureElement implementing the Optiga Trust X.
 *
 *  Example:
 *  @code{.cpp}
 *  #include "mbed.h"
 *  #include "se_trustx.h"
 *
 *  int main() 
 *  {
 *
 *      TrustX trustx;
 *      while (!trustx.ready())
 *      {
 *          ThisThread::sleep_for(100ms);
 *      }
 *  }
 *  @endcode
 */

class TrustX : public SecureElement
{
    public:
        TrustX()
        {
            pal_os_event_init();

            optiga_lib_status_t status;
            for (int i = 0; i < 5; i++)
            {
                status = optiga_util_open_application(&optiga_comms);
                if (status == OPTIGA_LIB_SUCCESS)
                {
                    tr_debug("Successfully initialized Trust X");
                    break;
                }

                tr_info("Failed to initialize Trust X, retrying...");
                ThisThread::sleep_for(200ms);
            }
            
            /* Reset system after too many retries */
            if (status != OPTIGA_LIB_SUCCESS)
            {
                NVIC_SystemReset();
                ThisThread::sleep_for(Kernel::wait_for_u32_forever);
            }

            /* Configuring S/W defined current limit */
            status = optiga_util_write_data(eCURRENT_LIMITATION, OPTIGA_UTIL_WRITE_ONLY, 0, &current_limit_, 1);
            if (status == OPTIGA_LIB_SUCCESS)
            {
                tr_debug("Set current limit to %dmA", current_limit_);
                ready = true;
            }
            else
            {
                tr_warn("Failed to set current limit to %dmA", current_limit_);
            }

        }

        bool GenerateEccKeypair(mbedtls_ecp_keypair& keypair);
        mbedtls_pk_info_t GetConfiguredPkInfo(void);

        static int SignFuncWrap(void *ctx, mbedtls_md_type_t md_alg,
                                    const unsigned char *hash, size_t hash_len,
                                    unsigned char *sig, size_t *sig_len,
                                    int (*f_rng)(void *, unsigned char *, size_t),
                                    void *p_rng);

private:
        uint8_t current_limit_ = 15;
};

#endif  // SE_TRUSTX_H