//
// ssl/detail/impl/openssl_init.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2005 Voipster / Indrek dot Juhani at voipster dot com
// Copyright (c) 2005-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_DETAIL_IMPL_OPENSSL_INIT_IPP
#define BOOST_ASIO_SSL_DETAIL_IMPL_OPENSSL_INIT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <vector>
#include <boost/asio/detail/assert.hpp>
#include <boost/asio/detail/mutex.hpp>
#include <boost/asio/detail/tss_ptr.hpp>
#include <boost/asio/ssl/detail/openssl_init.hpp>
#include <boost/asio/ssl/detail/openssl_types.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ssl {
namespace detail {

class openssl_init_base::do_init
{
public:
  do_init()
  {
    ::SSL_library_init();
    ::SSL_load_error_strings();        
    ::OpenSSL_add_all_algorithms();

    mutexes_.resize(::CRYPTO_num_locks());
    for (size_t i = 0; i < mutexes_.size(); ++i)
      mutexes_[i].reset(new boost::asio::detail::mutex);
    ::CRYPTO_set_locking_callback(&do_init::openssl_locking_func);
    ::CRYPTO_set_id_callback(&do_init::openssl_id_func);

#if !defined(SSL_OP_NO_COMPRESSION) \
  && (OPENSSL_VERSION_NUMBER >= 0x00908000L)
    null_compression_methods_ = sk_SSL_COMP_new_null();
#endif // !defined(SSL_OP_NO_COMPRESSION)
       // && (OPENSSL_VERSION_NUMBER >= 0x00908000L)
  }

  ~do_init()
  {
#if !defined(SSL_OP_NO_COMPRESSION) \
  && (OPENSSL_VERSION_NUMBER >= 0x00908000L)
    sk_SSL_COMP_free(null_compression_methods_);
#endif // !defined(SSL_OP_NO_COMPRESSION)
       // && (OPENSSL_VERSION_NUMBER >= 0x00908000L)

    ::CRYPTO_set_id_callback(0);
    ::CRYPTO_set_locking_callback(0);
    ::ERR_free_strings();
    ::ERR_remove_state(0);
    ::EVP_cleanup();
    ::CRYPTO_cleanup_all_ex_data();
    ::CONF_modules_unload(1);
#if !defined(OPENSSL_NO_ENGINE)
    ::ENGINE_cleanup();
#endif // !defined(OPENSSL_NO_ENGINE)
  }

#if !defined(SSL_OP_NO_COMPRESSION) \
  && (OPENSSL_VERSION_NUMBER >= 0x00908000L)
  STACK_OF(SSL_COMP)* get_null_compression_methods() const
  {
    return null_compression_methods_;
  }
#endif // !defined(SSL_OP_NO_COMPRESSION)
       // && (OPENSSL_VERSION_NUMBER >= 0x00908000L)

private:
  static unsigned long openssl_id_func()
  {
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
    return ::GetCurrentThreadId();
#else // defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
    void* id = instance()->thread_id_;
    if (id == 0)
      instance()->thread_id_ = id = &id; // Ugh.
    BOOST_ASIO_ASSERT(sizeof(unsigned long) >= sizeof(void*));
    return reinterpret_cast<unsigned long>(id);
#endif // defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  }

  static void openssl_locking_func(int mode, int n, 
    const char* /*file*/, int /*line*/)
  {
    if (mode & CRYPTO_LOCK)
      instance()->mutexes_[n]->lock();
    else
      instance()->mutexes_[n]->unlock();
  }

  // Mutexes to be used in locking callbacks.
  std::vector<boost::asio::detail::shared_ptr<
        boost::asio::detail::mutex> > mutexes_;

#if !defined(BOOST_ASIO_WINDOWS) && !defined(__CYGWIN__)
  // The thread identifiers to be used by openssl.
  boost::asio::detail::tss_ptr<void> thread_id_;
#endif // !defined(BOOST_ASIO_WINDOWS) && !defined(__CYGWIN__)

#if !defined(SSL_OP_NO_COMPRESSION) \
  && (OPENSSL_VERSION_NUMBER >= 0x00908000L)
  STACK_OF(SSL_COMP)* null_compression_methods_;
#endif // !defined(SSL_OP_NO_COMPRESSION)
       // && (OPENSSL_VERSION_NUMBER >= 0x00908000L)
};

boost::asio::detail::shared_ptr<openssl_init_base::do_init>
openssl_init_base::instance()
{
  static boost::asio::detail::shared_ptr<do_init> init(new do_init);
  return init;
}

#if !defined(SSL_OP_NO_COMPRESSION) \
  && (OPENSSL_VERSION_NUMBER >= 0x00908000L)
STACK_OF(SSL_COMP)* openssl_init_base::get_null_compression_methods()
{
  return instance()->get_null_compression_methods();
}
#endif // !defined(SSL_OP_NO_COMPRESSION)
       // && (OPENSSL_VERSION_NUMBER >= 0x00908000L)

} // namespace detail
} // namespace ssl
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SSL_DETAIL_IMPL_OPENSSL_INIT_IPP
