#ifndef SECURE_ELEMENT_SOCKET_H
#define SECURE_ELEMENT_SOCKET_H

// This class requires Mbed TLS SSL/TLS client code
#if defined(MBEDTLS_SSL_CLI_C) || defined(DOXYGEN_ONLY)

#include "SecureElementSocketWrapper.h"

/**
 * \brief SecureElementSocket is a wrapper around TCPSocket for interacting with TLS servers.
 *
 * SecureElementSocket is identical to TLSSocket except the underlying SecureElementSocketWrapper
 * uses an pre-configured mbedtls_pk_context with the client's private key. This removes the need
 * to pass in the private key in plaintext, deferring to the SecureElement to perform cryptographic
 * operations on the opaque private key.
 *
 */
class SecureElementSocket : public SecureElementSocketWrapper {
public:
    /** Create an uninitialized socket.
     *
     *  Must call open to initialize the socket on a network stack.
     */
    SecureElementSocket() : SecureElementSocketWrapper(&tcp_socket) {}

    /** Destroy the SecureElementSocket and closes the transport.
     */
    ~SecureElementSocket() {
        close();
    };

    /** Opens a socket.
     *
     *  Creates a network socket on the network stack of the given
     *  network interface.
     *
     *  @note SecureElementSocket cannot be reopened after closing. It should be destructed to
     *        clear internal TLS memory structures.
     *
     *  @param stack    Network stack as target for socket.
     *  @return         NSAPI_ERROR_OK on success. See @ref TCPSocket::open
     */
    nsapi_error_t open(NetworkStack *stack)
    {
        return tcp_socket.open(stack);
    }

    template <typename S>
    nsapi_error_t open(S *stack)
    {
        return open(nsapi_create_stack(stack));
    }

    using SecureElementSocketWrapper::connect;

private:
    TCPSocket tcp_socket;
};
#endif // MBEDTLS_SSL_CLI_C

#endif // SECURE_ELEMENT_SOCKET_H