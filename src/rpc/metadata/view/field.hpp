#ifndef __RPC_METADATA_VIEW_FIELD_HPP__
#define __RPC_METADATA_VIEW_FIELD_HPP__

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "rpc/metadata/view.hpp"

/* `metadata_field()` can be used to construct metadata views that point to some
field on another metadata view. Example:

    class point_t {
    public:
        double x, y;
    };

    boost::shared_ptr<metadata_readwrite_view_t<point_t> > point_view =
        get_a_view_from_somewhere();
    boost::shared_ptr<metadata_readwrite_view_t<double> > x_view =
        metadata_field(&point_t::x, point_view);
*/

template<class outer_t, class inner_t>
boost::shared_ptr<metadata_read_view_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<metadata_read_view_t<outer_t> > outer);

template<class outer_t, class inner_t>
boost::shared_ptr<metadata_readwrite_view_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<metadata_readwrite_view_t<outer_t> > outer);

#include "rpc/metadata/view/field.tcc"

#endif /* __RPC_METADATA_READWRITE_VIEW_HPP__ */
