### Steps For Migrating Your Cluster

1. Run `import_export.rb --export --host HOST --port PORT`, where HOST:PORT is
the address of a machine in your cluster.  This will write everything in your
cluster to disk, so make sure you have enough space.

2. Upgrade RethinkDB on all your servers, then move or delete all your
`rethinkdb_data` directories (they are incompatible with the new version).

3. Run `import_export.rb --import --host HOST --port PORT`.  This will re-import
all the data on disk into your database.  You must run this in the same
directory you ran the `import_export.rb --export` command, or else specify the
directory to read from with `import_export.rb --import DIR`.  You will also have
to specify the directory if you exported from more than one RethinkDB cluster.

3.a. If any of your tables have a nonstandard primary key (anything except
`id`), you need to tell the script about the nonstandard keys while importing.
(We stupidly forgot to build a way to fetch the primary key of a table into the
original RDB protocol; this will be fixed in future versions.)  You can specify
this with the `--primary-keys` option, which takes a JSON document of the form
`{"db_name.table_name": "key_name, ...}`.

### Simple Example

Alice is running a single RethinkDB server on localhost with the default ports.
She does the following:

```
~ $ mkdir rethinkdb_migration