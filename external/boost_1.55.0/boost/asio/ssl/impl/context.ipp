//
// ssl/impl/context.ipp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2005 Voipster / Indrek dot Juhani at voipster dot com
// Copyright (c) 2005-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_IMPL_CONTEXT_IPP
#define BOOST_ASIO_SSL_IMPL_CONTEXT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)
# include <cstring>
# include <boost/asio/detail/throw_error.hpp>
# include <boost/asio/error.hpp>
# include <boost/asio/ssl/context.hpp>
# include <boost/asio/ssl/error.hpp>
#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ssl {

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)

struct context::bio_cleanup
{
  BIO* p;
  ~bio_cleanup() { if (p) ::BIO_free(p); }
};

struct context::x509_cleanup
{
  X509* p;
  ~x509_cleanup() { if (p) ::X509_free(p); }
};

struct context::evp_pkey_cleanup
{
  EVP_PKEY* p;
  ~evp_pkey_cleanup() { if (p) ::EVP_PKEY_free(p); }
};

struct context::rsa_cleanup
{
  RSA* p;
  ~rsa_cleanup() { if (p) ::RSA_free(p); }
};

struct context::dh_cleanup
{
  DH* p;
  ~dh_cleanup() { if (p) ::DH_free(p); }
};

context::context(context::method m)
  : handle_(0)
{
  switch (m)
  {
#if defined(OPENSSL_NO_SSL2)
  case context::sslv2:
  case context::sslv2_client:
  case context::sslv2_server:
    boost::asio::detail::throw_error(
        boost::asio::error::invalid_argument, "context");
    break;
#else // defined(OPENSSL_NO_SSL2)
  case context::sslv2:
    handle_ = ::SSL_CTX_new(::SSLv2_method());
    break;
  case context::sslv2_client:
    handle_ = ::SSL_CTX_new(::SSLv2_client_method());
    break;
  case context::sslv2_server:
    handle_ = ::SSL_CTX_new(::SSLv2_server_method());
    break;
#endif // defined(OPENSSL_NO_SSL2)
  case context::sslv3:
    handle_ = ::SSL_CTX_new(::SSLv3_method());
    break;
  case context::sslv3_client:
    handle_ = ::SSL_CTX_new(::SSLv3_client_method());
    break;
  case context::sslv3_server:
    handle_ = ::SSL_CTX_new(::SSLv3_server_method());
    break;
  case context::tlsv1:
    handle_ = ::SSL_CTX_new(::TLSv1_method());
    break;
  case context::tlsv1_client:
    handle_ = ::SSL_CTX_new(::TLSv1_client_method());
    break;
  case context::tlsv1_server:
    handle_ = ::SSL_CTX_new(::TLSv1_server_method());
    break;
  case context::sslv23:
    handle_ = ::SSL_CTX_new(::SSLv23_method());
    break;
  case context::sslv23_client:
    handle_ = ::SSL_CTX_new(::SSLv23_client_method());
    break;
  case context::sslv23_server:
    handle_ = ::SSL_CTX_new(::SSLv23_server_method());
    break;
#if defined(SSL_TXT_TLSV1_1)
  case context::tlsv11:
    handle_ = ::SSL_CTX_new(::TLSv1_1_method());
    break;
  case context::tlsv11_client:
    handle_ = ::SSL_CTX_new(::TLSv1_1_client_method());
    break;
  case context::tlsv11_server:
    handle_ = ::SSL_CTX_new(::TLSv1_1_server_method());
    break;
#else // defined(SSL_TXT_TLSV1_1)
  case context::tlsv11:
  case context::tlsv11_client:
  case context::tlsv11_server:
    boost::asio::detail::throw_error(
        boost::asio::error::invalid_argument, "context");
    break;
#endif // defined(SSL_TXT_TLSV1_1)
#if defined(SSL_TXT_TLSV1_2)
  case context::tlsv12:
    handle_ = ::SSL_CTX_new(::TLSv1_2_method());
    break;
  case context::tlsv12_client:
    handle_ = ::SSL_CTX_new(::TLSv1_2_client_method());
    break;
  case context::tlsv12_server:
    handle_ = ::SSL_CTX_new(::TLSv1_2_server_method());
    break;
#else // defined(SSL_TXT_TLSV1_2) 
  case context::tlsv12:
  case context::tlsv12_client:
  case context::tlsv12_server:
    boost::asio::detail::throw_error(
        boost::asio::error::invalid_argument, "context");
    break;
#endif // defined(SSL_TXT_TLSV1_2) 
  default:
    handle_ = ::SSL_CTX_new(0);
    break;
  }

  if (handle_ == 0)
  {
    boost::system::error_code ec(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    boost::asio::detail::throw_error(ec, "context");
  }

  set_options(no_compression);
}

context::context(boost::asio::io_service&, context::method m)
  : handle_(0)
{
  context tmp(m);
  handle_ = tmp.handle_;
  tmp.handle_ = 0;
}

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
context::context(context&& other)
{
  handle_ = other.handle_;
  other.handle_ = 0;
}

context& context::operator=(context&& other)
{
  context tmp(BOOST_ASIO_MOVE_CAST(context)(*this));
  handle_ = other.handle_;
  other.handle_ = 0;
  return *this;
}
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

context::~context()
{
  if (handle_)
  {
    if (handle_->default_passwd_callback_userdata)
    {
      detail::password_callback_base* callback =
        static_cast<detail::password_callback_base*>(
            handle_->default_passwd_callback_userdata);
      delete callback;
      handle_->default_passwd_callback_userdata = 0;
    }

    if (SSL_CTX_get_app_data(handle_))
    {
      detail::verify_callback_base* callback =
        static_cast<detail::verify_callback_base*>(
            SSL_CTX_get_app_data(handle_));
      delete callback;
      SSL_CTX_set_app_data(handle_, 0);
    }

    ::SSL_CTX_free(handle_);
  }
}

context::native_handle_type context::native_handle()
{
  return handle_;
}

context::impl_type context::impl()
{
  return handle_;
}

void context::clear_options(context::options o)
{
  boost::system::error_code ec;
  clear_options(o, ec);
  boost::asio::detail::throw_error(ec, "clear_options");
}

boost::system::error_code context::clear_options(
    context::options o, boost::system::error_code& ec)
{
#if (OPENSSL_VERSION_NUMBER >= 0x009080DFL) \
  && (OPENSSL_VERSION_NUMBER != 0x00909000L)
# if !defined(SSL_OP_NO_COMPRESSION)
  if ((o & context::no_compression) != 0)
  {
# if (OPENSSL_VERSION_NUMBER >= 0x00908000L)
    handle_->comp_methods = SSL_COMP_get_compression_methods();
# endif // (OPENSSL_VERSION_NUMBER >= 0x00908000L)
    o ^= context::no_compression;
  }
# endif // !defined(SSL_OP_NO_COMPRESSION)

  ::SSL_CTX_clear_options(handle_, o);

  ec = boost::system::error_code();
#else // (OPENSSL_VERSION_NUMBER >= 0x009080DFL)
      //   && (OPENSSL_VERSION_NUMBER != 0x00909000L)
  (void)o;
  ec = boost::asio::error::operation_not_supported;
#endif // (OPENSSL_VERSION_NUMBER >= 0x009080DFL)
       //   && (OPENSSL_VERSION_NUMBER != 0x00909000L)
  return ec;
}

void context::set_options(context::options o)
{
  boost::system::error_code ec;
  set_options(o, ec);
  boost::asio::detail::throw_error(ec, "set_options");
}

boost::system::error_code context::set_options(
    context::options o, boost::system::error_code& ec)
{
#if !defined(SSL_OP_NO_COMPRESSION)
  if ((o & context::no_compression) != 0)
  {
#if (OPENSSL_VERSION_NUMBER >= 0x00908000L)
    handle_->comp_methods =
      boost::asio::ssl::detail::openssl_init<>::get_null_compression_methods();
#endif // (OPENSSL_VERSION_NUMBER >= 0x00908000L)
    o ^= context::no_compression;
  }
#endif // !defined(SSL_OP_NO_COMPRESSION)

  ::SSL_CTX_set_options(handle_, o);

  ec = boost::system::error_code();
  return ec;
}

void context::set_verify_mode(verify_mode v)
{
  boost::system::error_code ec;
  set_verify_mode(v, ec);
  boost::asio::detail::throw_error(ec, "set_verify_mode");
}

boost::system::error_code context::set_verify_mode(
    verify_mode v, boost::system::error_code& ec)
{
  ::SSL_CTX_set_verify(handle_, v, ::SSL_CTX_get_verify_callback(handle_));

  ec = boost::system::error_code();
  return ec;
}

void context::set_verify_depth(int depth)
{
  boost::system::error_code ec;
  set_verify_depth(depth, ec);
  boost::asio::detail::throw_error(ec, "set_verify_depth");
}

boost::system::error_code context::set_verify_depth(
    int depth, boost::system::error_code& ec)
{
  ::SSL_CTX_set_verify_depth(handle_, depth);

  ec = boost::system::error_code();
  return ec;
}

void context::load_verify_file(const std::string& filename)
{
  boost::system::error_code ec;
  load_verify_file(filename, ec);
  boost::asio::detail::throw_error(ec, "load_verify_file");
}

boost::system::error_code context::load_verify_file(
    const std::string& filename, boost::system::error_code& ec)
{
  if (::SSL_CTX_load_verify_locations(handle_, filename.c_str(), 0) != 1)
  {
    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::add_certificate_authority(const const_buffer& ca)
{
  boost::system::error_code ec;
  add_certificate_authority(ca, ec);
  boost::asio::detail::throw_error(ec, "add_certificate_authority");
}

boost::system::error_code context::add_certificate_authority(
    const const_buffer& ca, boost::system::error_code& ec)
{
  ::ERR_clear_error();

  bio_cleanup bio = { make_buffer_bio(ca) };
  if (bio.p)
  {
    x509_cleanup cert = { ::PEM_read_bio_X509(bio.p, 0, 0, 0) };
    if (cert.p)
    {
      if (X509_STORE* store = ::SSL_CTX_get_cert_store(handle_))
      {
        if (::X509_STORE_add_cert(store, cert.p) == 1)
        {
          ec = boost::system::error_code();
          return ec;
        }
      }
    }
  }

  ec = boost::system::error_code(
      static_cast<int>(::ERR_get_error()),
      boost::asio::error::get_ssl_category());
  return ec;
}

void context::set_default_verify_paths()
{
  boost::system::error_code ec;
  set_default_verify_paths(ec);
  boost::asio::detail::throw_error(ec, "set_default_verify_paths");
}

boost::system::error_code context::set_default_verify_paths(
    boost::system::error_code& ec)
{
  if (::SSL_CTX_set_default_verify_paths(handle_) != 1)
  {
    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::add_verify_path(const std::string& path)
{
  boost::system::error_code ec;
  add_verify_path(path, ec);
  boost::asio::detail::throw_error(ec, "add_verify_path");
}

boost::system::error_code context::add_verify_path(
    const std::string& path, boost::system::error_code& ec)
{
  if (::SSL_CTX_load_verify_locations(handle_, 0, path.c_str()) != 1)
  {
    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_certificate(
    const const_buffer& certificate, file_format format)
{
  boost::system::error_code ec;
  use_certificate(certificate, format, ec);
  boost::asio::detail::throw_error(ec, "use_certificate");
}

boost::system::error_code context::use_certificate(
    const const_buffer& certificate, file_format format,
    boost::system::error_code& ec)
{
  ::ERR_clear_error();

  if (format == context_base::asn1)
  {
    if (::SSL_CTX_use_certificate_ASN1(handle_,
          static_cast<int>(buffer_size(certificate)),
          buffer_cast<const unsigned char*>(certificate)) == 1)
    {
      ec = boost::system::error_code();
      return ec;
    }
  }
  else if (format == context_base::pem)
  {
    bio_cleanup bio = { make_buffer_bio(certificate) };
    if (bio.p)
    {
      x509_cleanup cert = { ::PEM_read_bio_X509(bio.p, 0, 0, 0) };
      if (cert.p)
      {
        if (::SSL_CTX_use_certificate(handle_, cert.p) == 1)
        {
          ec = boost::system::error_code();
          return ec;
        }
      }
    }
  }
  else
  {
    ec = boost::asio::error::invalid_argument;
    return ec;
  }

  ec = boost::system::error_code(
      static_cast<int>(::ERR_get_error()),
      boost::asio::error::get_ssl_category());
  return ec;
}

void context::use_certificate_file(
    const std::string& filename, file_format format)
{
  boost::system::error_code ec;
  use_certificate_file(filename, format, ec);
  boost::asio::detail::throw_error(ec, "use_certificate_file");
}

boost::system::error_code context::use_certificate_file(
    const std::string& filename, file_format format,
    boost::system::error_code& ec)
{
  int file_type;
  switch (format)
  {
  case context_base::asn1:
    file_type = SSL_FILETYPE_ASN1;
    break;
  case context_base::pem:
    file_type = SSL_FILETYPE_PEM;
    break;
  default:
    {
      ec = boost::asio::error::invalid_argument;
      return ec;
    }
  }

  if (::SSL_CTX_use_certificate_file(handle_, filename.c_str(), file_type) != 1)
  {
    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_certificate_chain(const const_buffer& chain)
{
  boost::system::error_code ec;
  use_certificate_chain(chain, ec);
  boost::asio::detail::throw_error(ec, "use_certificate_chain");
}

boost::system::error_code context::use_certificate_chain(
    const const_buffer& chain, boost::system::error_code& ec)
{
  ::ERR_clear_error();

  bio_cleanup bio = { make_buffer_bio(chain) };
  if (bio.p)
  {
    x509_cleanup cert = {
      ::PEM_read_bio_X509_AUX(bio.p, 0,
          handle_->default_passwd_callback,
          handle_->default_passwd_callback_userdata) };
    if (!cert.p)
    {
      ec = boost::system::error_code(ERR_R_PEM_LIB,
          boost::asio::error::get_ssl_category());
      return ec;
    }

    int result = ::SSL_CTX_use_certificate(handle_, cert.p);
    if (result == 0 || ::ERR_peek_error() != 0)
    {
      ec = boost::system::error_code(
          static_cast<int>(::ERR_get_error()),
          boost::asio::error::get_ssl_category());
      return ec;
    }

    if (handle_->extra_certs)
    {
      ::sk_X509_pop_free(handle_->extra_certs, X509_free);
      handle_->extra_certs = 0;
    }

    while (X509* cacert = ::PEM_read_bio_X509(bio.p, 0,
          handle_->default_passwd_callback,
          handle_->default_passwd_callback_userdata))
    {
      if (!::SSL_CTX_add_extra_chain_cert(handle_, cacert))
      {
        ec = boost::system::error_code(
            static_cast<int>(::ERR_get_error()),
            boost::asio::error::get_ssl_category());
        return ec;
      }
    }
  
    result = ::ERR_peek_last_error();
    if ((ERR_GET_LIB(result) == ERR_LIB_PEM)
        && (ERR_GET_REASON(result) == PEM_R_NO_START_LINE))
    {
      ::ERR_clear_error();
      ec = boost::system::error_code();
      return ec;
    }
  }

  ec = boost::system::error_code(
      static_cast<int>(::ERR_get_error()),
      boost::asio::error::get_ssl_category());
  return ec;
}

void context::use_certificate_chain_file(const std::string& filename)
{
  boost::system::error_code ec;
  use_certificate_chain_file(filename, ec);
  boost::asio::detail::throw_error(ec, "use_certificate_chain_file");
}

boost::system::error_code context::use_certificate_chain_file(
    const std::string& filename, boost::system::error_code& ec)
{
  if (::SSL_CTX_use_certificate_chain_file(handle_, filename.c_str()) != 1)
  {
    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_private_key(
    const const_buffer& private_key, context::file_format format)
{
  boost::system::error_code ec;
  use_private_key(private_key, format, ec);
  boost::asio::detail::throw_error(ec, "use_private_key");
}

boost::system::error_code context::use_private_key(
    const const_buffer& private_key, context::file_format format,
    boost::system::error_code& ec)
{
  ::ERR_clear_error();

  bio_cleanup bio = { make_buffer_bio(private_key) };
  if (bio.p)
  {
    evp_pkey_cleanup evp_private_key = { 0 };
    switch (format)
    {
    case context_base::asn1:
      evp_private_key.p = ::d2i_PrivateKey_bio(bio.p, 0);
      break;
    case context_base::pem:
      evp_private_key.p = ::PEM_read_bio_PrivateKey(bio.p, 0, 0, 0);
      break;
    default:
      {
        ec = boost::asio::error::invalid_argument;
        return ec;
      }
    }

    if (evp_private_key.p)
    {
      if (::SSL_CTX_use_PrivateKey(handle_, evp_private_key.p) == 1)
      {
        ec = boost::system::error_code();
        return ec;
      }
    }
  }

  ec = boost::system::error_code(
      static_cast<int>(::ERR_get_error()),
      boost::asio::error::get_ssl_category());
  return ec;
}

void context::use_private_key_file(
    const std::string& filename, context::file_format format)
{
  boost::system::error_code ec;
  use_private_key_file(filename, format, ec);
  boost::asio::detail::throw_error(ec, "use_private_key_file");
}

void context::use_rsa_private_key(
    const const_buffer& private_key, context::file_format format)
{
  boost::system::error_code ec;
  use_rsa_private_key(private_key, format, ec);
  boost::asio::detail::throw_error(ec, "use_rsa_private_key");
}

boost::system::error_code context::use_rsa_private_key(
    const const_buffer& private_key, context::file_format format,
    boost::system::error_code& ec)
{
  ::ERR_clear_error();

  bio_cleanup bio = { make_buffer_bio(private_key) };
  if (bio.p)
  {
    rsa_cleanup rsa_private_key = { 0 };
    switch (format)
    {
    case context_base::asn1:
      rsa_private_key.p = ::d2i_RSAPrivateKey_bio(bio.p, 0);
      break;
    case context_base::pem:
      rsa_private_key.p = ::PEM_read_bio_RSAPrivateKey(bio.p, 0, 0, 0);
      break;
    default:
      {
        ec = boost::asio::error::invalid_argument;
        return ec;
      }
    }

    if (rsa_private_key.p)
    {
      if (::SSL_CTX_use_RSAPrivateKey(handle_, rsa_private_key.p) == 1)
      {
        ec = boost::system::error_code();
        return ec;
      }
    }
  }

  ec = boost::system::error_code(
      static_cast<int>(::ERR_get_error()),
      boost::asio::error::get_ssl_category());
  return ec;
}

boost::system::error_code context::use_private_key_file(
    const std::string& filename, context::file_format format,
    boost::system::error_code& ec)
{
  int file_type;
  switch (format)
  {
  case context_base::asn1:
    file_type = SSL_FILETYPE_ASN1;
    break;
  case context_base::pem:
    file_type = SSL_FILETYPE_PEM;
    break;
  default:
    {
      ec = boost::asio::error::invalid_argument;
      return ec;
    }
  }

  if (::SSL_CTX_use_PrivateKey_file(handle_, filename.c_str(), file_type) != 1)
  {
    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_rsa_private_key_file(
    const std::string& filename, context::file_format format)
{
  boost::system::error_code ec;
  use_rsa_private_key_file(filename, format, ec);
  boost::asio::detail::throw_error(ec, "use_rsa_private_key_file");
}

boost::system::error_code context::use_rsa_private_key_file(
    const std::string& filename, context::file_format format,
    boost::system::error_code& ec)
{
  int file_type;
  switch (format)
  {
  case context_base::asn1:
    file_type = SSL_FILETYPE_ASN1;
    break;
  case context_base::pem:
    file_type = SSL_FILETYPE_PEM;
    break;
  default:
    {
      ec = boost::asio::error::invalid_argument;
      return ec;
    }
  }

  if (::SSL_CTX_use_RSAPrivateKey_file(
        handle_, filename.c_str(), file_type) != 1)
  {
    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_tmp_dh(const const_buffer& dh)
{
  boost::system::error_code ec;
  use_tmp_dh(dh, ec);
  boost::asio::detail::throw_error(ec, "use_tmp_dh");
}

boost::system::error_code context::use_tmp_dh(
    const const_buffer& dh, boost::system::error_code& ec)
{
  bio_cleanup bio = { make_buffer_bio(dh) };
  if (bio.p)
  {
    return do_use_tmp_dh(bio.p, ec);
  }

  ec = boost::system::error_code(
      static_cast<int>(::ERR_get_error()),
      boost::asio::error::get_ssl_category());
  return ec;
}

void context::use_tmp_dh_file(const std::string& filename)
{
  boost::system::error_code ec;
  use_tmp_dh_file(filename, ec);
  boost::asio::detail::throw_error(ec, "use_tmp_dh_file");
}

boost::system::error_code context::use_tmp_dh_file(
    const std::string& filename, boost::system::error_code& ec)
{
  bio_cleanup bio = { ::BIO_new_file(filename.c_str(), "r") };
  if (bio.p)
  {
    return do_use_tmp_dh(bio.p, ec);
  }

  ec = boost::system::error_code(
      static_cast<int>(::ERR_get_error()),
      boost::asio::error::get_ssl_category());
  return ec;
}

boost::system::error_code context::do_use_tmp_dh(
    BIO* bio, boost::system::error_code& ec)
{
  ::ERR_clear_error();

  dh_cleanup dh = { ::PEM_read_bio_DHparams(bio, 0, 0, 0) };
  if (dh.p)
  {
    if (::SSL_CTX_set_tmp_dh(handle_, dh.p) == 1)
    {
      ec = boost::system::error_code();
      return ec;
    }
  }

  ec = boost::system::error_code(
      static_cast<int>(::ERR_get_error()),
      boost::asio::error::get_ssl_category());
  return ec;
}

boost::system::error_code context::do_set_verify_callback(
    detail::verify_callback_base* callback, boost::system::error_code& ec)
{
  if (SSL_CTX_get_app_data(handle_))
  {
    delete static_cast<detail::verify_callback_base*>(
        SSL_CTX_get_app_data(handle_));
  }

  SSL_CTX_set_app_data(handle_, callback);

  ::SSL_CTX_set_verify(handle_,
      ::SSL_CTX_get_verify_mode(handle_),
      &context::verify_callback_function);

  ec = boost::system::error_code();
  return ec;
}

int context::verify_callback_function(int preverified, X509_STORE_CTX* ctx)
{
  if (ctx)
  {
    if (SSL* ssl = static_cast<SSL*>(
          ::X509_STORE_CTX_get_ex_data(
            ctx, ::SSL_get_ex_data_X509_STORE_CTX_idx())))
    {
      if (SSL_CTX* handle = ::SSL_get_SSL_CTX(ssl))
      {
        if (SSL_CTX_get_app_data(handle))
        {
          detail::verify_callback_base* callback =
            static_cast<detail::verify_callback_base*>(
                SSL_CTX_get_app_data(handle));

          verify_context verify_ctx(ctx);
          return callback->call(preverified != 0, verify_ctx) ? 1 : 0;
        }
      }
    }
  }

  return 0;
}

boost::system::error_code context::do_set_password_callback(
    detail::password_callback_base* callback, boost::system::error_code& ec)
{
  if (handle_->default_passwd_callback_userdata)
    delete static_cast<detail::password_callback_base*>(
        handle_->default_passwd_callback_userdata);

  handle_->default_passwd_callback_userdata = callback;

  SSL_CTX_set_default_passwd_cb(handle_, &context::password_callback_function);

  ec = boost::system::error_code();
  return ec;
}

int context::password_callback_function(
    char* buf, int size, int purpose, void* data)
{
  using namespace std; // For strncat and strlen.

  if (data)
  {
    detail::password_callback_base* callback =
      static_cast<detail::password_callback_base*>(data);

    std::string passwd = callback->call(static_cast<std::size_t>(size),
        purpose ? context_base::for_writing : context_base::for_reading);

#if defined(BOOST_ASIO_HAS_SECURE_RTL)
    strcpy_s(buf, size, passwd.c_str());
#else // defined(BOOST_ASIO_HAS_SECURE_RTL)
    *buf = '\0';
    strncat(buf, passwd.c_str(), size);
#endif // defined(BOOST_ASIO_HAS_SECURE_RTL)

    return static_cast<int>(strlen(buf));
  }

  return 0;
}

BIO* context::make_buffer_bio(const const_buffer& b)
{
  return ::BIO_new_mem_buf(
      const_cast<void*>(buffer_cast<const void*>(b)),
      static_cast<int>(buffer_size(b)));
}

#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

} // namespace ssl
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SSL_IMPL_CONTEXT_IPP
