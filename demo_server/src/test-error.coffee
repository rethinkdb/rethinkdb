# That's not a perfect version of deep copy, but it's enough for the kind of object we have
deep_copy = (obj) =>
    if typeof obj is 'object'
        if Object.prototype.toString.call(obj) is '[object Array]'
            result = []
            for element, i in obj
                result.push deep_copy element
            return result
        else if obj is null
            return null
        else
            result = {}
            for key, value of obj
                result[key] = deep_copy value
            return result
    else
        return obj

# TODO
# Refactor the test functions

collection = [
    [{id:1}, {id:2}, {id:3}],
    [{id:1, key: 4}, {id:2, key: 5}, {id:3, key: 6}],
    [{id:2, key: 3}, {id:5, key: 3}, {id:4, key: 1}, {id:1, key:1}, {id:3, key: 2}]
]

class Tests
    results: []

    constructor: (queries) ->
        @index = 0
        @queries = queries

    test: =>
        query = @queries[@index]
        that = @

        if query?
            window.r = window.rethinkdb_demo
            cursor_demo = eval(query)

            cursor_demo.collect (data) ->
                result_demo = data

                window.r = window.rethinkdb
                cursor_server = eval(query)
                cursor_server.collect (data) ->
                    result_server = data

                    # We do not compare the generated keys since they are not the same.
                    found_error_in_generated_keys = false
                    if result_demo[0]?.generated_keys? and result_server[0]?.generated_keys?
                        if result_demo[0].generated_keys.length isnt result_server[0].generated_keys.length
                            found_error_in_generated_keys = true
                            that.display
                                query: query
                                success: false
                                result_server: result_server
                                result_demo: result_demo
                        else
                            result_demo[0].generated_keys = [ '...' ]
                            result_server[0].generated_keys = [ '...' ]

                    if found_error_in_generated_keys is false
                        if _.isEqual(result_demo, result_server)
                            that.display
                                query: query
                                success: true
                        else
                            that.display
                                query: query
                                success: false
                                result_server: result_server
                                result_demo: result_demo
                    that.index++
                    that.test()


    display: (args) ->
        query = args.query
        success = args.success
        if success is true
            $('#results').append '<li class="result success">'+query+'<span style="float: right">^_^</span></li>'
        else
            result_server = args.result_server
            result_demo = args.result_demo

            $('#results').append '<li class="result fail">'+query+'<span style="float: right">&gt;_&lt;</span><br/>Demo server response:
                <pre>'+JSON.stringify(result_demo, undefined, 2)+'</pre>
                Real server response:
                <pre>'+JSON.stringify(result_server, undefined, 2)+'</pre>
                </li>'


