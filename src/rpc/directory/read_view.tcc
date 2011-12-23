#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

template<class metadata_t, class inner_t>
class subview_directory_single_rview_t :
    public directory_single_rview_t<inner_t>
{
public:
    subview_directory_single_rview_t(
            directory_single_rview_t<metadata_t> *clone_me,
            const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &l) :
        superview(clone_me->clone()), lens(l)
        { }

    subview_directory_single_rview_t *clone() {
        return new subview_directory_single_rview_t(superview.get(), lens);
    }

    boost::optional<inner_t> get_value() THROWS_NOTHING {
        boost::optional<metadata_t> outer = superview->get_value();
        if (outer) {
            return boost::optional<inner_t>(lens->get(outer.get()));
        } else {
            return boost::optional<inner_t>();
        }
    }

    directory_read_service_t *get_directory_service() THROWS_NOTHING {
        return superview->get_directory_service();
    }

    peer_id_t get_peer() THROWS_NOTHING {
        return superview->get_peer();
    }

private:
    boost::scoped_ptr<directory_single_rview_t<metadata_t> > superview;
    clone_ptr_t<read_lens_t<inner_t, metadata_t> > lens;
};

template<class metadata_t> template<class inner_t>
clone_ptr_t<directory_single_rview_t<inner_t> > directory_single_rview_t<metadata_t>::subview(const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &lens) THROWS_NOTHING {
    return clone_ptr_t<directory_single_rview_t<inner_t> >(
        new subview_directory_single_rview_t<metadata_t, inner_t>(
            this,
            lens
        ));
}

template<class metadata_t, class inner_t>
class subview_directory_rview_t :
    public directory_rview_t<inner_t>
{
public:
    subview_directory_rview_t(
            directory_rview_t<metadata_t> *clone_me,
            const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &l) :
        superview(clone_me->clone()), lens(l)
        { }

    subview_directory_rview_t *clone() {
        return new subview_directory_rview_t(superview.get(), lens);
    }

    boost::optional<inner_t> get_value(peer_id_t peer) THROWS_NOTHING {
        boost::optional<metadata_t> outer = superview->get_value(peer);
        if (outer) {
            return boost::optional<inner_t>(lens->get(outer.get()));
        } else {
            return boost::optional<inner_t>();
        }
    }

    directory_read_service_t *get_directory_service() THROWS_NOTHING {
        return superview->get_directory_service();
    }

private:
    boost::scoped_ptr<directory_rview_t<metadata_t> > superview;
    clone_ptr_t<read_lens_t<inner_t, metadata_t> > lens;
};

template<class metadata_t> template<class inner_t>
clone_ptr_t<directory_rview_t<inner_t> > directory_rview_t<metadata_t>::subview(const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &lens) THROWS_NOTHING {
    return clone_ptr_t<directory_rview_t<inner_t> >(
        new subview_directory_rview_t<metadata_t, inner_t>(
            this,
            lens
        ));
}

template<class metadata_t>
class peer_subview_directory_single_rview_t :
    public directory_single_rview_t<metadata_t>
{
public:
    peer_subview_directory_single_rview_t(
            directory_rview_t<metadata_t> *clone_me,
            peer_id_t p) :
        superview(clone_me->clone()), peer(p)
        { }

    peer_subview_directory_single_rview_t *clone() {
        return new peer_subview_directory_single_rview_t(superview.get(), peer);
    }

    boost::optional<metadata_t> get_value() THROWS_NOTHING {
        return superview->get_value(peer);
    }

    directory_read_service_t *get_directory_service() THROWS_NOTHING {
        return superview->get_directory_service();
    }

    peer_id_t get_peer() THROWS_NOTHING {
        return peer;
    }

private:
    boost::scoped_ptr<directory_rview_t<metadata_t> > superview;
    peer_id_t peer;
};
