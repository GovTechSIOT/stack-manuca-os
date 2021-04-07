/**
 * @defgroup se_trustx Optiga Trust X Secure Element
 * @{
 */

#include "mbed.h"
#include "mbed_trace.h"
#include "optiga/optiga_crypt.h"
#include "optiga/ifx_i2c/ifx_i2c_config.h"
#include "se_trustx.h"
#include "mbedtls/asn1write.h"

#undef TRACE_GROUP
#define TRACE_GROUP  "TrustX"

/* Non-volatile key for client certificate */
optiga_key_id_t ssl_key_id = OPTIGA_KEY_STORE_ID_E0F1;

/* Volatile keys for TLS session (ECDSA / EDCH) */
optiga_key_id_t ecdsa_key_id = OPTIGA_SESSION_ID_E100;
optiga_key_id_t ecdh_key_id = OPTIGA_SESSION_ID_E101;

/* Configuration for Trust X I2C */
optiga_comms_t optiga_comms = {
    (void*) &ifx_i2c_context_0, 
    NULL, 
    NULL, 
    OPTIGA_COMMS_SUCCESS
};

/**
 *  @brief      Generates ECC keypair using the Trust X API.
 *  @details    The Trust X generates and returns an octet string, and parsed into an mbedtls_pk_context
 *  @author     Lee Tze Han
 *  @param      keypair     mbedtls_ecp_keypair to configure
 *  @return     Success status
 */
bool TrustX::GenerateEccKeypair(mbedtls_ecp_keypair& keypair)
{
    uint8_t pk[MBEDTLS_ECP_MAX_PT_LEN];
    uint16_t pk_len = sizeof(pk);

    int ret = optiga_crypt_ecc_generate_keypair(
        OPTIGA_ECC_NIST_P_256,
        (OPTIGA_KEY_USAGE_AUTHENTICATION | OPTIGA_KEY_USAGE_SIGN), 
        false, 
        &ssl_key_id, 
        pk, 
        &pk_len
    );
    if (ret != OPTIGA_LIB_SUCCESS)
    {
        tr_warn("Failed to generate keypair");
        return false;
    }

    mbedtls_ecp_group_load(&keypair.grp, MBEDTLS_ECP_DP_SECP256R1);

    /* 
     * The first 3 bytes correspond to the ASN.1 OID
     * Remaining bytes are in the form [W][X][Y] (i.e. pk[3..])
     * mbedtls_ecp_point_read_binary expects an uncompressed octet string (i.e. W = 0x04) 
     */
    ret = mbedtls_ecp_point_read_binary(&keypair.grp, &keypair.Q, &pk[3], pk_len-3);
    if (ret) 
    {
        tr_warn("Error in mbedtls_ecp_point_read_binary (-0x%X)", -ret);
        return false;
    }

    return true;
}

/**
 *  @brief      Returns an mbedtls_pk_info_t with function pointers to TrustX wrappers around cryptographic operations.
 *  @author     Lee Tze Han
 *  @return     Configured mbedtls_pk_info_t
 */
mbedtls_pk_info_t TrustX::GetConfiguredPkInfo(void)
{
    mbedtls_pk_info_t mbedtls_trustx_info = mbedtls_eckey_info;
    /* Overwrite signing function to use Trust X signing */
    mbedtls_trustx_info.sign_func = SignFuncWrap;

    return mbedtls_trustx_info;
}

/**
 *  @brief      TrustX wrapper for mbedtls signing operation.
 *  @author     Lee Tze Han
 *  @return     0 on success or error code on failure
 */
