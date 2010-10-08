/*
 * Summary: Exceptions for the C++ interface
 *
 * Copy: See Copyright for the status of this software.
 *
 */

/**
 * @file
 * @brief Exception declarations
 */

#ifndef LIBMEMACHED_EXCEPTION_HPP
#define LIBMEMACHED_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace memcache 
{
  class Exception : public std::runtime_error
  {
  public:
    Exception(const std::string& msg, int in_errno)
      : 
        std::runtime_error(msg), 
        _errno(in_errno) 
    {}

    Exception(const char *msg, int in_errno)
      : 
        std::runtime_error(std::string(msg)), 
        _errno(in_errno) {}

    virtual ~Exception() throw() {}

    int getErrno() const 
    { 
      return _errno; 
    }

  private:
    int _errno;
  };

  class Warning : public Exception
  {
  public:
    Warning(const std::string& msg, int in_errno) : Exception(msg, in_errno) {}
    Warning(const char *msg, int in_errno) : Exception(msg, in_errno) {}
  };

  class Error : public Exception
  {
  public:
    Error(const std::string& msg, int in_errno) : Exception(msg, in_errno) {}
    Error(const char *msg, int in_errno) : Exception(msg, in_errno) {}
    virtual ~Error() throw() {}
  };

} /* namespace libmemcached */

#endif /* LIBMEMACHED_EXCEPTION_HPP */
