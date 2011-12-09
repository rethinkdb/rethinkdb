template<class outer_t, class inner_t>
inner_t &get_field(inner_t outer_t::*field, outer_t &object) {
    return object->*field;
}

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rview_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<directory_rview_t<outer_t> > outer)
{
    return metadata_function(boost::bind(&get_field<outer_t, inner_t>, field), outer);
}

template<class outer_t, class inner_t>
boost::shared_ptr<directory_rwview_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<directory_rwview_t<outer_t> > outer)
{
    return metadata_function(boost::bind(&get_field<outer_t, inner_t>, field), outer);
}
