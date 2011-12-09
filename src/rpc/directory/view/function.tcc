template<class outer_t, class inner_t>
class directory_function_rview_t : public directory_rview_t<inner_t> {
public:
    directory_function_rview_t(boost::function<inner_t &(outer_t &)> xf, boost::shared_ptr<directory_rview_t<outer_t> > o) :
        extractor(xf), outer(o) { }
    boost::optional<inner_t> get_value(peer_id_t peer, directory_t::right_to_access_peer_value_t *right) {
        boost::optional<outer_t> outer_value = outer->get_value(peer, right);
        if (outer_value) {
            return boost::optional<inner_t>(extractor(outer_value.get());
        } else {
            return boost::optional<inner_t>();
        }
    }
    directory_t *get_directory() {
        return outer->get_directory();
    }
private:
    boost::function<inner_t &(outer_t &)> extractor;
    boost::shared_ptr<directory_rview_t<outer_t> > outer;
};

template<class outer_t, class inner_t>
class directory_function_rwview_t : public directory_rwview_t<inner_t> {
public:
    directory_function_rwview_t(boost::function<inner_t &(outer_t &)> xf, boost::shared_ptr<directory_rwview_t<outer_t> > o) :
        extractor(xf), outer(o) { }
    boost::optional<inner_t> get_value(peer_id_t peer, directory_t::right_to_access_peer_value_t *right) {
        boost::optional<outer_t> outer_value = outer->get_value(peer, right);
        if (outer_value) {
            return boost::optional<inner_t>(extractor(outer_value.get());
        } else {
            return boost::optional<inner_t>();
        }
    }
    directory_t *get_directory() {
        return outer->get_directory();
    }
    peer_id_t get_me() {
        return outer->get_me();
    }
    void set_our_value(const inner_t &new_value_for_us, directory_t::right_to_watch_or_change_peer_value_t *right) {
        boost::optional<outer_t> outer_value = outer->get_value(outer->get_me(), right);
        extractor(outer_value.get()) = new_value_for_us;
        outer->set_our_value(outer_value.get(), right);
    }
private:
    boost::function<inner_t &(outer_t &)> extractor;
    boost::shared_ptr<directory_rwview_t<outer_t> > outer;
};

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rview_t<inner_t> > metadata_function(
        boost::function<inner_t &(outer_t &)> extractor,
        boost::shared_ptr<directory_rview_t<outer_t> > outer)
{
    return boost::make_shared<directory_function_rview_t<outer_t, inner_t> >(
        extractor, outer);
}

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rwview_t<inner_t> > metadata_function(
        boost::function<inner_t &(outer_t &)> extractor,
        boost::shared_ptr<directory_rwview_t<outer_t> > outer)
{
    return boost::make_shared<directory_function_rwview_t<outer_t, inner_t> >(
        extractor, outer);
}
