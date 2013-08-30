import rethinkdb as r
r.connect(port = 42865).repl()
r.table_create("foo").run()
r.table("foo").index_create("tags", lambda x: x["tags"], tags=True).run()
r.table("foo").insert({"tags" : [1,2,3]}).run()
