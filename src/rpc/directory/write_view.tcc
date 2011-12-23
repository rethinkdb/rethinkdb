#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

template<class metadata_t, class inner_t>
class subview_directory_wview_t : public directory_wview_t<inner_t> {
public:
    subview_directory_wview_t(
            directory_wview_t<metadata_t> *clone_me,
            const clone_ptr_t<readwrite_lens_t<inner_t, metadata_t> > &l) :
        superview(clone_me->clone()), lens(l)
        { }

    subview_directory_wview_t *clone() {
        return new subview_directory_wview_t(superview.get(), lens);
    }

    inner_t get_our_value(directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
        return lens->get(superview->get_our_value(proof));
    }

    void set_our_value(const inner_t &new_value_for_us, directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
        metadata_t outer = superview->get_our_value(proof);
        lens->set(&outer, new_value_for_us);
        superview->set_our_value(outer, proof);
    }

    directory_write_service_t *get_directory_service() THROWS_NOTHING {
        return superview->get_directory_service();
    }

private:
    boost::scoped_ptr<directory_wview_t<metadata_t> > superview;
    clone_ptr_t<readwrite_lens_t<inner_t, metadata_t> > lens;
};

template<class metadata_t> template<class inner_t>
clone_ptr_t<directory_wview_t<inner_t> > directory_wview_t<metadata_t>::subview(const clone_ptr_t<readwrite_lens_t<inner_t, metadata_t> > &lens) {
    return clone_ptr_t<directory_wview_t<inner_t> >(
        new subview_directory_wview_t<metadata_t, inner_t>(this, lens)
        );
}
