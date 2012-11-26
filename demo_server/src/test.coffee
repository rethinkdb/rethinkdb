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

state =
    1:
        test:
            test:
                data:
                    "N1":
                        id: 1
                    "N2":
                        id: 2
                    "N3":
                        id: 3
    2:
        test:
            test:
                data:
                    "N1":
                        id: 1
                        key: 1
                    "N2":
                        id: 2
                        key: 2
                    "N3":
                        id: 3
                        key: 3
                options:
                    'primary_key': 'id'

class Tests
    results: []

    constructor: (queries) ->
        @index = 0
        @queries = queries

    test: =>
        query = @queries[@index]
        if query?
            console.log query.query
            cursor = eval(query.query)
            @callback()[query.callback_name](query.query, cursor, query.expected_result, @)

            ###
            @index++
            @test()
            ###

    display: (success, query, results) ->
        if success is true
            $('#results').append '<li class="result success">'+query+'<span style="float: right">^_^</span></li>'
        else
            $('#results').append '<li class="result fail">'+query+'<span style="float: right">&gt;_&lt;</span><br/>Response:
                <pre>'+JSON.stringify(results, undefined, 2)+'</pre>
                </li>'

    callback: =>
        is_true: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [true])), query, @results
                that.index++
                that.test()
                return false
        is_false: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [false])), query, @results
                that.index++
                that.test()
                return false
        is_int_132: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [132])), query, @results
                that.index++
                that.test()
                return false
        is_float_0_132: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [0.132])), query, @results
                that.index++
                that.test()
                return false
        is_array_132: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [1,3,2])), query, @results
                that.index++
                that.test()
                return false
        is_object: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [{"key_string": "i_am_a_string", "key_int": 132}])), query, @results
                that.index++
                that.test()
                return false
        is_string: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, ['I am a string'])), query, @results
                that.index++
                that.test()
                return false
        db_test_exists: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display demo_server.dbs.test?, query, @results
                that.index++
                that.test()
                return false
        db_other_test_exists: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display demo_server.dbs.other_test?, query, @results
                that.index++
                that.test()
                return false
        db_test_doesnt_exist: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (not demo_server.dbs.test?), query, @results
                that.index++
                that.test()
                return false
        empty_answer: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [])), query, @results
                that.index++
                that.test()
                return false
        two_strings_test_and_other_test: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, ['test', 'other_test'])), query, @results
                that.index++
                that.test()
                return false
        table_test_exists: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display demo_server.dbs.test.tables.test?, query, @results
                that.index++
                that.test()
                return false
        table_test_doesnt_exist: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (not demo_server.dbs?.test.tables.test?), query, @results
                that.index++
                that.test()
                return false
        table_other_test_exists: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display demo_server.dbs?.test.tables.other_test?, query, @results
                that.index++
                that.test()
                return false
        inserted_one: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (@results[0].inserted is 1), query, @results
                that.index++
                that.test()
                return false
        inserted_three: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (@results[0].inserted is 3), query, @results
                that.index++
                that.test()
                return false
        get_one_doc_key_is_int: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [{"id":1, "key":"i_am_a_string"}])), query, @results
                that.index++
                that.test()
                return false
        get_one_doc_key_is_string: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [{"id": "i_am_a_string", "key": 132}])), query, @results
                that.index++
                that.test()
                return false
        doc_key_int_deleted: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (not demo_server.dbs?.test.tables.test['N1']?), query, @results
                that.index++
                that.test()
                return false
        table_point_updated_age: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                record = demo_server.dbs?.test.getTable('test').get("i_am_a_string").asJSON()
                @display (record['age'] is 30 and record['key'] is 321), query, @results
                that.index++
                that.test()
                return false
        is_state_1: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display _.isEqual(demo_server.dbs, state[1]), query, @results
                that.index++
                that.test()
                return false
        is_doc_1: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, [{"id":1}])), query, @results
                that.index++
                that.test()
                return false
        expect_result: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(@results, expected_result)), query, @results
                that.index++
                that.test()
                return false
        expect_state: (query, cursor, expected_state, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(demo_server.dbs?.test.tables.test, expected_state)), query, demo_server.dbs?.test.tables.test
                that.index++
                that.test()
                return false
        two_has_been_replaced: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(demo_server.dbs?.test.tables.test['N2'], {id:2, key:"new_value", other_key:"new_other_value"})), query, @results
                that.index++
                that.test()
                return false
        two_has_been_replaced_and_filter: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(demo_server.dbs?.test.tables.test['N2'], {id:2, key:"new_value", other_key:"new_other_value"}) and @results[0].modified is 1 and @results[0].errors is 1), query, @results
                that.index++
                that.test()
                return false
        table_empty: (query, cursor, expected_result, that) =>
            @results = []
            cursor.next (data) =>
                if data?
                    @results.push data
                    return true
                @display (_.isEqual(demo_server.dbs?.test.tables.test, {})), query, @results
                that.index++
                that.test()
                return false


