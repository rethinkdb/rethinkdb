// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_VIEW_MEMBER_HPP_
#define RPC_SEMILATTICE_VIEW_MEMBER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "rpc/semilattice/view.hpp"

// TODO: For fuck's sake that's not what a member is.

/* `metadata_member` is used to construct a metadata view that points to some
member of a `std::map` in another metadata view. */

template<class key_t, class value_t>
boost::shared_ptr<semilattice_read_view_t<value_t> > metadata_member(
    key_t,
    boost::shared_ptr<semilattice_read_view_t<std::map<key_t, value_t> > >);

template<class key_t, class value_t>
boost::shared_ptr<semilattice_readwrite_view_t<value_t> > metadata_member(
    key_t,
    boost::shared_ptr<semilattice_readwrite_view_t<std::map<key_t, value_t> > >);

/* `metadata_new_member` inserts a new default-constructed `value_t` into the
given view under the given key, and returns a view pointing to the newly-
inserted object. */

template<class key_t, class value_t>
boost::shared_ptr<semilattice_readwrite_view_t<value_t> > metadata_new_member(
    key_t,
    boost::shared_ptr<semilattice_readwrite_view_t<std::map<key_t, value_t> > >);

#include "rpc/semilattice/view/member.tcc"

#endif /* RPC_SEMILATTICE_VIEW_MEMBER_HPP_ */
