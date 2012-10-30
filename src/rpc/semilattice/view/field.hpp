// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_VIEW_FIELD_HPP_
#define RPC_SEMILATTICE_VIEW_FIELD_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "rpc/semilattice/view.hpp"

/* `metadata_field()` can be used to construct metadata views that point to some
field on another metadata view. Example:

    class point_t {
    public:
        double x, y;
    };

    boost::shared_ptr<semilattice_readwrite_view_t<point_t> > point_view =
        get_a_view_from_somewhere();
    boost::shared_ptr<semilattice_readwrite_view_t<double> > x_view =
        metadata_field(&point_t::x, point_view);
*/

template<class outer_t, class inner_t>
boost::shared_ptr<semilattice_read_view_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<semilattice_read_view_t<outer_t> > outer);

template<class outer_t, class inner_t>
boost::shared_ptr<semilattice_readwrite_view_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<semilattice_readwrite_view_t<outer_t> > outer);

#include "rpc/semilattice/view/field.tcc"

#endif /* RPC_SEMILATTICE_VIEW_FIELD_HPP_ */
