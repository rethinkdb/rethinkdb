// (C) Copyright 2005 Matthias Troyer 

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Matthias Troyer

#ifndef BOOST_MPI_DETAIL_FORWARD_SKELETON_IARCHIVE_HPP
#define BOOST_MPI_DETAIL_FORWARD_SKELETON_IARCHIVE_HPP

#include <boost/serialization/pfto.hpp>

#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/archive/detail/iserializer.hpp>
#include <boost/archive/detail/interface_iarchive.hpp>
#include <boost/archive/detail/common_iarchive.hpp>
#include <boost/serialization/collection_size_type.hpp>

namespace boost { namespace mpi { namespace detail {

template<class Archive, class ImplementationArchive>
class forward_skeleton_iarchive 
  : public archive::detail::common_iarchive<Archive>
{
public:

    typedef ImplementationArchive implementation_archive_type;

    forward_skeleton_iarchive(implementation_archive_type& ar) 
      : archive::detail::common_iarchive<Archive>(archive::no_header),
        implementation_archive(ar)
    {
    }

#ifdef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
    friend class archive::detail::interface_iarchive<Archive>;
    friend class archive::load_access;
protected:
#endif

    // intermediate level to support override of operators
    // for templates in the absence of partial function 
    // template ordering
    template<class T>
    void load_override(T & t, BOOST_PFTO int)
    {
        archive::load(* this->This(), t);
    }

#define BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(T) \
    void load_override(T & t , int)             \
    {                                           \
      implementation_archive >> t;              \
    }

BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(archive::class_id_optional_type)
BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(archive::version_type)
BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(archive::class_id_type)
BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(archive::class_id_reference_type)
BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(archive::object_id_type)
BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(archive::object_reference_type)
BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(archive::tracking_type)
BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(archive::class_name_type)
BOOST_ARCHIVE_FORWARD_IMPLEMENTATION(serialization::collection_size_type)

    void load_override(std::string & s , int)    
    {                            
      serialization::collection_size_type length(s.size());              
      load_override(length,0);       
      s.resize(length);
    }

#undef BOOST_ARCHIVE_FORWARD_IMPLEMENTATION
protected:
    /// the actual archive used to serialize the information we actually want to store
    implementation_archive_type& implementation_archive;
};


} } } // end namespace boost::mpi::detail

#endif // BOOST_MPI_DETAIL_FORWARD_SKELETON_IARCHIVE_HPP
