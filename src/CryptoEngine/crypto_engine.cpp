/**
 * @defgroup crypto_engine_v3 Crypto Engine V3
 * @{
 */

#include "crypto_engine.h"
#include <regex>
#include <sstream> 
#include <string>
#include <vector>
#include "mbed.h"
#include "mbedtls/pk_internal.h"
#include "mbedtls/x509_csr.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbed_trace.h"
#include "global_params.h"
#include "time_engine.h"
#include "conversions.h"
#include "persist_store.h"
#include "decada_manager.h"
#include "se_trustx.h"

#undef TRACE_GROUP
#define TRACE_GROUP  "CryptoEngine"

/**
 *  @brief  Generate an ECC keypair.
 *  @author Lee Tze Han
 *  @return true (success) / false (failure)
 *  @note   If a secure element is not used, the private key will be written to flash
 */
bool CryptoEngine::GenerateKeypair(void)
{
    int rc;
    unsigned char buf[512];

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
    if (!secure_element_->GenerateEccKeypair(ecp_keypair_))
    {
        return false;
    }
    
    pk_ctx_.pk_ctx = &ecp_keypair_;

    /* Configure mbedTLS to use SE-enabled methods */
    /* Must be configured before generating keypair */
    pk_info_ = secure_element_->GetConfiguredPkInfo();
    pk_ctx_.pk_info = &pk_info_;
#else
    rc = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &ecp_keypair_, mbedtls_ctr_drbg_random, &ctrdrbg_ctx_);
    if (rc != 0)
    {
        tr_warn("mbedtls_ecp_gen_key returned -0x%04X - FAILED", -rc);
        return false;
    }

    pk_ctx_.pk_ctx = &ecp_keypair_;
    pk_ctx_.pk_info = &mbedtls_eckey_info;

    rc = mbedtls_pk_write_key_pem(&pk_ctx_, buf, sizeof(buf));
    if (rc != 0)
    {
        tr_warn("mbedtls_pk_write_key_pem returned -0x%04X - FAILED", -rc);
        return false;
    }

    WriteClientPrivateKey(string((const char*)buf));
#endif // MBED_CONF_APP_USE_SECURE_ELEMENT

    return true;
}

/**
 *  @brief  Generate CSR for retrieving client certificate from DECADA.
 *  @author Goh Kok Boon, Lau Lee Hong, Lee Tze Han
 *  @return Return PEM-formatted CSR
 */
std::string CryptoEngine::GenerateCertificateSigningRequest(void)
{
    mbedtls_x509write_csr mbedtls_csr_request;
    unsigned char mbedtls_csr_pem[1024];

    std::string mbedtls_subject_name = GetCertificateSubjectName(); 

    int rc;
    std::string csr = "invalid";
    do 
    {
        /* Always create new keypair */
        if (!GenerateKeypair())
        {
            tr_warn("Failed to generate keypair");
            return "";
        }

        /* Configure CSR */
        mbedtls_x509write_csr_init(&mbedtls_csr_request);
        mbedtls_x509write_csr_set_md_alg(&mbedtls_csr_request, MBEDTLS_MD_SHA256);
        mbedtls_x509write_csr_set_key_usage(&mbedtls_csr_request, MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
        mbedtls_x509write_csr_set_key(&mbedtls_csr_request, &pk_ctx_);

        /* Check the subject name for validity */
        rc = mbedtls_x509write_csr_set_subject_name(&mbedtls_csr_request, mbedtls_subject_name.c_str());
        if (rc)
        {
            tr_warn("mbedtls_x509write_csr_set_subject_name returned -0x%04X - FAILED", -rc);
            break;
        }

        /* Write CSR in PEM format */
        memset(mbedtls_csr_pem, 0, sizeof(mbedtls_csr_pem));
        rc = mbedtls_x509write_csr_pem(&mbedtls_csr_request, mbedtls_csr_pem, sizeof(mbedtls_csr_pem), mbedtls_ctr_drbg_random, &ctrdrbg_ctx_);        
        if (rc < 0)
        {
            tr_warn("mbedtls_x509write_csr_pem returned -0x%04X - FAILED", -rc);
            break;
        }

        tr_info("CSR Generation Successful");
        csr = (char*)mbedtls_csr_pem;
    }
    while (false);

    mbedtls_x509write_csr_free(&mbedtls_csr_request);

    return csr;
}

/**
 *  @brief  Generate subject name for certificate.
 *  @author Lee Tze Han
 *  @return Return C++ string with subject name
 */
std::string CryptoEngine::GetCertificateSubjectName(void)
{
    const std::string timestamp_ms = MsPaddingIntToString(RawRtcTimeNow());

    return cert_subject_base_ + device_uuid + timestamp_ms;
}

/**
 *  @brief  Extracts issuer's info from X509 CA Certificate into a buffer.
 *  @author Lau Lee Hong
 *  @param  buf     Output buffer to store result
 *  @param  size    Size of buffer
 *  @param  crt     Pointer to x509 certificate struct
 *  @return true (success) / false (failure)
 */
bool CryptoEngine::X509IssuerInfo(char* buf, size_t size, const mbedtls_x509_crt* crt)
{
    int ret;
    size_t n;
    char *p;
    
    p = buf;
    n = size;
    
    /* Safety check for uninitialized x509 struct */
    if(NULL == crt)
    {
        ret = mbedtls_snprintf(p, n, "\nCertificate is uninitialised!\n");

        return false;
    }

    ret = mbedtls_x509_dn_gets(p, n, &crt->issuer);
    
    return (ret < 0);
}

/**
 *  @brief  Generic SHA256 Signature Generator.
 *  @author Lau Lee Hong
 *  @param  input  Content required to generate signature 
 *  @return C++ string containing 64-character hexadecimal representation of signature
 */
std::string CryptoEngine::GenericSHA256Generator(std::string input)
{
    char signing[input.size()];
    strcpy(signing, input.c_str());

    unsigned char *signing_buffer = (unsigned char *) signing;

    const size_t buffer_len = strlen(signing);

    unsigned char output[32];
    mbedtls_sha256(signing_buffer, buffer_len, output, 0);

    char converted[32*2 + 1];
    for(int i=0;i<32;i++) 
    {
        snprintf(&converted[i*2], sizeof(converted)-(i*2), "%02X", output[i]);
    }
    converted[64] = '\0';

    return converted;
}

 /** @}*/