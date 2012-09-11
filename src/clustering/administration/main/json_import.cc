#include "clustering/administration/main/json_import.hpp"

// TODO: Implement.
csv_to_json_importer_t::csv_to_json_importer_t(std::string separators, std::string filepath)
    : position_(0) {
    thread_pool_t::run_in_blokcer_pool(boost::bind(&thread_pool_t::import_json_from_file, this, separators, filepath));
}

bool csv_to_json_importer_t::get_json(UNUSED scoped_cJSON_t *out) {
    return true;
}

void csv_to_json_importer_t::import_json_from_file(std::string separators, std::string filepath) {
    // TODO: Ahem, what if the file does not exist?
    blocking_read_file_stream_t file(filepath.c_str());







}
