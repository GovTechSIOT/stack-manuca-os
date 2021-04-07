#ifndef MQTTNETWORK_H
#define MQTTNETWORK_H

#include "NetworkInterface.h"

#undef USE_TLS
#if defined(MBED_CONF_APP_USE_TLS) && (MBED_CONF_APP_USE_TLS == 1)
  #define USE_TLS
#endif

#ifdef USE_TLS
/**
 * This file has been modified from the original MQTT Library to support 
 * the usage of opaque private keys through SecureElementSocket 
 */
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
#include "mbedtls/pk.h"
#include "SecureElementSocket.h"
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
#else
#include "TCPSocket.h"
#endif  // USE_TLS

class MQTTNetwork {
public:
    MQTTNetwork(NetworkInterface* aNetwork) : network(aNetwork) {
#ifdef USE_TLS
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
        socket = new SecureElementSocket();
#else
        socket = new TLSSocket();
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
#else
        socket = new TCPSocket();
#endif  // USE_TLS
    }

    ~MQTTNetwork() {
        delete socket;
    }

    int read(unsigned char* buffer, int len, int timeout) {
        return socket->recv(buffer, len);
    }

    int write(unsigned char* buffer, int len, int timeout) {
        return socket->send(buffer, len);
    }

#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
    int connect(const char* hostname, int port, const char *ssl_ca_pem,
            const char *ssl_cli_pem, const mbedtls_pk_context& mbedtls_pk_ctx) {
#else   
    int connect(const char* hostname, int port, const char *ssl_ca_pem,
            const char *ssl_cli_pem, const char *ssl_pk_pem) {
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
        int ret = NSAPI_ERROR_OK;
        if ((ret = socket->open(network)) != NSAPI_ERROR_OK) {
            return ret;
        }

        SocketAddress addr;
        if (network->gethostbyname(hostname, &addr) != NSAPI_ERROR_OK) {
            return NSAPI_ERROR_DNS_FAILURE;
        }

        addr.set_port(port);
        socket->set_hostname(hostname);

#ifdef USE_TLS
        socket->set_root_ca_cert(ssl_ca_pem);
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
        socket->set_client_cert_key(ssl_cli_pem, mbedtls_pk_ctx);
#else
        socket->set_client_cert_key(ssl_cli_pem, ssl_pk_pem);
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
#endif  // USE_TLS
        return socket->connect(addr);
    }

    int disconnect() {
        return socket->close();
    }

private:
    NetworkInterface* network;
#ifdef USE_TLS
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 1)
    SecureElementSocket* socket;
#else
    TLSSocket* socket;
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT
#else
    TCPSocket* socket;
#endif  // USE_TLS
};

#endif  // MQTTNETWORK_H
