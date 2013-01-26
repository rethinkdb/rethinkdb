// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "containers/archive/vector_stream.hpp"
#include "protocol_api.hpp"

template <class protocol_t>
void get_sindex_metainfo(const secondary_index_t &sindex, region_map_t<protocol_t, sindex_details::sindex_state_t> *metainfo) {
    vector_read_stream_t read_stream(&sindex.metainfo);
    int res = deserialize(&read_stream, metainfo);
    guarantee_err(res == 0, "Corrupted metainfo.");
}

template <class protocol_t>
void set_sindex_metainfo(secondary_index_t *sindex, const region_map_t<protocol_t, sindex_details::sindex_state_t> &metainfo) {
    write_message_t wm;
    wm << metainfo;

    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0);

    sindex->metainfo = stream.vector();
}
