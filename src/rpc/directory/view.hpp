#ifndef __RPC_DIRECTORY_VIEW_HPP__
#define __RPC_DIRECTORY_VIEW_HPP__

#include "rpc/directory/read_view.hpp"
#include "rpc/directory/write_view.hpp"

class directory_readwrite_service_t :
    public virtual directory_read_service_t,
    public virtual directory_write_service_t
{
protected:
    virtual ~directory_readwrite_service_t() THROWS_NOTHING { }
};

template<class metadata_t>
class directory_rwview_t :
    public directory_rview_t<metadata_t>,
    public directory_wview_t<metadata_t>
{
public:
    virtual ~directory_rwview_t<metadata_t>() THROWS_NOTHING { }
    virtual directory_rwview_t<metadata_t> *clone() const THROWS_NOTHING = 0;

    virtual directory_readwrite_service_t *get_directory_service() THROWS_NOTHING = 0;

    template<class inner_t>
    clone_ptr_t<directory_rwview_t<inner_t> > subview(const clone_ptr_t<readwrite_lens_t<inner_t, metadata_t> > &lens);
};

#endif /* __RPC_DIRECTORY_VIEW_HPP__ */

#include "rpc/directory/view.tcc"
