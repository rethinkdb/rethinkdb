#ifndef CLUSTERING_ADMINISTRATION_MAIN_COMMAND_LINE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_COMMAND_LINE_HPP_

int main_rethinkdb_create(int argc, char *argv[]);
int main_rethinkdb_serve(int argc, char *argv[]);
int main_rethinkdb_proxy(int argc, char *argv[]);
int main_rethinkdb_admin(int argc, char *argv[]);
int main_rethinkdb_porcelain(int argc, char *argv[]);
int main_rethinkdb_import(int argc, char *argv[]);

void help_rethinkdb_create();
void help_rethinkdb_serve();
void help_rethinkdb_proxy();

#endif /* CLUSTERING_ADMINISTRATION_MAIN_COMMAND_LINE_HPP_ */