int TrustX::SignFuncWrap(void *ctx, mbedtls_md_type_t md_alg,
                        const unsigned char *hash, size_t hash_len,
                        unsigned char *sig, size_t *sig_len,
                        int (*f_rng)(void *, unsigned char *, size_t),
                        void *p_rng)
{
    int status;
    uint8_t der_sig[MBEDTLS_ECDSA_MAX_LEN];
    uint16_t ds_len = sizeof(der_sig);

    /* Truncate hash if longer than key */
    hash_len = (hash_len > 32) ? 32 : hash_len;

    status = optiga_crypt_ecdsa_sign(
        (uint8_t*)hash, 
        hash_len, 
        ssl_key_id, 
        der_sig, 
        &ds_len
    );
    if (status != OPTIGA_LIB_SUCCESS)
    {
        tr_warn("Error in optiga_crypt_ecdsa_sign (-0x%X)", -status);
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    /* ASN.1 DER format for signature */
    sig[0] = 0x30;
    sig[1] = ds_len;
    memcpy(sig + 2, der_sig, ds_len);
    *sig_len = 2 + ds_len;
    
    return 0;
}

/**
 *  @brief      This function computes the ECDSA signature of a previously-hashed message.
 *  @author     Lee Tze Han
 *  @return     0 on success or error code on failure
 *  @note       Overrides the default mbedTLS implementation to use the Trust X
 */
#ifdef MBEDTLS_ECDSA_SIGN_ALT
int mbedtls_ecdsa_sign(mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                        const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                        int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
    tr_debug("Using MBEDTLS_ECDSA_SIGN_ALT implementation");

    /* Expects NIST P-256 */
    if (grp->id != MBEDTLS_ECP_DP_SECP256R1)
    {
        tr_warn("Group not supported (Expected secp256r1)");
        return MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
    }

	int ret;
	uint8_t der_signature[MBEDTLS_ECDSA_MAX_LEN];
	uint16_t dslen = sizeof(der_signature);
    uint8_t *p = der_signature;
    const uint8_t *end = der_signature + dslen;

    ret = optiga_crypt_ecdsa_sign(
        (unsigned char*)buf, 
        blen, 
        ecdsa_key_id, 
        der_signature, 
        &dslen
    );
    if (ret != OPTIGA_LIB_SUCCESS) 
    {
	    return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }
	
	MBEDTLS_MPI_CHK(mbedtls_asn1_get_mpi(&p, end, r));
	MBEDTLS_MPI_CHK(mbedtls_asn1_get_mpi(&p, end, s));

cleanup:
    return ret;
}
#endif // MBEDTLS_ECDSA_SIGN_ALT

/**
 *  @brief      This function verifies the ECDSA signature of a previously-hashed message.
 *  @author     Lee Tze Han
 *  @return     0 on success or error code on failure
 *  @note       Overrides the default mbedTLS implementation to use the Trust X
 */
#ifdef MBEDTLS_ECDSA_VERIFY_ALT
int mbedtls_ecdsa_verify(mbedtls_ecp_group *grp,
                  const unsigned char *buf, size_t blen,
                  const mbedtls_ecp_point *Q, const mbedtls_mpi *r, const mbedtls_mpi *s)
{
    tr_debug("Using MBEDTLS_ECDSA_VERIFY_ALT implementation");

    /* Expects NIST P-256 */
    if (grp->id != MBEDTLS_ECP_DP_SECP256R1)
    {
        tr_warn("Group not supported (Expected secp256r1)");
        return MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
    }
	
    int ret;
    public_key_from_host_t host_pk;
    uint8_t pk[MBEDTLS_ECP_MAX_PT_LEN];
    size_t pk_len = 0;
    uint8_t sig[MBEDTLS_ECDSA_MAX_LEN];
    uint8_t* p = sig + sizeof(sig);
	size_t sig_len = 0;

    /* Expected format for signature to Trust X */
    /* Ref.: https://github.com/Infineon/optiga-trust-x/wiki/Data-format-examples#ECDSA-Signature */
    MBEDTLS_ASN1_CHK_ADD(sig_len, mbedtls_asn1_write_mpi(&p, sig, s));
    MBEDTLS_ASN1_CHK_ADD(sig_len, mbedtls_asn1_write_mpi(&p, sig, r));

    ret = mbedtls_ecp_point_write_binary(
        grp, 
        Q, 
        MBEDTLS_ECP_PF_UNCOMPRESSED,
        &pk_len, 
        &pk[3], 
        sizeof(pk)
    );
    if (ret)
    {
        tr_warn("Error in mbedtls_ecp_point_read_binary (-0x%X)", -ret);
        return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    }

    /* Expected format for public key to Trust X */
    /* Ref.: https://github.com/Infineon/optiga-trust-x/wiki/Data-format-examples#ECC-Public-Key */
    pk[0] = 0x03;
    pk[1] = pk_len + 1;
    pk[2] = 0x00;

    host_pk.public_key = pk;
    host_pk.length = pk_len + 3;
    host_pk.curve = OPTIGA_ECC_NIST_P_256;

    /* Truncate hash if longer than key */
    blen = (blen > 32) ? 32 : blen;

    ret = optiga_crypt_ecdsa_verify(
        (uint8_t *)buf, 
        blen, 
        (uint8_t *)p, 
        sig_len,
        OPTIGA_CRYPT_HOST_DATA, 
        (void *)&host_pk
    );
    if (ret != OPTIGA_LIB_SUCCESS)
    {
        tr_warn("Error in optiga_crypt_ecdsa_verify (-0x%X)", -ret);
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }
	
    return ret;
}
#endif // MBEDTLS_ECDSA_VERIFY_ALT

/**
 *  @brief      This function generates an ECDSA keypair on the given curve.
 *  @author     Lee Tze Han
 *  @return     0 on success or error code on failure
 *  @note       Overrides the default mbedTLS implementation to use the Trust X
 */
#ifdef MBEDTLS_ECDSA_GENKEY_ALT
int mbedtls_ecdsa_genkey(mbedtls_ecdsa_context *ctx, mbedtls_ecp_group_id gid,
                            int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
    tr_debug("Using MBEDTLS_ECDSA_GENKEY_ALT implementation");

    /* Expects NIST P-256 */
    if (gid != MBEDTLS_ECP_DP_SECP256R1)
    {
        tr_warn("Group not supported (Expected secp256r1)");
        return MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
    }

    int ret;
    uint8_t pk[MBEDTLS_ECP_MAX_PT_LEN];
    uint16_t pk_len = sizeof(pk);
    mbedtls_ecp_group* grp = &ctx->grp;
 
    mbedtls_ecp_group_load(&ctx->grp, gid);

    ret = optiga_crypt_ecc_generate_keypair(
        OPTIGA_ECC_NIST_P_256,
        (OPTIGA_KEY_USAGE_KEY_AGREEMENT | OPTIGA_KEY_USAGE_AUTHENTICATION),
        false,
        &ecdsa_key_id, 
        pk, 
        &pk_len
    );
    if (ret != OPTIGA_LIB_SUCCESS)
    {
        tr_warn("Error in optiga_crypt_ecc_generate_keypair (-0x%X)", -ret);
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    ret = mbedtls_ecp_point_read_binary(grp, &ctx->Q, &pk[3], pk_len - 3);
    if (ret)
    {
        tr_warn("Error in mbedtls_ecp_point_read_binary (-0x%X)", -ret);
        return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    }

    return ret;
}					  
#endif // MBEDTLS_ECDSA_GENKEY_ALT

/**
 *  @brief      This function generates an ECDH keypair on an elliptic curve.
 *  @author     Lee Tze Han
 *  @return     0 on success or error code on failure
 *  @note       Overrides the default mbedTLS implementation to use the Trust X
 */
#ifdef MBEDTLS_ECDH_GEN_PUBLIC_ALT
int mbedtls_ecdh_gen_public(mbedtls_ecp_group *grp, mbedtls_mpi *d, mbedtls_ecp_point *Q,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng)
{
    tr_debug("Using MBEDTLS_ECDH_GEN_PUBLIC_ALT implementation");

    /* Expects NIST P-256 */
    if (grp->id != MBEDTLS_ECP_DP_SECP256R1)
    {
        tr_warn("Group not supported (Expected secp256r1)");
        return MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
    }

    int ret;
    uint8_t pk[MBEDTLS_ECP_MAX_PT_LEN];
    uint16_t pk_len = sizeof(pk);

    ret = optiga_crypt_ecc_generate_keypair(
        OPTIGA_ECC_NIST_P_256,
        (OPTIGA_KEY_USAGE_KEY_AGREEMENT | OPTIGA_KEY_USAGE_AUTHENTICATION),
        false, 
        &ecdh_key_id, 
        pk, 
        &pk_len
    );
    if (ret != OPTIGA_LIB_SUCCESS)
    {
		tr_warn("Error in optiga_crypt_ecc_generate_keypair (-0x%X)", -ret);
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    ret = mbedtls_ecp_point_read_binary(grp, Q, &pk[3], pk_len - 3);
    if (ret)
	{
		tr_warn("Error in mbedtls_ecp_point_read_binary (-0x%X)", -ret);
        return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
	}

    return ret;
}
#endif // MBEDTLS_ECDH_GEN_PUBLIC_ALT

/**
 *  @brief      This function computes the shared secret.
 *  @author     Lee Tze Han
 *  @return     0 on success or error code on failure
 *  @note       Overrides the default mbedTLS implementation to use the Trust X
 */
#ifdef MBEDTLS_ECDH_COMPUTE_SHARED_ALT
int mbedtls_ecdh_compute_shared(mbedtls_ecp_group *grp, mbedtls_mpi *z,
                               const mbedtls_ecp_point *Q, const mbedtls_mpi *d,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng)
{
    tr_debug("Using MBEDTLS_ECDH_COMPUTE_SHARED_ALT implementation");

    /* Expects NIST P-256 */
    if (grp->id != MBEDTLS_ECP_DP_SECP256R1)
    {
        tr_warn("Group not supported (Expected secp256r1)");
        return MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
    }

    int ret;
    public_key_from_host_t host_pk;
    uint8_t pk[MBEDTLS_ECP_MAX_PT_LEN];
    size_t pk_len;
    uint8_t buf[MBEDTLS_ECP_MAX_PT_LEN];

    /* Expected format for signature to Trust X */
    /* Ref.: https://github.com/Infineon/optiga-trust-x/wiki/Data-format-examples#ECDSA-Signature */
    pk[0] = 0x03;
    pk[1] = 0x42;
    pk[2] = 0x00;

    mbedtls_ecp_point_write_binary(
        grp,
        Q, 
        MBEDTLS_ECP_PF_UNCOMPRESSED, 
        &pk_len,
        &pk[3], 
        sizeof(buf)
    );

    host_pk.curve = OPTIGA_ECC_NIST_P_256;
    host_pk.public_key = pk;
    host_pk.length = pk_len + 3;

    ret = optiga_crypt_ecdh(ecdh_key_id, &host_pk, true, buf);
    if (ret != OPTIGA_LIB_SUCCESS)
    {
        tr_warn("Error in optiga_crypt_ecdh (-0x%X)", -ret);
        return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    }

    mbedtls_mpi_read_binary(z, buf, mbedtls_mpi_size(&grp->P));

    return ret;
}
#endif // MBEDTLS_ECDH_COMPUTE_SHARED_ALT

/**
 *  @brief      Entropy poll callback for a hardware source.
 *  @author     Lee Tze Han
 *  @return     0 (success) / 1 (failure)
 *  @note       Uses the Trust X TRNG to seed mbedTLS DRNG
 */
#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    tr_debug("Using MBEDTLS_ENTROPY_HARDWARE_ALT implementation");
	
	int status = optiga_crypt_random(OPTIGA_RNG_TYPE_TRNG, output, len);
	if (status != OPTIGA_LIB_SUCCESS)
	{
		*olen = 0;
		return 1;
	}

    *olen = len;
	return 0;
}
#endif // MBEDTLS_ENTROPY_HARDWARE_ALT

/** @}*/