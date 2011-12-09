#ifndef __RPC_DIRECTORY_VIEW_FUNCTION_HPP__
#define __RPC_DIRECTORY_VIEW_FUNCTION_HPP__

#include "rpc/directory/view.hpp"

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rview_t<inner_t> > metadata_function(
        boost::function<inner_t &(outer_t &)>,
        boost::shared_ptr<directory_rview_t<outer_t> > outer);

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rwview_t<inner_t> > metadata_function(
        boost::function<inner_t &(outer_t &)>,
        boost::shared_ptr<directory_rwview_t<outer_t> > outer);

#include "rpc/directory/view/function.tcc"

#endif /* __RPC_DIRECTORY_VIEW_FUNCTION_HPP__ */
