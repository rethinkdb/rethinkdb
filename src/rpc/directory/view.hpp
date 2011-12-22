#ifndef __RPC_DIRECTORY_VIEW_HPP__
#define __RPC_DIRECTORY_VIEW_HPP__

#include "rpc/directory/read_view.hpp"
#include "rpc/directory/write_view.hpp"

class directory_readwrite_service_t :
    public directory_read_service_t,
    public directory_write_service_t
    { };

template<class metadata_t>
class directory_rwview_t :
    public directory_rview_t<metadata_t>,
    public directory_wview_t<metadata_t>
{
public:
    virtual directory_readwrite_service_t *get_directory_service() = 0;
};

#endif /* __RPC_DIRECTORY_VIEW_HPP__ */
