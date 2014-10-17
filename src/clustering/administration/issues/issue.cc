// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/issue.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/datum_adapter.hpp"

void issue_t::to_datum(const metadata_t &metadata,
                       ql::datum_t *datum_out) const {
    ql::datum_t info = build_info(metadata);

    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(issue_id));
    builder.overwrite("type", ql::datum_t(get_name()));
    builder.overwrite("critical", ql::datum_t::boolean(is_critical()));
    builder.overwrite("description", ql::datum_t(build_description(info)));
    builder.overwrite("info", info);
    *datum_out = std::move(builder).to_datum();
}

const std::string issue_t::get_server_name(const issue_t::metadata_t &metadata,
                                           const machine_id_t &server_id) {
    auto machine_it = metadata.machines.machines.find(server_id);
    if (machine_it == metadata.machines.machines.end() ||
        machine_it->second.is_deleted()) {
        return std::string("__deleted_server__");
    }
    return machine_it->second.get_ref().name.get_ref().str();
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
