#ifndef __RPC_DIRECTORY_VIEW_FIELD_HPP__
#define __RPC_DIRECTORY_VIEW_FIELD_HPP__

#include "rpc/directory/view.hpp"

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rview_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<directory_rview_t<outer_t> > outer);

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rwview_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<directory_rwview_t<outer_t> > outer);

#include "rpc/directory/view/field.tcc"

#endif /* __RPC_DIRECTORY_VIEW_FIELD_HPP__ */
