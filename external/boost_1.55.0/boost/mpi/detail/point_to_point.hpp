// Copyright 2005 Douglas Gregor.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Message Passing Interface 1.1 -- Section 3. MPI Point-to-point
#ifndef BOOST_MPI_DETAIL_POINT_TO_POINT_HPP
#define BOOST_MPI_DETAIL_POINT_TO_POINT_HPP

// For (de-)serializing sends and receives
#include <boost/mpi/config.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi/packed_iarchive.hpp>

namespace boost { namespace mpi { namespace detail {

/** Sends a packed archive using MPI_Send. */
BOOST_MPI_DECL void
packed_archive_send(MPI_Comm comm, int dest, int tag,
                    const packed_oarchive& ar);

/** Sends a packed archive using MPI_Isend.
 *
 * This routine may split sends into multiple packets. The MPI_Request
 * for each packet will be placed into the out_requests array, up to
 * num_out_requests packets. The number of packets sent will be
 * returned from the function.
 *
 * @pre num_out_requests >= 2
 */
BOOST_MPI_DECL int
packed_archive_isend(MPI_Comm comm, int dest, int tag,
                     const packed_oarchive& ar,
                     MPI_Request* out_requests, int num_out_requests);

/**
 * \overload
 */
BOOST_MPI_DECL int
packed_archive_isend(MPI_Comm comm, int dest, int tag,
                     const packed_iarchive& ar,
                     MPI_Request* out_requests, int num_out_requests);

/** Receives a packed archive using MPI_Recv. */
BOOST_MPI_DECL void
packed_archive_recv(MPI_Comm comm, int source, int tag, packed_iarchive& ar,
                    MPI_Status& status);

} } } // end namespace boost::mpi::detail

#endif // BOOST_MPI_DETAIL_POINT_TO_POINT_HPP