$(document).ready ->
    window.r = rethinkdb

    window.demo_server = new DemoServer

    r.demoConnect(window.demo_server)


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
            query: 'r.expr(1).eq(r.expr(1)).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(1).eq(r.expr(2)).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr("abc").eq(r.expr("abc")).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr("abc").eq(r.expr("ac")).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr("abc").eq(r.expr("abcd")).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr({id:1, key:2}).eq({id:1, key:2}).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr([{id:1}]).eq([{id:1}]).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr([{id:1}, {id:2}, {id:3}]).eq([{id:1}, {id:2}, {id:3}]).run()'
            callback_name: 'is_true'
        },

        {
            query: 'r.expr(1).ge(r.expr(1)).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(1).ge(r.expr(2)).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(2).ge(r.expr(1)).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(1).gt(r.expr(1)).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(1).gt(r.expr(2)).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(2).gt(r.expr(1)).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(1).lt(r.expr(1)).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(1).lt(r.expr(2)).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(2).lt(r.expr(1)).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(1).le(1).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(1).le(2).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(2).le(1).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(2).add(7).eq(9).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(7).sub(2).eq(5).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(2).mul(7).eq(14).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(15).div(3).eq(5).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(8).mod(3).eq(2).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(true).and(true).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(true).and(false).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(true).or(true).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(true).or(false).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(false).or(false).run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr(false).not().run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr(true).not().run()'
            callback_name: 'is_false'
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
        {
            query: 'r.db("test").table("test").get("i_am_a_string").update({"age":30, "key": 321}).run()'
            callback_name: 'table_point_updated_age'
        },
        {
            query: 'r.expr({id: 1, key:"value", other_key: "other_value"}).pick("id", "other_key").eq({id:1, other_key: "other_value"}).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr({id: 1, key:"value", other_key: "other_value"}).unpick("id", "other_key").eq({key:"value"}).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr({id: 1, key:"value", other_key: "other_value"}).merge({id: 2, key:"new_value", new_key: "other_new_value"}).eq({id: 2, key:"new_value", other_key: "other_value", new_key: "other_new_value"}).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr([1, 2, 3]).append(4).eq([1,2,3,4]).run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr({id:1, key:"value", other_key:"other_value"})("key").eq("value").run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr({id:1, key:"value", other_key:"other_value"}).contains("key").run()'
            callback_name: 'is_true'
        },
        {
            query: 'r.expr({id:1, key:"value", other_key:"other_value"}).contains("key_that_doesnt_exist").run()'
            callback_name: 'is_false'
        },
        {
            query: 'r.expr([1,3,2]).filter(r.expr(true)).run()'
            callback_name: 'is_array_132'
        },
        {
            query: 'r.expr([1,2,3]).filter(r.expr(false)).run()'
            callback_name: 'empty_answer'
        },
        {
            state: deep_copy state['1']
            query: 'r.db("test").table("test").filter(r.expr(true)).run()'
            callback_name: 'is_state_1'
        },
        {
            state: deep_copy state['1']
            query: 'r.db("test").table("test").filter(r("@").eq({id:1})).run()'
            callback_name: 'is_doc_1'
        },
        {
            state: deep_copy state['1']
            query: 'r.db("test").table("test").filter(r("@")("id").eq(1)).run()'
            callback_name: 'is_doc_1'
        },
        {
            state: deep_copy state['1']
            query: 'r.db("test").table("test").filter(r("id").eq(1)).run()'
            callback_name: 'is_doc_1'
        },
        {
            state: deep_copy state['1']
            query: 'r.db("test").table("test").between(2, 3).run()'
            callback_name: 'expect_result'
            expected_result: [{id:2}, {id:3}]
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").get(2).replace({id:2, key:"new_value", other_key:"new_other_value"}).run()'
            callback_name: 'two_has_been_replaced'
        },

        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").del().run()'
            callback_name: 'table_empty'
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").replace({id:2, key:"new_value", other_key:"new_other_value"}).run()'
            callback_name: 'two_has_been_replaced'
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").update({key:0}).run()'
            callback_name: 'expect_state'
            expected_result: {"N1": {id:1, key:0}, "N2": {id:2, key:0}, "N3": {id:3, key:0}}
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").filter(r("id").ge(2)).replace({id:2, key:"new_value", other_key:"new_other_value"}).run()'
            callback_name: 'two_has_been_replaced_and_filter'
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").filter(r("id").ge(2)).del().run()'
            callback_name: 'expect_state'
            expected_result: { "N1": {id:1, key:1} }
        },
        {
            state: deep_copy state['1']
            query: 'r.db("test").table("test").filter(r("id").le(2)).between(2, 3).run()'
            callback_name: 'expect_result'
            expected_result: [{id:2}]
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").filter(r("id").eq(1)).update({key:0}).run()'
            callback_name: 'expect_state'
            expected_result: {"N1": {id:1, key:0}, "N2": {id:2, key:2}, "N3": {id:3, key:3}}
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").filter(function(doc) { return r.branch(doc("id").gt(2), r.expr(true), r.expr(false)) }).run()'
            callback_name: 'expect_result'
            expected_result: [{id:3, key:3}]
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").filter(function(doc) { return doc.eq({id:2, key:2}) }).run()'
            callback_name: 'expect_result'
            expected_result: [{id:2, key:2}]
        },
       {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").map(r("id")).run()'
            callback_name: 'expect_result'
            expected_result: [1, 2, 3]
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").map("id").run()'
            callback_name: 'expect_result'
            expected_result: ['id', 'id', 'id']
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").map(function(doc) { return doc("id")}).run()'
            callback_name: 'expect_result'
            expected_result: [1, 2, 3]
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").map(function(doc) { return doc("id").add(10)}).run()'
            callback_name: 'expect_result'
            expected_result: [11, 12, 13]
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").concatMap(function(doc) { return r.expr([1]) }).run()'
            callback_name: 'expect_result'
            expected_result: [[1, 1, 1]]
        },
        {
            state: deep_copy state['2']
            query: 'r.db("test").table("test").concatMap(function(doc) { return r.expr([r.expr(r("id"))]) }).run()'
            callback_name: 'expect_result'
            expected_result: [[1, 2, 3]]
        },
        {
            state: deep_copy state['1']
            query: 'r.db("test").table("test").orderBy("id").run()'
            callback_name: 'expect_result'
            expected_result: [{id:1}, {id:2}, {id:3}]
        },
        {
            state: deep_copy state['1']
            query: 'r.db("test").table("test").orderBy(r.desc("id")).run()'
            callback_name: 'expect_result'
            expected_result: [{id: 3}, {id: 2}, {id:1}]
        },
    ]
    tests = new Tests queries
    tests.test()
