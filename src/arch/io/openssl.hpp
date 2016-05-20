#ifndef ARCH_IO_OPENSSL_HPP_
#define ARCH_IO_OPENSSL_HPP_

#ifdef ENABLE_TLS

// errors.hpp defines an empty final for compilers that don't support it
// That conflicts with <openssl/ssl.h>
#ifdef final
#define FINAL_WAS_DEFINED
#undef final
#endif

#include <openssl/ssl.h>

#ifdef FINAL_WAS_DEFINED
#define final
#undef FINAL_WAS_DEFINED
#endif

typedef SSL_CTX tls_ctx_t;

#else /* ENABLE_TLS */

// Some non-TLS code still passes around a null SSL_CTX*
typedef void tls_ctx_t;

#endif /* ENABLE_TLS */


#endif /* ARCH_IO_OPENSSL_HPP_ */
