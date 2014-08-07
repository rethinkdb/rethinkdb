// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_

#include "clustering/administration/namespace_metadata.hpp"

class server_name_client_t;

/* Returns a recommended `table_config_t` for the table. */
/* RSI(reql_admin): This will eventually take more parameters, such as which table we are
configuring, the user's preferences for the table configuration, access to the directory,
the pre-existing `table_config_t` if any, etc. */
table_config_t table_reconfigure(server_name_client_t *name_client);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_ */

