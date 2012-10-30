// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_VIEW_FUNCTION_HPP_
#define RPC_SEMILATTICE_VIEW_FUNCTION_HPP_
#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "rpc/semilattice/view.hpp"

//using metadata_function allows you to make a view using an accessor function,
//this is convenient of the value you want to extract is contained within a few
//layers of member classes
template<class outer_t, class inner_t>
boost::shared_ptr<semilattice_read_view_t<inner_t> > metadata_function(
        boost::function<inner_t &(outer_t &)>,
        boost::shared_ptr<semilattice_read_view_t<outer_t> > outer);


template<class outer_t, class inner_t>
boost::shared_ptr<semilattice_readwrite_view_t<inner_t> > metadata_function(
        boost::function<inner_t &(outer_t &)>,
        boost::shared_ptr<semilattice_readwrite_view_t<outer_t> > outer);

#include "rpc/semilattice/view/function.tcc"

#endif /* RPC_SEMILATTICE_VIEW_FUNCTION_HPP_ */
