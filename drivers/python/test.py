import rethinkdb as r
r.connect(port = 42865).repl()
r.table_drop("foo").run()
r.table_create("foo").run()
print r.table("foo").index_create("sid", lambda x: r.js("1")).run()
