/*
 *  Copyright (C) 2006-2016, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
  
/* Enable entropy for devices with TRNG. This means entropy is disabled for all other targets. */
/* Do **NOT** deploy this code in production on other targets! */
/* See https://tls.mbed.org/kb/how-to/add-entropy-sources-to-entropy-pool */
#if defined(DEVICE_TRNG)
#undef MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
#undef MBEDTLS_TEST_NULL_ENTROPY
#endif

#ifndef MBEDTLS_X509_USE_C
    #define MBEDTLS_X509_USE_C
#endif //MBEDTLS_X509_USE_C

#ifndef MBEDTLS_X509_CRT_PARSE_C
    #define MBEDTLS_X509_CRT_PARSE_C
#endif //MBEDTLS_X509_CRT_PARSE_C

#ifndef MBEDTLS_X509_CSR_PARSE_C
    #define MBEDTLS_X509_CSR_PARSE_C
#endif //MBEDTLS_X509_CSR_PARSE_C

#ifndef MBEDTLS_X509_CREATE_C
    #define MBEDTLS_X509_CREATE_C
#endif //MBEDTLS_X509_CREATE_C

#ifndef MBEDTLS_X509_CSR_WRITE_C
    #define MBEDTLS_X509_CSR_WRITE_C
#endif //MBEDTLS_X509_CSR_WRITE_C

#ifndef MBEDTLS_GENPRIME
    #define MBEDTLS_GENPRIME
#endif //MBEDTLS_GENPRIME

#ifndef MBEDTLS_PEM_WRITE_C
    #define MBEDTLS_PEM_WRITE_C
#endif //MBEDTLS_PEM_WRITE_C

// Configure mbedTLS to use alternative implementation
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
    // #define MBEDTLS_ENTROPY_HARDWARE_ALT
    #define MBEDTLS_ECDSA_GENKEY_ALT
    #define MBEDTLS_ECDSA_VERIFY_ALT
    #define MBEDTLS_ECDSA_SIGN_ALT
    #define MBEDTLS_ECDH_COMPUTE_SHARED_ALT
    #define MBEDTLS_ECDH_GEN_PUBLIC_ALT
#endif // MBED_CONF_APP_USE_SE_TLS