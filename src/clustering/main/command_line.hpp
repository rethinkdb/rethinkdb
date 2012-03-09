#ifndef __CLUSTERING_ADMINISTRATION_COMMAND_LINE_HPP__
#define __CLUSTERING_ADMINISTRAITON_COMMAND_LINE_HPP__

int main_rethinkdb_create(int argc, char *argv[]);
int main_rethinkdb_serve(int argc, char *argv[]);
int main_rethinkdb_porcelain(int argc, char *argv[]);

void help_rethinkdb_create();
void help_rethinkdb_serve();

#endif /* __CLUSTERING_ADMINISTRAITON_COMMAND_LINE_HPP__ */
