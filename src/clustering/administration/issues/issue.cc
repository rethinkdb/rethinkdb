// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/issue.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/datum_adapter.hpp"

bool issue_t::to_datum(const metadata_t &metadata,
                       server_config_client_t *server_config_client,
                       table_meta_client_t *table_meta_client,
                       admin_identifier_format_t identifier_format,
                       ql::datum_t *datum_out) const {
    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(issue_id));
    builder.overwrite("type", ql::datum_t(get_name()));
    builder.overwrite("critical", ql::datum_t::boolean(is_critical()));
    ql::datum_t info;
    datum_string_t description;
    if (!build_info_and_description(metadata, server_config_client, table_meta_client,
            identifier_format, &info, &description)) {
        return false;
    }
    builder.overwrite("info", info);
    builder.overwrite("description", ql::datum_t(description));
    *datum_out = std::move(builder).to_datum();
    return true;
}

std::string issue_t::item_to_str(const name_string_t &str) {
    return item_to_str(str.str());
}

std::string issue_t::item_to_str(const std::string &str) {
    return 'S' + str + '\0';
}

std::string issue_t::item_to_str(const uuid_u &id) {
    return 'U' + std::string(reinterpret_cast<const char *>(id.data()),
                             id.static_size()) + '\0';
}
