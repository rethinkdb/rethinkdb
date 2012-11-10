

class Tests
    results: []

    constructor: (queries) ->
        @index = 0
        @queries = queries

    test: =>
        query = @queries[@index]
        if query?
            cursor = eval(query.query)
            @callback()[query.callback_name](query.query, cursor)

            @index++
            @test()

    display: (success, query, results) ->
        if success is true
            $('#results').append '<li class="result success">'+query+'<span style="float: right">^_^</span></li>'
        else
            $('#results').append '<li class="result fail">'+query+'<span style="float: right">&gt;_&lt;</span><br/>Response:
                <pre>'+JSON.stringify(results, undefined, 2)+'</pre>
                </li>'

    callback: =>
        is_true: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [true])), query, @results
                return false
        is_false: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [false])), query, @results
                return false
        is_int_132: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [132])), query, @results
                return false
        is_float_0_132: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [0.132])), query, @results
                return false
        is_array_132: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [1,3,2])), query, @results
                return false
        is_object: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [{"key_string": "i_am_a_string", "key_int": 132}])), query, @results
                return false
        is_string: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, ['I am a string'])), query, @results
                return false
        db_test_exists: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display demo_server.local_server.test?, query, @results
                return false
        db_other_test_exists: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display demo_server.local_server.other_test?, query, @results
                return false
        db_test_doesnt_exist: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (not demo_server.local_server.test?), query, @results
                return false
        empty_answer: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [])), query, @results
                return false
        two_strings_test_and_other_test: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, ['test', 'other_test'])), query, @results
                return false
        table_test_exists: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display demo_server.local_server.test.test?, query, @results
                return false
        table_test_doesnt_exist: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (not demo_server.local_server.test.test?), query, @results
                return false
        table_other_test_exists: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display demo_server.local_server.test.other_test?, query, @results
                return false
        inserted_one: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (@results[0].inserted is 1), query, @results
                return false
        inserted_three: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (@results[0].inserted is 3), query, @results
                return false
        get_one_doc_key_is_int: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [{"id":1, "key":"i_am_a_string"}])), query, @results
                return false
        get_one_doc_key_is_string: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [{"id": "i_am_a_string", "key": 132}])), query, @results
                return false
        doc_key_int_deleted: (query, cursor) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (not demo_server.local_server.test.test[1]?), query, @results
                return false



$(document).ready ->
    window.r = rethinkdb

    window.demo_server = new DemoServer

    r.fake_connect(window.demo_server)

    # Order matters!
    queries = [
        {
            query: 'r.expr(true).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(false).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(132).run()'
            callback_name: 'is_int_132'
        },
        {
            query: 'r.expr(0.132).run()'
            callback_name: 'is_float_0_132'
        },
        {
            query: 'r.expr("I am a string").run()'
            callback_name: 'is_string'
        },
        {
            query: 'r.expr([1,3,2]).run()'
            callback_name: 'is_array_132'
        },
        {
            query: 'r.expr({"key_string": "i_am_a_string", "key_int": 132}).run()'
            callback_name: 'is_object'
        },
        {
            query: 'r.dbCreate("test").run()'
            callback_name: 'db_test_exists'
        },
        {
            query: 'r.dbDrop("test").run()'
            callback_name: 'db_test_doesnt_exist'
        },
        {
            query: 'r.dbList().run() // Empty database'
            callback_name: 'empty_answer'
        },
        {
            query: 'r.dbCreate("test").run()'
            callback_name: 'db_test_exists'
        },
        {
            query: 'r.dbCreate("other_test").run()'
            callback_name: 'db_other_test_exists'
        },
        {
            query: 'r.dbList().run() // Two tables, "test" and "other_test"'
            callback_name: 'two_strings_test_and_other_test'
        },
        {
            query: 'r.db("test").tableCreate("test").run()'
            callback_name: 'table_test_exists'
        },
        {
            query: 'r.db("test").tableDrop("test").run()'
            callback_name: 'table_test_doesnt_exist'
        },
        {
            query: 'r.db("test").tableList().run() // No tables'
            callback_name: 'empty_answer'
        },
        {
            query: 'r.db("test").tableCreate("test").run()'
            callback_name: 'table_test_exists'
        },
        {
            query: 'r.db("test").tableCreate("other_test").run()'
            callback_name: 'table_other_test_exists'
        },
        {
            query: 'r.db("test").tableList().run() // Two tables, "test" and "other_test"'
            callback_name: 'two_strings_test_and_other_test'
        },
        {
            query: 'r.db("test").table("test").run() // Empty table'
            callback_name: 'empty_answer'
        },
        {
            query: 'r.db("test").table("test").insert({"id":1, "key":"i_am_a_string"}).run() // Id provided by the user'
            callback_name: 'inserted_one'
        },
        {
            query: 'r.db("test").table("test").insert({}).run() // No id provided by the user'
            callback_name: 'inserted_one'
        },
        {
            query: 'r.db("test").table("test").insert([{"id": "i_am_a_string", "key": 132}, {}, {}]).run() // Multiple insert'
            callback_name: 'inserted_three'
        },
        {
            query: 'r.db("test").table("test").get(1).run()'
            callback_name: 'get_one_doc_key_is_int'
        },
        {
            query: 'r.db("test").table("test").get("i_am_a_string").run()'
            callback_name: 'get_one_doc_key_is_string'
        },
        {
            query: 'r.db("test").table("test").get(132).run() // This key does not exist. Expecting empty answer'
            callback_name: 'empty_answer'
        },
        {
            query: 'r.db("test").table("test").get(1).del().run()'
            callback_name: 'doc_key_int_deleted'
        },

    ]
    tests = new Tests queries
    tests.test()
