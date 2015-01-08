// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_COMMAND_LINE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_COMMAND_LINE_HPP_

void print_version_message();

int main_rethinkdb_create(int argc, char *argv[]);
int main_rethinkdb_serve(int argc, char *argv[]);
int main_rethinkdb_proxy(int argc, char *argv[]);
int main_rethinkdb_porcelain(int argc, char *argv[]);
int main_rethinkdb_export(int argc, char *argv[]);
int main_rethinkdb_import(int argc, char *argv[]);
int main_rethinkdb_dump(int argc, char *argv[]);
int main_rethinkdb_restore(int argc, char *argv[]);
int main_rethinkdb_index_rebuild(int argc, char *argv[]);

void help_rethinkdb_create();
void help_rethinkdb_serve();
void help_rethinkdb_proxy();
void help_rethinkdb_porcelain();
void help_rethinkdb_export();
void help_rethinkdb_import();
void help_rethinkdb_dump();
void help_rethinkdb_restore();
void help_rethinkdb_index_rebuild();

#endif /* CLUSTERING_ADMINISTRATION_MAIN_COMMAND_LINE_HPP_ */
