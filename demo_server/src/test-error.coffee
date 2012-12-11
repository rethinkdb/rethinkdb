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
        if @queries[@index]?
            window.r = window.rethinkdb_demo

            try
                cursor_demo = eval(@queries[@index])
            catch err
                @callback_demo err.toString()
                return ''
            
            cursor_demo.collect @callback_demo
        
    callback_demo:(data) =>
        @result_demo = data
        # TODO Remove when the backtraces are implemented
        #@result_demo = @remove_backtraces(@result_demo)


        window.r = window.rethinkdb
        try
            cursor_server = eval(@queries[@index])
        catch err
            @callback_server err.toString()
            return ''

        cursor_server.collect @callback_server

    callback_server: (data) =>
        @result_server = data
        # TODO Remove when the backtraces are implemented
        #@result_server = @remove_backtraces(@result_server)

        # We do not compare the generated keys since they are not the same.
        found_error_in_generated_keys = false
        if @result_demo[0]?.generated_keys? and @result_server[0]?.generated_keys?
            if @result_demo[0].generated_keys.length isnt @result_server[0].generated_keys.length
                found_error_in_generated_keys = true
                @display
                    query: @queries[@index]
                    success: false
                    result_server: @result_server
                    result_demo: @result_demo
            else
                @result_demo[0].generated_keys = [ '...' ]
                @result_server[0].generated_keys = [ '...' ]

        if found_error_in_generated_keys is false
            # We need to overwrite prototype or underscore will unmatch the objects if they are errors.
            for result in @result_demo
                result?.__proto__ = Object.prototype
            for result in @result_server
                result?.__proto__ = Object.prototype

            if _.isEqual(@result_demo, @result_server)
                @display
                    query: @queries[@index]
                    success: true
                    result_server: @result_server
                    result_demo: @result_demo
            else
                @display
                    query: @queries[@index]
                    success: false
                    result_server: @result_server
                    result_demo: @result_demo
        @index++
        @test()

    # We remove the backtraces from the response for the time being since we don't support them on the demo server.
    remove_backtraces: (data) =>
        for doc in data
            if doc?.name? and doc.message? and doc.name is 'Runtime Error'
                lines = doc.message.split('\n')
                doc.message = lines[0]
            if doc?.first_error?
                lines = doc.first_error.split('\nBacktrace')
                doc.first_error = lines[0]
        return data

    newlines = (key, value) ->
        if key is 'message'
            value.replace(/\n/g, '\n')
        else
            value

    display: (args) ->
        query = args.query
        success = args.success
        result_server = JSON.stringify(args.result_server, newlines, 2)
        result_demo = JSON.stringify(args.result_demo, newlines, 2)
        if success is true
            $('#results').append '<li class="result success">'+query+'<span style="float: right">^_^</span>
                </li>'
            ###
            $('#results').append '<li class="result success">'+query+'<span style="float: right">^_^</span>
                <pre>'+JSON.stringify(result_demo, undefined, 2)+'</pre>
                Real server response:
                <pre>'+JSON.stringify(result_server, undefined, 2)+'</pre>
                </li>'
            ###
        else
            $('#err-results').append '<li class="result fail">'+query+'<span style="float: right">&gt;_&lt;</span><br/>Demo server response:
                <pre>'+result_demo+'</pre>
                Real server response:
                <pre>'+result_server+'</pre>
                </li>'


