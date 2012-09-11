#include "clustering/administration/main/import.hpp"

#include "clustering/administration/main/json_import.hpp"
#include "http/json.hpp"
#include "utils.hpp"

bool run_json_import(UNUSED io_backender_t *backender, UNUSED std::set<peer_address_t> peers, UNUSED int client_port, UNUSED std::string db_table, UNUSED json_importer_t *importer) {
    scoped_cJSON_t json;
    for (scoped_cJSON_t json; importer->get_json(&json); json.reset(NULL)) {
        debugf("json: %s\n", json.Print().c_str());
    }


    debugf("run_json_import... returning bogus success!\n");
    return true;
}
