// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_PTREE_SERIALIZATION_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_PTREE_SERIALIZATION_HPP_INCLUDED

#include <boost/property_tree/ptree.hpp>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/collections_save_imp.hpp>
#include <boost/serialization/collections_load_imp.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/utility.hpp>

namespace boost { namespace property_tree
{

    ///////////////////////////////////////////////////////////////////////////
    // boost::serialization support

    /**
     * Serialize the property tree to the given archive.
     * @note In addition to serializing to regular archives, this supports
     *       serializing to archives requiring name-value pairs, e.g. XML
     *       archives.  However, the output format in the XML archive is not
     *       guaranteed to be the same as that when using the Boost.PropertyTree
     *       library's @c boost::property_tree::xml_parser::write_xml.
     * @param ar The archive to which to save the serialized property tree.
     *           This archive should conform to the concept laid out by the
     *           Boost.Serialization library.
     * @param t The property tree to serialize.
     * @param file_version file_version for the archive.
     * @post @c ar will contain the serialized form of @c t.
     */
    template<class Archive, class K, class D, class C>
    inline void save(Archive &ar,
                     const basic_ptree<K, D, C> &t,
                     const unsigned int file_version)
    {
        using namespace boost::serialization;
        stl::save_collection<Archive, basic_ptree<K, D, C> >(ar, t);
        ar << make_nvp("data", t.data());
    }

    /**
     * De-serialize the property tree to the given archive.
     * @note In addition to de-serializing from regular archives, this supports
     *       loading from archives requiring name-value pairs, e.g. XML
     *       archives. The format should be that used by
     *       boost::property_tree::save.
     * @param ar The archive from which to load the serialized property tree.
     *           This archive should conform to the concept laid out by the
     *           Boost.Serialization library.
     * @param t The property tree to de-serialize.
     * @param file_version file_version for the archive.
     * @post @c t will contain the de-serialized data from @c ar.
     */
    template<class Archive, class K, class D, class C>
    inline void load(Archive &ar,
                     basic_ptree<K, D, C> &t,
                     const unsigned int file_version)
    {
        using namespace boost::serialization;
        // Load children
        stl::load_collection<Archive,
                             basic_ptree<K, D, C>,
                             stl::archive_input_seq<Archive,
                                 basic_ptree<K, D, C> >,
                             stl::no_reserve_imp<
                                 basic_ptree<K, D, C> >
                            >(ar, t);

        // Load data (must be after load_collection, as it calls clear())
        ar >> make_nvp("data", t.data());
    }

    /**
     * Load or store the property tree using the given archive.
     * @param ar The archive from which to load or save the serialized property
     *           tree. The type of this archive will determine whether saving or
     *           loading is performed.
     * @param t The property tree to load or save.
     * @param file_version file_version for the archive.
     */
    template<class Archive, class K, class D, class C>
    inline void serialize(Archive &ar,
                          basic_ptree<K, D, C> &t,
                          const unsigned int file_version)
    {
        using namespace boost::serialization;
        split_free(ar, t, file_version);
    }

} }

#endif
