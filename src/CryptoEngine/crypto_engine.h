#ifndef CRYPTO_ENGINE_H
#define CRYPTO_ENGINE_H

#include <string>
#include "mbed.h"
#include "mbedtls/pk_internal.h"
#include "mbed_trace.h"
#include "persist_store.h"
#include "secure_element.h"

#undef TRACE_GROUP
#define TRACE_GROUP  "CryptoEngine"

class CryptoEngine
{
    public:
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)    
        CryptoEngine(SecureElement* se)  
            : secure_element_(se)
        {
#else
        CryptoEngine(void)
        {
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)    
            mbedtls_ecp_keypair_init(&ecp_keypair_);
#else
            mbedtls_rsa_init(&rsa_keypair_, MBEDTLS_RSA_PKCS_V15, 0);
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT

            mbedtls_pk_init(&pk_ctx_);
            mbedtls_entropy_init(&entropy_ctx_);
            mbedtls_ctr_drbg_init(&ctrdrbg_ctx_);

            /* Seed PRNG on start of CryptoEngine lifecycle */
            int rc = mbedtls_ctr_drbg_seed(&ctrdrbg_ctx_, mbedtls_entropy_func, &entropy_ctx_, (const unsigned char*)mbedtls_pers_, strlen(mbedtls_pers_));
            if (rc)
            {
                tr_warn("mbedtls_ctr_drbg_seed returned -0x%04X - FAILED", -rc);
                return;
            }

            std::string client_cert = ReadClientCertificate();
            csr_ = "";

            /* Generate keypair if certificate is invalid */
            if (client_cert == "" || client_cert == "invalid")
            {
                csr_ = GenerateCertificateSigningRequest();
                if (csr_ == "")
                {
                    tr_error("No client certificate; failed to generate new CSR");
                }
            }
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
            else
            {
                /* Configure mbedTLS to use SE-enabled methods */
                pk_info_ = secure_element_->GetConfiguredPkInfo();
                pk_ctx_.pk_info = &pk_info_;
            }
#endif   // MBED_CONF_APP_USE_SECURE_ELEMENT
        }

        ~CryptoEngine(void)
        {
            mbedtls_pk_free(&pk_ctx_);
            mbedtls_ctr_drbg_free(&ctrdrbg_ctx_);
            mbedtls_entropy_free(&entropy_ctx_);

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)    
            mbedtls_ecp_keypair_free(&ecp_keypair_);
#else
            mbedtls_rsa_free(&rsa_keypair_);
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
        }

        std::string GenerateCertificateSigningRequest(void);
        std::string GetCertificateSubjectName(void);
        bool X509IssuerInfo (char* buf, size_t size, const mbedtls_x509_crt* crt);

        static std::string GenericSHA256Generator(std::string input);
    
    protected:
        mbedtls_pk_context pk_ctx_;
        std::string csr_;

    private:
        bool GenerateKeypair(void);

        const char* mbedtls_pers_ = "gen_key";
        const std::string cert_subject_base_ = "C=SG, ST=Singapore, L=Singapore, O=DECADA, OU=DECADA CA, CN=";

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
        SecureElement* secure_element_;
        mbedtls_pk_info_t pk_info_;
        mbedtls_ecp_keypair ecp_keypair_;
#else
        mbedtls_rsa_context rsa_keypair_;
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT

        mbedtls_entropy_context entropy_ctx_;
        mbedtls_ctr_drbg_context ctrdrbg_ctx_;
};

#endif  // CRYPTO_ENGINE_H