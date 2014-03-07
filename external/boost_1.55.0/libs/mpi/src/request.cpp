// Copyright (C) 2006 Douglas Gregor.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/mpi/request.hpp>
#include <boost/mpi/status.hpp>

namespace boost { namespace mpi {

/***************************************************************************
 * request                                                                 *
 ***************************************************************************/
request::request()
  : m_handler(0), m_data()
{
  m_requests[0] = MPI_REQUEST_NULL;
  m_requests[1] = MPI_REQUEST_NULL;
}

status request::wait()
{
  if (m_handler) {
    // This request is a receive for a serialized type. Use the
    // handler to wait for completion.
    return *m_handler(this, ra_wait);
  } else if (m_requests[1] == MPI_REQUEST_NULL) {
    // This request is either a send or a receive for a type with an
    // associated MPI datatype, or a serialized datatype that has been
    // packed into a single message. Just wait on the one receive/send
    // and return the status to the user.
    status result;
    BOOST_MPI_CHECK_RESULT(MPI_Wait, (&m_requests[0], &result.m_status));
    return result;
  } else {
    // This request is a send of a serialized type, broken into two
    // separate messages. Complete both sends at once.
    MPI_Status stats[2];
    int error_code = MPI_Waitall(2, m_requests, stats);
    if (error_code == MPI_ERR_IN_STATUS) {
      // Dig out which status structure has the error, and use that
      // one when throwing the exception.
      if (stats[0].MPI_ERROR == MPI_SUCCESS 
          || stats[0].MPI_ERROR == MPI_ERR_PENDING)
        boost::throw_exception(exception("MPI_Waitall", stats[1].MPI_ERROR));
      else
        boost::throw_exception(exception("MPI_Waitall", stats[0].MPI_ERROR));
    } else if (error_code != MPI_SUCCESS) {
      // There was an error somewhere in the MPI_Waitall call; throw
      // an exception for it.
      boost::throw_exception(exception("MPI_Waitall", error_code));
    } 

    // No errors. Returns the first status structure.
    status result;
    result.m_status = stats[0];
    return result;
  }
}

optional<status> request::test()
{
  if (m_handler) {
    // This request is a receive for a serialized type. Use the
    // handler to test for completion.
    return m_handler(this, ra_test);
  } else if (m_requests[1] == MPI_REQUEST_NULL) {
    // This request is either a send or a receive for a type with an
    // associated MPI datatype, or a serialized datatype that has been
    // packed into a single message. Just test the one receive/send
    // and return the status to the user if it has completed.
    status result;
    int flag = 0;
    BOOST_MPI_CHECK_RESULT(MPI_Test, 
                           (&m_requests[0], &flag, &result.m_status));
    return flag != 0? optional<status>(result) : optional<status>();
  } else {
    // This request is a send of a serialized type, broken into two
    // separate messages. We only get a result if both complete.
    MPI_Status stats[2];
    int flag = 0;
    int error_code = MPI_Testall(2, m_requests, &flag, stats);
    if (error_code == MPI_ERR_IN_STATUS) {
      // Dig out which status structure has the error, and use that
      // one when throwing the exception.
      if (stats[0].MPI_ERROR == MPI_SUCCESS 
          || stats[0].MPI_ERROR == MPI_ERR_PENDING)
        boost::throw_exception(exception("MPI_Testall", stats[1].MPI_ERROR));
      else
        boost::throw_exception(exception("MPI_Testall", stats[0].MPI_ERROR));
    } else if (error_code != MPI_SUCCESS) {
      // There was an error somewhere in the MPI_Testall call; throw
      // an exception for it.
      boost::throw_exception(exception("MPI_Testall", error_code));
    }

    // No errors. Returns the second status structure if the send has
    // completed.
    if (flag != 0) {
      status result;
      result.m_status = stats[1];
      return result;
    } else {
      return optional<status>();
    }
  }
}

void request::cancel()
{
  if (m_handler) {
    m_handler(this, ra_cancel);
  } else {
    BOOST_MPI_CHECK_RESULT(MPI_Cancel, (&m_requests[0]));
    if (m_requests[1] != MPI_REQUEST_NULL)
      BOOST_MPI_CHECK_RESULT(MPI_Cancel, (&m_requests[1]));
  }
}

} } // end namespace boost::mpi
