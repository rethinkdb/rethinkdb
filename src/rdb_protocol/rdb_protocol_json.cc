// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <string.h>
#include <algorithm>

#include "rdb_protocol/rdb_protocol_json.hpp"
#include "utils.hpp"

void serialize(write_message_t *wm, const std::shared_ptr<const scoped_cJSON_t> &cjson) {
    rassert(NULL != cjson.get() && NULL != cjson->get());
    serialize(wm, *cjson->get());
}

MUST_USE archive_result_t deserialize(read_stream_t *s, std::shared_ptr<const scoped_cJSON_t> *cjson) {
    cJSON *data = cJSON_CreateBlank();

    archive_result_t res = deserialize(s, data);
    if (bad(res)) { return res; }

    *cjson = std::shared_ptr<const scoped_cJSON_t>(new scoped_cJSON_t(data));

    return archive_result_t::SUCCESS;
}
