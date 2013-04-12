# RQL docs system

The rethinkdb.com website displays documentation for the RQL query language
from a specially formatted json file that gives descriptions and code examples
for each of the three supported languages.

This json file is generated from a set of yaml source files located in this
directory.

## How to generate the json file

To generate the json output file simply run make. To specify the output file
location set the `JSON_OUT` make variable.

```bash
$ make JSON_OUT=~/rethinkdb-www/_site/assets/js/rql_docs.json
```

## How to edit the documentation

New sections can be added in any file. Simply add a top level property called
`sections` and give a list of new section objects.

```yaml
sections:
  - tag: db_admin
    name: Manipulating databases
    description: These commands allow database manipulation.
    order: 1 # if omitted this section is placed last
```

Similarly, new commands can be specified anywhere by adding a top level property
to the file called `commands` and giving a list of new commands.

For commands almost any field can be overriden for a specific language. Either
the field itself may be replaced with an object giving `py`, `js`, and `rb`
fields or a field called `py`, `js`, or `rb` may be defined in the command with
an internal structure that mirrors the structure of the command object itself
giving overrides for any and as many fields as you'd like.

```yaml
commands:
  - tag: db_create
    section: db_admin # gives tag of section this command belongs to
    description: |
      Create a database. A RethinkDB database is a collection of tables,
      similar to relational databases.<br /><br />If successful, the operation returns
      an object: <code>{created: 1}</code>. If a database with the same name already
      exists the operation throws <code>RqlRuntimeError</code>.

    # Used to display this the signature of this command, e.g. `r.db_create(db_name) -> json`
    body: db_name
    parent: r
    returns: json

    examples:
      - description: Create a database named 'superheroes'.
        code: r.db_create('superheroes').run(conn)

    # Overrides above values for the given language
    js:
      name: dbCreate
      body: dbName
      examples:
        0: # Give index of example to override
          code: r.dbCreate('superheroes').run(conn)
          can_try: true
          dataset: marvel
```

## How to validate the examples

Validating examples means passing them through the appropriate interpreter and driver
to ensure that they are accepted as valid RQL. Validing will catch syntax errors and
certain kinds of API usage errors. It cannot catch errors that would otherwise be
reported by the server though since the queries go into a black hole.

To run the validation script simply run `make validate` in this directory or
`make validate -C <rethinkdb-src>/test/rql_test/docs/`.