$(document).ready ->
    #TODO test groupedMapReduce
    # Some tests are commented because the real server doesn't implement (yet) object comparison.
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
        #'r.expr({id:1, key:2}).eq({id:1, key:2}).run()',
        #'r.expr([{id:1}]).eq([{id:1}]).run()',
        #'r.expr([{id:1}, {id:2}, {id:3}]).eq([{id:1}, {id:2}, {id:3}]).run()',
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
        'r.db("db_that_doesnt_exist").tableList().run()',
        'r.db("db_that_doesnt_exist").tableCreate("food").run()',
        'r.db("db_that_doesnt_exist").tableDrop("clothes").run()',
        'r.dbCreate("test").run()',
        'r.dbDrop("test").run()',
        'r.dbList().map({ foo: r("@")}).orderBy("foo").run() // Empty database',
        'r.dbCreate("test").run()',
        'r.dbCreate("other_test").run()',
        'r.dbList().map({ foo: r("@")}).orderBy("foo").run() // Two tables, "test" and "other_test"',
        'r.db("test").tableCreate("test").run()',
        'r.db("test").tableDrop("test").run()',
        'r.db("test").tableList().map({ foo: r("@")}).orderBy("foo").run() // No tables',
        'r.db("test").tableCreate("test").run()',
        'r.db("test").tableCreate("other_test").run()',
        'r.db("test").tableList().map({ foo: r("@")}).orderBy("foo").run() // Two tables, "test" and "other_test"',
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
        #'r.db("test").table("test").filter(r("@").eq({id:1})).run()',
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
        'r.db("db_that_does_not_exist").table("table_that_does_not_exist").run()',
        'r.db("test").table("table_that_does_not_exist").run()',
        'r.expr(Infinity).run()',
        'r.expr(-Infinity).run()',
        'r.expr(NaN).run()',
        'r.db("test").tableCreate("test").run() // Table already exists',
        'r.db("test").tableCreate("test").run() // Table already exists',
        'r.db("test").tableCreate("我不懂").run() // Bad name',
        'r.dbCreate("test").run() // db alreay exists',
        'r.dbCreate("while (true) { }").run() // Bad name',
        'r.db("test").tableCreate("test").run() // Table already exists',
        'r.dbDrop("database_that_doesnt_exist").run()'
        'r.db("test").tableDrop("table_that_doesnt_exist").run()'
        'r.db("test").table("test").get(1).del().run()',
        'r.db("test").table("test").get(1).del().run() // Element does\'nt exist',
        'r.db("test").table("test").insert({id:1}).run()',
        'r.db("test").table("test").insert({id:1, tetefsdfdsst:{key: "value"}}).run()',
        'r.db("test").table("test").insert({id: null}).run() // Key is not a number or not string',
        'r.db("test").table("test").insert({id: null}).run() // Key is not a number or not string',
        'r.db("test").table("test").update({attribute_that_doesnt_exist: r("attribute_that_doesnt_exist")}).run() // Update field that does not exist',
        'r.db("test").table("test").update({attribute_that_doesnt_exist: r("attribute_that_doesnt_exist").add(1)}).run() // Update field that does not exist',
        'r.db("test").table("test").insert({id: [1], key: {other_key: "value", more: [1, 2, {a: "a"}]}}).run() // PK is not string or number',
        'r.db("test").table("test").insert([{id: 1}, {id: 2}]).run()',
        'r.db("test").table("test").update({id: 1, key: {other_key: "value", more: [1, 2, {a: "a"}]}}).run() // Try to overwrite a PK',
        'r.db("test").table("test").get("key_that_doesnt_exist").update({id:"key_that_doesnt_exist", key:"whatever"}).run()',
        'r.db("test").table("test").replace({id:"key_that_doesnt_exist", key:"whatever"}).run()',
        'r.db("test").table("test").insert({id:1}).run()',
        'r.db("test").table("test").get(1).replace({id:1, key:"whatever"}).run()',
        'r.db("test").table("test").get(1).replace({id:2, key:"whatever"}).run()',
        'r.db("test").table("test").replace({id:3, key:"whatever"}).run()',
        'r.db("test").table("test").get("key_that_doesnt_exist").replace({id:"key_that_doesnt_exist", key:"whatever"}).run()',
        'r.db("test").table("test").get("random_key_that_doesnt_exist").del().run()',
        'r.db("test").table("test").get([1]).run() // Key is not string or number',
        'r.db("test").table("test").between([1], 2).run()',
        'r.db("test").table("test").between(1, {a: "a"}).run()',
        'r.db("test").table("test").between(1).run() // miss an argument',
        'r.db("test").table("test").between(1, ).run() // unexpected token )',
        'r.db("test").table("test").filter("whatever").run() // non boolean value',
        'r.db("test").tableDrop("test").run()',
        'r.db("test").tableCreate("test").run()',
        'r.db("test").tableCreate("test_join").run()',
        'r.db("test").table("test").insert([{id:1, key: 1}, {id:2, key: 2}, {id:3, key: 3}]).run()',
        'r.db("test").table("test_join").insert([{id:1, join_key: 2}, {id:2, join_key: 2}, {id:3, join_key: 3}]).run()',
        'r.db("test").table("test").innerJoin(r.db("test").table("test_join"), function(test, join_test) { true} ).zip().run() // Not boolean for the join',
        'r.db("test").table("test").innerJoin(r.db("test").table("test_join"), function(test, join_test) {return "whatever"} ).zip().run() // Not boolean for the join',
        'r.db("test").table("test").outerJoin(r.db("test").table("test_join"), function(test, join_test) { true} ).zip().run() // Not boolean for the join',
        'r.db("test").table("test").outerJoin(r.db("test").table("test_join"), function(test, join_test) {return "whatever"} ).zip().run() // Not boolean for the join',
        'r.db("test").table("test").eqJoin("id", r.db("test").table("test_join"), "id").zip().orderBy("id").run()',
        'r.db("test").table("test").eqJoin("id", r.db("test").table("test_join"), "id_other").zip().orderBy("id").run()',
        'r.db("test").table("test").insert([{id: 25}, {id: "test"}, {id: 4}, {id: "i_am_a_string"}]).run()',
        'r.db("test").table("test").orderBy("id").run() // Order with mutliples types',
        'r.db("test").table("test").orderBy("non_existing_key").run()',
        'r.db("test").table("test").insert([{id: 25}, {id: "test"}, {id: 4}, {id: "i_am_a_string"}]).run()',
        'r.db("test").table("test").insert([{id: 25}, {id: "test"}, {id: 4}, {id: "i_am_a_string"}]).run()',
        'r.db("test").table("test").orderBy("id").skip(null).run() // null slice start',
        'r.db("test").table("test").orderBy("id").skip(true).run() // Bad slice start',
        'r.db("test").table("test").orderBy("id").skip(-1).run() // Bad slice start',
        'r.db("test").table("test").orderBy("id").skip(NaN).run() // Bad slice start',
        'r.db("test").table("test").orderBy("id").skip(-Infinity).run() // Bad slice start',
        'r.db("test").table("test").orderBy("id").skip(Infinity).run() // Bad slice start',
        'r.db("test").table("test").orderBy("id").limit(true).run() // Bad slice end',
        'r.db("test").table("test").orderBy("id").limit(null).run() // null slice end',
        'r.db("test").table("test").orderBy("id").limit(-1).run() // Bad slice end',
        'r.db("test").table("test").orderBy("id").limit(NaN).run() // Bad slice end',
        'r.db("test").table("test").orderBy("id").limit(-Infinity).run() // Bad slice end',
        'r.db("test").table("test").orderBy("id").limit(Infinity).run() // Bad slice end',
        'r.db("test").table("test").orderBy("id").slice(true, 4).run() // Bad slice start',
        'r.db("test").table("test").orderBy("id").slice(1, true).run() // Bad slice start/end',
        'r.db("test").table("test").orderBy("id").slice(true, true).run() // Bad slice start/end',
        'r.db("test").table("test").orderBy("id").slice(3, 2).run() // Start > end',
        'r.db("test").table("test").orderBy("id").slice(NaN, 2).run() // Bad slice start',
        'r.db("test").table("test").orderBy("id").slice(3, NaN).run() // Bad slice end',
        'r.db("test").table("test").orderBy("id").slice(null, 2).run() // null slice start',
        'r.db("test").table("test").orderBy("id").slice(3, null).run() // null slice end',
        'r.db("test").table("test").orderBy("id").slice(null, null).run() // null slice start/end',
        'r.db("test").table("test").orderBy("id").skip(2).run()',
        'r.db("test").table("test").orderBy("id").limit(2).run()',
        'r.db("test").table("test").orderBy("id").skip(2, 3).run()',
        'r.db("test").table("test").orderBy("id").nth(2).run()',
        'r.db("test").table("test").orderBy("id").nth(-1).run()',
        'r.db("test").table("test").orderBy("id").nth(NaN).run()',
        'r.db("test").table("test").orderBy("id").nth(null).run()',
        'r.db("test").table("test").orderBy("id").nth().run()',
        'r.db("test").table("test").orderBy("id").nth(Infinity).run()',
        'r.db("test").table("test").pluck("attribute_that_doesnt_exist").run()',
        'r.db("test").table("test").without("attribute_that_doesnt_exist").orderBy("id").run()',
        'r.expr("a_string").union([1]).run()',
        #'r.expr([1]).union("other_string").run()',
        'r.expr([1,2]).union([3,4]).run()',
        'r.expr([1,2,3,4,5]).reduce(0 , function(acc, val) { return acc.add(val) }).run() // Good reduce',
        'r.expr([1,2,3,4,5]).reduce(function() {}, function(acc, val) { return acc.add(val) }).run( // Bad start)',
        'r.expr("test").count().run()',
        'r.expr(2).count().run()',
        'r.expr([1,2,3,4]).count().run()',
        'r.expr([1,2,1]).distinct().run()',
        'r.expr(1).distinct().run()',
        'r.expr("string").distinct().run()',
        'r.db("db_that_doesnt_exist").table("table_that_doesnt_exist").insert({}).run() // Fails because of backtrace?',
        'r.db("test").table("table_that_doesnt_exist").insert({}).run() // Fails because of backtrace?',
        'r.db("test").table("test").groupBy("key_that_doesnt_exist", r.sum("id")).run()',
        'r.db("test").table("test").groupBy("key", r.sum("id")).run()',
        'r.db("test").table("test").groupBy("key_that_doesnt_exist", r.count).run()',
        'r.db("test").table("test").groupBy("id", r.count).orderBy("group").run()',
        'r.db("test").table("test").groupBy("key_that_doesnt_exist", r.sum("id")).run()',
        'r.db("test").table("test").groupBy("key", r.sum("key_that_doesnt_exist")).run()',
        'r.db("test").table("test").groupBy("key", r.avg("key_that_doesnt_exist")).run()',
        'r.expr({id:1, key: 2}).pick("id").run()',
        'r.expr({id:1, key: 2}).pick("id", "key").run()',
        'r.expr({id:1, key: 2}).pick("key_that_doesnt_exist").run()',
        'r.expr({id:1, key: 2}).pick("id", "key_that_doesnt_exist").run()',
        'r.expr(1).pick("id").run()',
        'r.expr({id:1}).pick(1).run()',
        'r.expr({id: 1, key: 2}).unpick("id").run()',
        'r.expr({id: 1}).unpick("id").run()',
        'r.expr({id: 1, key: 2}).unpick("key_that_doesnt_exist").run()',
        'r.expr("a_string!!").unpick("id").run()',
        'r.expr({id: 1, key: 2}).merge({id: 3, other_key: "test"}).run()',
        'r.expr("string!!").merge({id: 3, other_key: "test"}).run()',
        'r.expr({id: 1, key: 2}).merge("test").run()',
        'r.expr(null).merge({id: 1}).run()',
        'r.expr({id: 1, key: 2}).merge(null).run()',
        'r.expr([1,2,3]).append(4).run()',
        'r.expr([1,2,3]).append([4]).run()',
        'r.expr([1,2,3]).append(4, 5).run()',
        'r.expr(2).append(4).run()',
        'r.expr([2]).append(null).run()',
        'r.expr(null).append(2).run()',
        'r.expr({id:1, key: 2})("id").run()',
        'r.expr({id:1, key: 2})("key_that_doesnt_exist").run()',
        'r.expr({id:1, key: 2})(1).run() // Non string key',
        'r.expr("string!!")("1").run()',
        'r.expr(null)("1").run()',
        'r.expr({id:1}).contains("id").run()'
        'r.expr({id:1}).contains("key_that_doesnt_exist").run()'
        'r.expr({id:1}).contains(1).run()'
        'r.expr("string!!").contains("id").run()'
        'r.expr(null).contains("id").run()'
        'r.expr([1]).add([2, 3]).run()',
        'r.expr(1).add(2, 3).run()',
        'r.expr(2).add(3).run()',
        'r.expr(2).add("3").run()',
        'r.expr("2").add(3).run()',
        'r.expr(2).add(null).run()',
        'r.expr(null).add(3).run()',
        'r.expr(1).add([2]).run()',
        'r.expr([1]).add(2).run()',
        'r.expr(3).add(2).add(10).run()'
        'r.expr(10).sub(3).run()'
        'r.expr("string").sub(3).run()'
        'r.expr(10).sub("string").run()'
        'r.expr(10).nul(3).run()'
        'r.expr("string").mul(3).run()'
        'r.expr(10).mul("string").run()'
        'r.expr(10).div(3).run()'
        'r.expr("string").div(3).run()'
        'r.expr(10).div("string").run()'
        'r.expr(10).mod(3).run()'
        'r.expr("string").mod(3).run()'
        'r.expr(10).mod("string").run()'
        'r.expr(true).and(r.expr(false)).run()'
        'r.expr(true).and(r.expr(true)).run()'
        'r.expr(false).and(r.expr(false)).run()'
        'r.expr("string").and(r.expr(true)).run()'
        'r.expr(true).and(r.expr("string")).run()'
        'r.expr(true).or(r.expr(false)).run()'
        'r.expr(true).or(r.expr(true)).run()'
        'r.expr(false).or(r.expr(false)).run()'
        'r.expr("string").or(r.expr(true)).run()'
        'r.expr(true).or(r.expr("string")).run()'
        'r.expr(1).eq(r.expr(1)).run()',
        'r.expr(1).eq(r.expr("string")).run()',
        'r.expr(1).eq(r.expr(null)).run()',
        'r.expr(1).eq(r.expr({id: 1})).run()',
        'r.expr(1).eq(r.expr(2)).run()',
        'r.expr(1).eq(r.expr(NaN)).run()',
        'r.expr(1).eq(r.expr([1])).run()',
        'r.expr(1).eq(r.expr(true)).run()',
        'r.expr("string!!").eq(r.expr(1)).run()',
        'r.expr("string!!").eq(r.expr("string")).run()',
        'r.expr("string!!").eq(r.expr(null)).run()',
        'r.expr("string!!").eq(r.expr({id: 1})).run()',
        'r.expr("string!!").eq(r.expr(2)).run()',
        'r.expr("string!!").eq(r.expr(NaN)).run()',
        'r.expr("string!!").eq(r.expr([1])).run()',
        'r.expr("string!!").eq(false).run()',
        'r.expr([1]).eq(r.expr(1)).run()',
        'r.expr([1]).eq(r.expr("string")).run()',
        'r.expr([1]).eq(r.expr(null)).run()',
        'r.expr([1]).eq(r.expr({id: 1})).run()',
        'r.expr([1]).eq(r.expr(2)).run()',
        'r.expr([1]).eq(r.expr(NaN)).run()',
        'r.expr([1]).eq(r.expr([1])).run()',
        'r.expr([1]).eq(false).run()',
        'r.expr({id:1}).eq(r.expr(1)).run()',
        'r.expr({id:1}).eq(r.expr("string")).run()',
        'r.expr({id:1}).eq(r.expr(null)).run()',
        'r.expr({id:1}).eq(r.expr({id: 1})).run()',
        'r.expr({id:1}).eq(r.expr(2)).run()',
        'r.expr({id:1}).eq(r.expr(NaN)).run()',
        'r.expr({id:1}).eq(r.expr([1])).run()',
        'r.expr({id:1}).eq(false).run()',
        'r.expr(null).eq(r.expr(1)).run()',
        'r.expr(null).eq(r.expr("string")).run()',
        'r.expr(null).eq(r.expr(null)).run()',
        'r.expr(null).eq(r.expr({id: 1})).run()',
        'r.expr(null).eq(r.expr(2)).run()',
        'r.expr(null).eq(r.expr(NaN)).run()',
        'r.expr(null).eq(r.expr([1])).run()',
        'r.expr(null).eq(false).run()',
        'r.expr(true).eq(r.expr(1)).run()',
        'r.expr(true).eq(r.expr("string")).run()',
        'r.expr(true).eq(r.expr(null)).run()',
        'r.expr(true).eq(r.expr({id: 1})).run()',
        'r.expr(true).eq(r.expr(2)).run()',
        'r.expr(true).eq(r.expr(NaN)).run()',
        'r.expr(true).eq(r.expr([1])).run()',
        'r.expr(true).eq(false).run()',
        'r.expr({id: {key: "value", array: [1,2,3]}}).eq({id: {key: "value", array: [1,2,3]}}).run()'
        'r.expr({id: {key: "value", array: [1,2,3]}}).eq({id: {key: "value", array: [1,2,2]}}).run()'
        'r.expr(1).ne(r.expr(1)).run()',
        'r.expr(1).ne(r.expr("string")).run()',
        'r.expr(1).ne(r.expr(null)).run()',
        'r.expr(1).ne(r.expr({id: 1})).run()',
        'r.expr(1).ne(r.expr(2)).run()',
        'r.expr(1).ne(r.expr(NaN)).run()',
        'r.expr(1).ne(r.expr([1])).run()',
        'r.expr(1).ne(r.expr(true)).run()',
        'r.expr("string!!").ne(r.expr(1)).run()',
        'r.expr("string!!").ne(r.expr("string")).run()',
        'r.expr("string!!").ne(r.expr(null)).run()',
        'r.expr("string!!").ne(r.expr({id: 1})).run()',
        'r.expr("string!!").ne(r.expr(2)).run()',
        'r.expr("string!!").ne(r.expr(NaN)).run()',
        'r.expr("string!!").ne(r.expr([1])).run()',
        'r.expr("string!!").ne(false).run()',
        'r.expr([1]).ne(r.expr(1)).run()',
        'r.expr([1]).ne(r.expr("string")).run()',
        'r.expr([1]).ne(r.expr(null)).run()',
        'r.expr([1]).ne(r.expr({id: 1})).run()',
        'r.expr([1]).ne(r.expr(2)).run()',
        'r.expr([1]).ne(r.expr(NaN)).run()',
        'r.expr([1]).ne(r.expr([1])).run()',
        'r.expr([1]).ne(false).run()',
        'r.expr({id:1}).ne(r.expr(1)).run()',
        'r.expr({id:1}).ne(r.expr("string")).run()',
        'r.expr({id:1}).ne(r.expr(null)).run()',
        'r.expr({id:1}).ne(r.expr({id: 1})).run()',
        'r.expr({id:1}).ne(r.expr(2)).run()',
        'r.expr({id:1}).ne(r.expr(NaN)).run()',
        'r.expr({id:1}).ne(r.expr([1])).run()',
        'r.expr({id:1}).ne(false).run()',
        'r.expr(null).ne(r.expr(1)).run()',
        'r.expr(null).ne(r.expr("string")).run()',
        'r.expr(null).ne(r.expr(null)).run()',
        'r.expr(null).ne(r.expr({id: 1})).run()',
        'r.expr(null).ne(r.expr(2)).run()',
        'r.expr(null).ne(r.expr(NaN)).run()',
        'r.expr(null).ne(r.expr([1])).run()',
        'r.expr(null).ne(false).run()',
        'r.expr(true).ne(r.expr(1)).run()',
        'r.expr(true).ne(r.expr("string")).run()',
        'r.expr(true).ne(r.expr(null)).run()',
        'r.expr(true).ne(r.expr({id: 1})).run()',
        'r.expr(true).ne(r.expr(2)).run()',
        'r.expr(true).ne(r.expr(NaN)).run()',
        'r.expr(true).ne(r.expr([1])).run()',
        'r.expr(true).ne(false).run()',
        'r.expr({id: {key: "value", array: [1,2,3]}}).ne({id: {key: "value", array: [1,2,3]}}).run()'
        'r.expr({id: {key: "value", array: [1,2,3]}}).ne({id: {key: "value", array: [1,2,2]}}).run()'
        'r.expr({id:1}).gt(r.expr({id:2})).run()',
        'r.expr({id:1}).gt(r.expr({id:0})).run()',
        'r.expr({id:1}).gt(r.expr({zd:1})).run()',
        'r.expr({id:1}).gt(r.expr({ad:1})).run()',
        'r.expr({id:1}).gt(r.expr({id:1, a:"foo"})).run()',
        'r.expr({id:1}).gt(r.expr({id:1, z:"foo"})).run()',
        'r.expr({id:1}).gt(r.expr({id:2, z:"foo"})).run()',
        'r.expr({id:1}).gt(r.expr({id:0, z:"foo"})).run()',
        'r.expr({id: {key: "value", array: [1,2,3]}}).gt({id: {key: "value", array: [1,2,3]}}).run()'
        'r.expr({id: {key: "value", array: [1,2,3]}}).gt({id: {key: "value", array: [1,2,2]}}).run()'
        'r.expr({id:1}).gt(r.expr(1)).run()',
        'r.expr({id:1}).gt(r.expr("string")).run()',
        'r.expr({id:1}).gt(r.expr(null)).run()',
        'r.expr({id:1}).gt(r.expr(2)).run()',
        'r.expr({id:1}).gt(r.expr(NaN)).run()',
        'r.expr({id:1}).gt(r.expr([1])).run()',
        'r.expr({id:1}).gt(false).run()',
        'r.expr(1).gt(r.expr(1)).run()',
        'r.expr(1).gt(r.expr("string")).run()',
        'r.expr(1).gt(r.expr(null)).run()',
        'r.expr(1).gt(r.expr({id: 1})).run()',
        'r.expr(1).gt(r.expr(2)).run()',
        'r.expr(1).gt(r.expr(NaN)).run()',
        'r.expr(1).gt(r.expr([1])).run()',
        'r.expr(1).gt(r.expr(true)).run()',
        'r.expr("string!!").gt(r.expr(1)).run()',
        'r.expr("string!!").gt(r.expr("string")).run()',
        'r.expr("string!!").gt(r.expr(null)).run()',
        'r.expr("string!!").gt(r.expr({id: 1})).run()',
        'r.expr("string!!").gt(r.expr(2)).run()',
        'r.expr("string!!").gt(r.expr(NaN)).run()',
        'r.expr("string!!").gt(r.expr([1])).run()',
        'r.expr("string!!").gt(false).run()',
        'r.expr([1]).gt(r.expr(1)).run()',
        'r.expr([1]).gt(r.expr("string")).run()',
        'r.expr([1]).gt(r.expr(null)).run()',
        'r.expr([1]).gt(r.expr({id: 1})).run()',
        'r.expr([1]).gt(r.expr(2)).run()',
        'r.expr([1]).gt(r.expr(NaN)).run()',
        'r.expr([1]).gt(r.expr([1])).run()',
        'r.expr([1]).gt(false).run()',
        'r.expr(null).gt(r.expr(1)).run()',
        'r.expr(null).gt(r.expr("string")).run()',
        'r.expr(null).gt(r.expr(null)).run()',
        'r.expr(null).gt(r.expr({id: 1})).run()',
        'r.expr(null).gt(r.expr(2)).run()',
        'r.expr(null).gt(r.expr(NaN)).run()',
        'r.expr(null).gt(r.expr([1])).run()',
        'r.expr(null).gt(false).run()',
        'r.expr(true).gt(r.expr(1)).run()',
        'r.expr(true).gt(r.expr("string")).run()',
        'r.expr(true).gt(r.expr(null)).run()',
        'r.expr(true).gt(r.expr({id: 1})).run()',
        'r.expr(true).gt(r.expr(2)).run()',
        'r.expr(true).gt(r.expr(NaN)).run()',
        'r.expr(true).gt(r.expr([1])).run()',
        'r.expr(true).gt(false).run()',
        'r.expr({id:1}).ge(r.expr(1)).run()',
        'r.expr({id:1}).ge(r.expr("string")).run()',
        'r.expr({id:1}).ge(r.expr(null)).run()',
        'r.expr({id:1}).ge(r.expr(2)).run()',
        'r.expr({id:1}).ge(r.expr(NaN)).run()',
        'r.expr({id:1}).ge(r.expr([1])).run()',
        'r.expr({id:1}).ge(false).run()',
        'r.expr(1).ge(r.expr(1)).run()',
        'r.expr(1).ge(r.expr("string")).run()',
        'r.expr(1).ge(r.expr(null)).run()',
        'r.expr(1).ge(r.expr({id: 1})).run()',
        'r.expr(1).ge(r.expr(2)).run()',
        'r.expr(1).ge(r.expr(NaN)).run()',
        'r.expr(1).ge(r.expr([1])).run()',
        'r.expr(1).ge(r.expr(true)).run()',
        'r.expr("string!!").ge(r.expr(1)).run()',
        'r.expr("string!!").ge(r.expr("string")).run()',
        'r.expr("string!!").ge(r.expr(null)).run()',
        'r.expr("string!!").ge(r.expr({id: 1})).run()',
        'r.expr("string!!").ge(r.expr(2)).run()',
        'r.expr("string!!").ge(r.expr(NaN)).run()',
        'r.expr("string!!").ge(r.expr([1])).run()',
        'r.expr("string!!").ge(false).run()',
        'r.expr([1]).ge(r.expr(1)).run()',
        'r.expr([1]).ge(r.expr("string")).run()',
        'r.expr([1]).ge(r.expr(null)).run()',
        'r.expr([1]).ge(r.expr({id: 1})).run()',
        'r.expr([1]).ge(r.expr(2)).run()',
        'r.expr([1]).ge(r.expr(NaN)).run()',
        'r.expr([1]).ge(r.expr([1])).run()',
        'r.expr([1]).ge(false).run()',
        'r.expr(null).ge(r.expr(1)).run()',
        'r.expr(null).ge(r.expr("string")).run()',
        'r.expr(null).ge(r.expr(null)).run()',
        'r.expr(null).ge(r.expr({id: 1})).run()',
        'r.expr(null).ge(r.expr(2)).run()',
        'r.expr(null).ge(r.expr(NaN)).run()',
        'r.expr(null).ge(r.expr([1])).run()',
        'r.expr(null).ge(false).run()',
        'r.expr(true).ge(r.expr(1)).run()',
        'r.expr(true).ge(r.expr("string")).run()',
        'r.expr(true).ge(r.expr(null)).run()',
        'r.expr(true).ge(r.expr({id: 1})).run()',
        'r.expr(true).ge(r.expr(2)).run()',
        'r.expr(true).ge(r.expr(NaN)).run()',
        'r.expr(true).ge(r.expr([1])).run()',
        'r.expr(true).ge(false).run()',
        'r.expr({id:1}).le(r.expr(1)).run()',
        'r.expr({id:1}).le(r.expr("string")).run()',
        'r.expr({id:1}).le(r.expr(null)).run()',
        'r.expr({id:1}).le(r.expr(2)).run()',
        'r.expr({id:1}).le(r.expr(NaN)).run()',
        'r.expr({id:1}).le(r.expr([1])).run()',
        'r.expr({id:1}).le(false).run()',
        'r.expr(1).le(r.expr(1)).run()',
        'r.expr(1).le(r.expr("string")).run()',
        'r.expr(1).le(r.expr(null)).run()',
        'r.expr(1).le(r.expr({id: 1})).run()',
        'r.expr(1).le(r.expr(2)).run()',
        'r.expr(1).le(r.expr(NaN)).run()',
        'r.expr(1).le(r.expr([1])).run()',
        'r.expr(1).le(r.expr(true)).run()',
        'r.expr("string!!").le(r.expr(1)).run()',
        'r.expr("string!!").le(r.expr("string")).run()',
        'r.expr("string!!").le(r.expr(null)).run()',
        'r.expr("string!!").le(r.expr({id: 1})).run()',
        'r.expr("string!!").le(r.expr(2)).run()',
        'r.expr("string!!").le(r.expr(NaN)).run()',
        'r.expr("string!!").le(r.expr([1])).run()',
        'r.expr("string!!").le(false).run()',
        'r.expr([1]).le(r.expr(1)).run()',
        'r.expr([1]).le(r.expr("string")).run()',
        'r.expr([1]).le(r.expr(null)).run()',
        'r.expr([1]).le(r.expr({id: 1})).run()',
        'r.expr([1]).le(r.expr(2)).run()',
        'r.expr([1]).le(r.expr(NaN)).run()',
        'r.expr([1]).le(r.expr([1])).run()',
        'r.expr([1]).le(false).run()',
        'r.expr(null).le(r.expr(1)).run()',
        'r.expr(null).le(r.expr("string")).run()',
        'r.expr(null).le(r.expr(null)).run()',
        'r.expr(null).le(r.expr({id: 1})).run()',
        'r.expr(null).le(r.expr(2)).run()',
        'r.expr(null).le(r.expr(NaN)).run()',
        'r.expr(null).le(r.expr([1])).run()',
        'r.expr(null).le(false).run()',
        'r.expr(true).le(r.expr(1)).run()',
        'r.expr(true).le(r.expr("string")).run()',
        'r.expr(true).le(r.expr(null)).run()',
        'r.expr(true).le(r.expr({id: 1})).run()',
        'r.expr(true).le(r.expr(2)).run()',
        'r.expr(true).le(r.expr(NaN)).run()',
        'r.expr(true).le(r.expr([1])).run()',
        'r.expr(true).le(false).run()',
        'r.expr({id:1}).lt(r.expr(1)).run()',
        'r.expr({id:1}).lt(r.expr("string")).run()',
        'r.expr({id:1}).lt(r.expr(null)).run()',
        'r.expr({id:1}).lt(r.expr(2)).run()',
        'r.expr({id:1}).lt(r.expr(NaN)).run()',
        'r.expr({id:1}).lt(r.expr([1])).run()',
        'r.expr({id:1}).lt(false).run()',
        'r.expr(1).lt(r.expr(1)).run()',
        'r.expr(1).lt(r.expr("string")).run()',
        'r.expr(1).lt(r.expr(null)).run()',
        'r.expr(1).lt(r.expr({id: 1})).run()',
        'r.expr(1).lt(r.expr(2)).run()',
        'r.expr(1).lt(r.expr(NaN)).run()',
        'r.expr(1).lt(r.expr([1])).run()',
        'r.expr(1).lt(r.expr(true)).run()',
        'r.expr("string!!").lt(r.expr(1)).run()',
        'r.expr("string!!").lt(r.expr("string")).run()',
        'r.expr("string!!").lt(r.expr(null)).run()',
        'r.expr("string!!").lt(r.expr({id: 1})).run()',
        'r.expr("string!!").lt(r.expr(2)).run()',
        'r.expr("string!!").lt(r.expr(NaN)).run()',
        'r.expr("string!!").lt(r.expr([1])).run()',
        'r.expr("string!!").lt(false).run()',
        'r.expr([1]).lt(r.expr(1)).run()',
        'r.expr([1]).lt(r.expr("string")).run()',
        'r.expr([1]).lt(r.expr(null)).run()',
        'r.expr([1]).lt(r.expr({id: 1})).run()',
        'r.expr([1]).lt(r.expr(2)).run()',
        'r.expr([1]).lt(r.expr(NaN)).run()',
        'r.expr([1]).lt(r.expr([1])).run()',
        'r.expr([1]).lt(false).run()',
        'r.expr(null).lt(r.expr(1)).run()',
        'r.expr(null).lt(r.expr("string")).run()',
        'r.expr(null).lt(r.expr(null)).run()',
        'r.expr(null).lt(r.expr({id: 1})).run()',
        'r.expr(null).lt(r.expr(2)).run()',
        'r.expr(null).lt(r.expr(NaN)).run()',
        'r.expr(null).lt(r.expr([1])).run()',
        'r.expr(null).lt(false).run()',
        'r.expr(true).lt(r.expr(1)).run()',
        'r.expr(true).lt(r.expr("string")).run()',
        'r.expr(true).lt(r.expr(null)).run()',
        'r.expr(true).lt(r.expr({id: 1})).run()',
        'r.expr(true).lt(r.expr(2)).run()',
        'r.expr(true).lt(r.expr(NaN)).run()',
        'r.expr(true).lt(r.expr([1])).run()',
        'r.expr(true).lt(false).run()',
        'r.expr(true).not().run()',
        'r.expr(false).not().run()',
        'r.expr("true").not().run()',
        'r.expr("false").not().run()',
        'r.expr("string").not().run()',
        'r.expr({id: 1}).not().run()',
        'r.expr([1]).not().run()',
        'r.expr(null).not().run()',
        'r.db("test").tableDrop("test").run()'
        'r.db("test").tableCreate("test").run()'
        'r.let({ironman: r.db("test").table("test").get("IronMan")}, r.letVar("variable_not_defined")("name")).run()'
        'r.let({ironman: r.db("test").table("test").get("IronMan")}, r.letVar("ironman")("name")).run()'
        'r.let({ironman: r.db("test").table("test").get("IronMan")}, r.letVar("ironman")("name_that_we_cant_find")).run()'
        'r.db("test").table("test").insert([{id:1}, {id:2}, {id:3}]).run()',
        'r.db("test").table("test").filter(r.branch(r("id").eq(1), r.expr(true), r.expr(false))).run()'
        'r.db("test").table("test").filter(r.branch(r("id").eq(1), "stupid_string", r.expr(false))).run()'
        'r.db("test").table("test").filter(r.branch(r("id").eq(1), r.expr(true), "stupid_string")).run()'
        'r.db("test").table("test").filter(r.branch(r("id"), r.expr(true), r.expr(false))).run()'
        'r.db("test").table("test").forEach( function(doc) { return r.db("test").table("test").get( doc("id") ) } ).run()'
        'r.db("test").table("test").forEach( function(doc) { return r.db("test").table("test").insert( {id: doc("id").add(100)} )} ).run()'
        'r.db("test").table("test").filter(r.branch(r("id").eq(1), r.expr(true), r.error("impossible code path"))).run()'
        'r.db("test").table("test").filter(r.branch(r("id").eq(1), r.expr(true), r.error(true))).run()'
        'r.db("test").tableCreate("test").run()',
        'r.db("test").table("test").orderBy("id").nth(1).run()'
        'r.db("test").table("test").streamToArray().orderBy("id").run()'
        'r.db("test").table("test").streamToArray().orderBy("id").nth(1).run()'
        'r.db("test").table("test").streamToArray().arrayToStream().orderBy("id").run()'
        'r.js("\'str1\' + \'str2\'").run()'
    ]

    # Connect the two drivers
    # The demo server first
    window.demo_server = new DemoServer
    rethinkdb_demo.demoConnect(window.demo_server)

    # Then the real server
    server =
        host: 'localhost'
        port: 8080
    rethinkdb.connect server, ->
        # Clean the database test
        rethinkdb.dbList().run().collect (dbs) ->
            for db in dbs
                rethinkdb.dbDrop(db).run()
            rethinkdb.dbCreate('test').run().collect ->
                tests = new Tests queries
                tests.test()
