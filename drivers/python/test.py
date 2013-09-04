import rethinkdb as r
r.connect(port = 42865).repl()
r.table_create("foo").run()
r.table("foo").index_create("sid").run()
r.table("foo").insert({"sid" : "a" * 1000}).run()
r.table("foo").insert({"sid" : "b" * 1000}).run()
print list(r.table("foo").order_by(index="sid").run())
