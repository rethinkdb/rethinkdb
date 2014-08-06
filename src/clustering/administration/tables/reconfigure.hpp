// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_

#include "clustering/administration/namespace_metadata.hpp"

class server_name_client_t;

/* RSI(reql_admin): This will eventually take more parameters so the user can customize
what sort of configuration they get. */
table_config_t table_reconfigure(server_name_client_t *name_client);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_ */

