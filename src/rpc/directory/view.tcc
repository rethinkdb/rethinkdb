

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rview_t<inner_t> > directory_simple_subview(const boost::function<inner_t *(outer_t *)> &, const boost::shared_ptr<directory_rview_t<outer_t> > &);

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rwview_t<inner_t> > directory_simple_subview(const boost::function<inner_t *(outer_t *)> &, const boost::shared_ptr<directory_rwview_t<outer_t> > &);