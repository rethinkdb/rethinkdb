#include "clustering/administration/main/json_import.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"
#include "arch/types.hpp"
#include "containers/archive/file_stream.hpp"

csv_to_json_importer_t::csv_to_json_importer_t(std::string separators, std::string filepath)
    : position_(0) {
    thread_pool_t::run_in_blocker_pool(boost::bind(&csv_to_json_importer_t::import_json_from_file, this, separators, filepath));
}

// TODO: Implement.
bool csv_to_json_importer_t::get_json(UNUSED scoped_cJSON_t *out) {
    return true;
}

void csv_to_json_importer_t::import_json_from_file(UNUSED std::string separators, std::string filepath) {
    blocking_read_file_stream_t file;
    bool success = file.init(filepath.c_str());
    // TODO(sam): Fail more cleanly.
    guarantee(success);







}