$(document).ready ->
    queries = [
        'r.expr(true).run()',
        'r.expr(false).run()',
        'r.expr(132).run()',
        'r.expr(0.132).run()',
        'r.expr("I am a string").run()',
        'r.expr([1,3,2]).run()',
        'r.expr({"key_string": "i_am_a_string", "key_int": 132}).run()',
        'r.expr(1).eq(r.expr(1)).run()',
        'r.expr(1).eq(r.expr(2)).run()',
        'r.expr("abc").eq(r.expr("abc")).run()',
        'r.expr("abc").eq(r.expr("ac")).run()',
        'r.expr("abc").eq(r.expr("abcd")).run()',
        'r.expr({id:1, key:2}).eq({id:1, key:2}).run()',
        'r.expr([{id:1}]).eq([{id:1}]).run()',
        'r.expr([{id:1}, {id:2}, {id:3}]).eq([{id:1}, {id:2}, {id:3}]).run()',
        'r.expr(1).ge(r.expr(1)).run()',
        'r.expr(1).ge(r.expr(2)).run()',
        'r.expr(2).ge(r.expr(1)).run()',
        'r.expr(1).gt(r.expr(1)).run()',
        'r.expr(1).gt(r.expr(2)).run()',
        'r.expr(2).gt(r.expr(1)).run()',
        'r.expr(1).lt(r.expr(1)).run()',
        'r.expr(1).lt(r.expr(2)).run()',
        'r.expr(2).lt(r.expr(1)).run()',
        'r.expr(1).le(1).run()',
        'r.expr(1).le(2).run()',
        'r.expr(2).le(1).run()',
        'r.expr(2).add(7).eq(9).run()',
        'r.expr(7).sub(2).eq(5).run()',
        'r.expr(2).mul(7).eq(14).run()',
        'r.expr(15).div(3).eq(5).run()',
        'r.expr(8).mod(3).eq(2).run()',
        'r.expr(true).and(true).run()',
        'r.expr(true).and(false).run()',
        'r.expr(true).or(true).run()',
        'r.expr(true).or(false).run()',
        'r.expr(false).or(false).run()',
        'r.expr(false).not().run()',
        'r.expr(true).not().run()',
        'r.expr({id: 1, key:"value", other_key: "other_value"}).pick("id", "other_key").run()',
        'r.expr({id: 1, key:"value", other_key: "other_value"}).unpick("id", "other_key").run()',
        'r.expr({id: 1, key:"value", other_key: "other_value"}).merge({id: 2, key:"new_value", new_key: "other_new_value"}).run()',
        'r.dbCreate("test").run()',
        'r.dbDrop("test").run()',
        'r.dbList().run() // Empty database',
        'r.dbCreate("test").run()',
        'r.dbCreate("other_test").run()',
        'r.dbList().run() // Two tables, "test" and "other_test"',
        'r.db("test").tableCreate("test").run()',
        'r.db("test").tableDrop("test").run()',
        'r.db("test").tableList().run() // No tables',
        'r.db("test").tableCreate("test").run()',
        'r.db("test").tableCreate("other_test").run()',
        'r.db("test").tableList().run() // Two tables, "test" and "other_test"',
        'r.db("test").table("test").run() // Empty table',
        'r.db("test").table("test").insert({}).run() // No id provided by the user',
        'r.db("test").table("test").del().run()',
        'r.db("test").table("test").run()',
        'r.db("test").table("test").insert({"id":1, "key":"i_am_a_string"}).run() // Id provided by the user',
        'r.db("test").table("test").insert([{"id": "i_am_a_string", "key": 132}, {}, {}]).run() // Multiple insert',
        'r.db("test").table("test").del().run()',
        'r.db("test").table("test").insert([{"id": "i_am_a_string", "key": 132}, {id: "03df088b-d77c-13d6-9eb0-7bd34d96696b"}, {id: "92bd504c-1a7b-7112-c0c6-c68a0d700494"}]).run()',
        'r.db("test").table("test").get(1).run()',
        'r.db("test").table("test").get("i_am_a_string").run()',
        'r.db("test").table("test").get(132).run() // This key does not exist. Expecting empty answer',
        'r.db("test").table("test").insert({id:1}).run()',
        'r.db("test").table("test").get(1).del().run()',
        'r.db("test").table("test").get("i_am_a_string").update({"age":30, "key": 321}).run()',
        'r.expr([1, 2, 3]).append(4).eq([1,2,3,4]).run()',
        'r.expr({id:1, key:"value", other_key:"other_value"})("key").eq("value").run()',
        'r.expr({id:1, key:"value", other_key:"other_value"}).contains("key").run()',
        'r.expr({id:1, key:"value", other_key:"other_value"}).contains("key_that_doesnt_exist").run()',
        'r.expr([1,3,2]).filter(r.expr(true)).run()',
        'r.expr([1,2,3]).filter(r.expr(false)).run()',
        'r.db("test").table("test").filter(r.expr(true)).orderBy("id").run()',
        'r.db("test").table("test").filter(r("@").eq({id:1})).run()',
        'r.db("test").table("test").filter(r("@")("id").eq(1)).run()',
        'r.db("test").table("test").filter(r("id").eq(1)).run()',
        'r.db("test").table("test").between(2, 3).run()',
        'r.db("test").table("test").del().run()'
        'r.db("test").table("test").insert([{id:1}, {id:2}, {id:3}]).run()',
        'r.db("test").table("test").get(2).replace({id:2, key:"new_value", other_key:"new_other_value"}).run();',
        'r.db("test").table("test").get(2).run()',
        'r.db("test").table("test").del().run()',
        'r.db("test").table("test").run()',
        'r.db("test").table("test").replace({id:2, key:"new_value", other_key:"new_other_value"}).run(); r.db("test").table("test").orderBy("id").run()',
        'r.db("test").table("test").update({key:0}).run(); r.db("test").table("test").run()',
        'r.db("test").table("test").filter(r("id").ge(2)).replace({id:2, key:"new_value", other_key:"new_other_value"}).run(); r.db("test").table("test").orderBy("id").run()',
        'r.db("test").table("test").filter(r("id").ge(2)).del().run(); r.db("test").table("test").run()',
        'r.db("test").table("test").filter(r("id").le(2)).between(2, 3).run()',
        'r.db("test").table("test").filter(r("id").eq(1)).update({key:0}).run(); r.db("test").table("test").run()',
        'r.db("test").table("test").filter(function(doc) { return r.branch(doc("id").gt(2), r.expr(true), r.expr(false)) }).run()',
        'r.db("test").table("test").filter(function(doc) { return doc.eq({id:2, key:5}) }).run()',
        'r.db("test").table("test").map(r("id")).run()',
        'r.db("test").table("test").map("id").run()',
        'r.db("test").table("test").map(function(doc) { return doc("id")}).run()',
        'r.db("test").table("test").concatMap(function(doc) { return r.expr([r.expr(r("id"))]) }).run()',
        'r.db("test").table("test").skip(2).run()',
        'r.db("test").table("test").limit(2).run()',
        'r.db("test").table("test").skip(1).limit(1).run()',
        'r.db("test").table("test").limit(1).skip(1).run()',
        'r.db("test").table("test").orderBy("key", "id").run()',
        'r.db("test").table("test").orderBy(r.desc("key"), r.asc("id")).run()',
    ]

    # Connect the two drivers
    # The demo server first
    window.demo_server = new DemoServer
    rethinkdb_demo.demoConnect(window.demo_server)

    # Then the real server
    server =
        host: 'newton'
        port: 23000
    rethinkdb.connect server, ->
        # Clean the database test
        rethinkdb.dbList('test').run().collect (dbs) ->
            for db in dbs
                rethinkdb.dbDrop(db).run()
        tests = new Tests queries
        tests.test()
