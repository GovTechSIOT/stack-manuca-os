#ifndef SECURE_ELEMENT_H
#define SECURE_ELEMENT_H

#include "mbedtls/pk.h"
#include "mbedtls/pk_internal.h"

/** SecureElement class.
 *  @brief  Interface (abstract class) for secure elements to implement.
 */

class SecureElement
{
    public:
        virtual ~SecureElement() {};
        
        virtual bool GenerateEccKeypair(mbedtls_ecp_keypair& keypair) = 0;
        virtual mbedtls_pk_info_t GetConfiguredPkInfo(void) = 0;

        bool isReady(void)
        {
            return ready;
        }

    protected:
        bool ready = false;
};

#endif  // SECURE_ELEMENT_H