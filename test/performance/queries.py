write_queries = [
    {
        "query": "r.db('test').table(table['name']).get(table['ids'][i]).update({'update_field': 'value'})",
        "tag": "single_update",
        # We just clean what we update using between and the fact that the array table['ids'] is sorted
        "clean": "r.db('test').table(table['name']).between(r.minval, table['ids'][i], right_bound='closed').replace(r.row.without('update_field'))"
    },
    {
        "query": "r.db('test').table(table['name']).get(table['ids'][i]).replace(r.row.merge({'replace_field': 'value'}))",
        "tag": "single_replace",
        "clean": "r.db('test').table(table['name']).between(r.minval, table['ids'][i], right_bound='closed').replace(r.row.without('replace_field'))"
    }
]

delete_queries = [
    {
        "query": "r.db('test').table(table['name']).get(table['ids'][i]).delete()",
        "tag": "single-delete-pk"
    }
]

table_queries = [
    {
        "query": "r.db('test').table(table['name']).get(table['ids'][i])",
        "tag": "single-read_pk"
    },
    {
        "query": "r.db('test').table(table['name']).get_all(str(i), index='field0')",
        "tag": "single_read_sindex0"
    },
    {
        "query": "r.db('test').table(table['name']).get_all(str(i), index='field1')",
        "tag": "single_read_sindex1"
    },
    {
        "query": "r.db('test').table(table['name']).between(table['ids'][i], table['ids'][i+100])",
        "tag": "range_read_sindex1",
        "imax": 100
    },
    {
        "query": "r.db('test').table(table['name']).between(table['ids'][i], table['ids'][i+100], index='field0')",
        "tag": "between_100",
        "imax": 100
    },
    {
        "query": "r.db('test').table(table['name']).filter(r.row['boolean'])",
        "tag": "filter_field_bool"
    },
    {
        "query": "r.db('test').table(table['name']).filter(True)",
        "tag": "filter_field_true"
    },
    {
        "query": "r.db('test').table(table['name']).filter(False)",
        "tag": "filter_field_false"
    },
    {
        "query": "r.db('test').table(table['name']).filter(r.row['missing_field'])",
        "tag": "filter_missing_field"
    },
    {
        "query": "r.db('test').table(table['name']).filter(r.row['field0'].gt('5'))",
        "tag": "filter_string_5"
    },
    {
        "query": "r.db('test').table(table['name']).limit(10).inner_join(r.db('test').table(table['name']), lambda left, right: left['id'] == right['id'])",
        "tag": "inner_join"
    },
    {
        "query": "r.db('test').table(table['name']).limit(10).outer_join(r.db('test').table(table['name']), lambda left, right: left['id'] == right['id'])",
        "tag": "outer_join"
    },
    {
        "query": "r.db('test').table(table['name']).eq_join('id', r.db('test').table(table['name']))",
        "tag": "eq_join"
    },
    {
        "query": "r.db('test').table(table['name']).eq_join('id', r.db('test').table(table['name'])).zip()",
        "tag": "eq_join_zip"
    },
    {
        "query": "r.db('test').table(table['name']).map(r.row['id'])",
        "tag": "map_id"
    },
    {
        "query": "r.db('test').table(table['name']).with_fields('id')",
        "tag": "with_field_id"
    },
    {
        "query": "r.db('test').table(table['name']).with_fields('non_existing_field')",
        "tag": "with_field_non_existing"
    },
    {
        "query": "r.db('test').table(table['name']).concat_map(r.row['array_num'].default([]))",
        "tag": "concat_map_array_num"
    },
    {
        "query": "r.db('test').table(table['name']).concat_map(r.row['array_str'].default([]))",
        "tag": "concat_map_array_str"
    },
    {
        "query": "r.db('test').table(table['name']).limit(900).order_by(r.row['id'])",
        "tag": "order_by_id"
    },
    {
        "query": "r.db('test').table(table['name']).order_by(index='id')",
        "tag": "order_by_id_index"
    },
    {
        "query": "r.db('test').table(table['name']).skip(0)",
        "tag": "skip_0"
    },
    {
        "query": "r.db('test').table(table['name']).skip(100)",
        "tag": "skip_100"
    },
    {
        "query": "r.db('test').table(table['name']).limit(0)",
        "tag": "limit_0"
    },
    {
        "query": "r.db('test').table(table['name']).limit(100)",
        "tag": "limit_100"
    },
    {
        "query": "r.db('test').table(table['name'])[0]",
        "tag": "[0]"
    },
    {
        "query": "r.db('test').table(table['name']).has_fields('id')",
        "tag": "has_fields_id"
    },
    {
        "query": "r.db('test').table(table['name']).has_fields('missing_field')",
        "tag": "has_fields_missing"
    },
    {
        "query": "r.db('test').table(table['name']).info()",
        "tag": "info"
    },
    {
        "query": "r.db('test').table(table['name']).count()",
        "tag": "count"
    },
    {
        "query": "r.db('test').table(table['name']).between(table['ids'][i], table['ids'][i+100]).count()",
        "tag": "count-pk-between",
        "imax": 100
    },
    {
        "query": "r.db('test').table(table['name']).between(table['ids'][i], table['ids'][i+100], index='field0').count()",
        "tag": "count-sindex-between",
        "imax": 100
    },
    {
        "query": "r.db('test').table(table['name']).filter(r.expr(True)).count()",
        "tag": "filter-true-count"
    }
]

constant_queries = [
    # Terms
    "r.expr(123456789)",
    "r.expr('Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer vel est id leo pellentesque tempus. Vivamus condimentum porttitor lobortis. Donec scelerisque vel nisi vitae hendrerit. Curabitur rhoncus orci enim, eu faucibus nulla varius eu. Duis vel massa mi. Etiam non nibh non ligula dapibus congue. Donec sed lorem eu quam bibendum posuere nec lacinia nunc.')",
    "r.expr(None)",
    "r.expr(True)",
    "r.expr(False)",
    "r.expr([0, 1, 2, 3, 4, 5])",
    "r.expr({'a': 'aaaaa', 'b': 'bbbbb', 'c': 'ccccc'})",
    "r.expr({'a': 'aaaaa', 'b': 'bbbbb', 'c': 'ccccc'}).keys()",
    {"query": "r.expr({'a': 'aaaaa', 'c': 'ccccc', 'b': 'bbbbb', 'e': 'eeeee', 'd': 'ddddd', 'g': 'ggggg', 'f': 'fffff', 'i': 'iiiii', 'h': 'hhhhh', 'k': 'kkkkk', 'j': 'jjjjj', 'm': 'mmmmm', 'l': 'lllll', 'o': 'ooooo', 'n': 'nnnnn', 'q': 'qqqqq', 'p': 'ppppp', 's': 'sssss', 'r': 'rrrrr', 'u': 'uuuuu', 't': 'ttttt', 'w': 'wwwww', 'v': 'vvvvv', 'y': 'yyyyy', 'x': 'xxxxx', 'z': 'zzzzz'}).keys()", "tag": "keys for big object"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).insert_at(0, 100)", "tag": "insert_at start"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).insert_at(10, 100)", "tag": "insert_at middle"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).insert_at(20, 100)", "tag": "insert_at end"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).insert_at(20, 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", "tag": "insert_at long string"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).delete_at(0)", "tag": "delet_at start"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).delete_at(10)", "tag": "delete_at middle"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).delete_at(19)", "tag": "delete_at end"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).delete_at(5, 15)", "tag": "delete_at range"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).change_at(0, 999)", "tag": "change_at start"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).change_at(10, 999)", "tag": "change_at middle"},
    {"query": "r.expr([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]).change_at(19, 999)", "tag": "change_at end"},
    {
        "query": "r.expr([0, 1, 2, 3, 4, 5])[1:3]",
        "tag": "slice"
    },
    "r.expr('abcdefghijklmnopqrstuvwxyz').match('^abcdefghijkl')",
    "r.expr('abcdefghijklmnopqrstuvwxyz').match('qrstuvwxyz$')",
    "r.expr('abcdefghijklmnopqrstuvwxyz').match('hijklmnopqrst')",
    "r.expr('abcdefghijklmnopqrstuvwxyz').match('^[abcdef]{3}')",
    "(r.expr(1)+6)",
    "(r.expr('abcdef')+'abcdef')",
    "(r.expr([1,2,3,4,5,6,7,8,9])+[1,2,3,4,5,6,7,8,9])",
    "(r.now()+213414)",
    "(r.expr(12331321)-5884631)",
    "(r.now()-5884631)",
    "(r.now()-(r.now()-12312))",
    "(r.expr(12331321)*5884631)",
    "(r.expr([1,2,3])*5000)",
    "(r.expr(12345353213)/56631)",
    "(r.expr(12345353213)%56631)",
    "(r.expr(True) | r.expr(False))",
    "(r.expr(True) | (r.expr([1])*99000))",
    "(r.expr('abc') == r.expr('abc'))",
    "(r.expr('abc') == 2)",
    "(r.expr('abc') == r.expr(['a', 'b', 'c']))",
    "(r.expr('abc') != 'abc')",
    "(r.expr('aaaaaaaaaaaaaaaaaaaaaaaab') != 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')",
    "(r.expr(1000) > 1)",
    "(r.expr(1000) > 1000)",
    "(r.expr(1000) > 2000)",
    "(r.expr(1000) >= 1)",
    "(r.expr(1000) >= 1000)",
    "(r.expr(1000) >= 2000)",
    "(r.expr(1000) < 1)",
    "(r.expr(1000) < 1000)",
    "(r.expr(1000) < 2000)",
    "(r.expr(1000) <= 1)",
    "(r.expr(1000) <= 1000)",
    "(r.expr(1000) <= 2000)",
    "(~r.expr(True))",
    "(r.expr(True).not_())",
    "(~r.expr(False))",
    "(r.expr(False).not_())",
    # Dates
    "r.now()",
    "r.time(1986, 11, 3, 'Z')",
    "r.epoch_time(531360000)",
    "r.iso8601('1986-11-03T08:30:00-07:00')",
    "r.now().in_timezone('-08:00')",
    "r.now().in_timezone('-08:00').timezone()",
    {"query": "r.now().during(r.time(2013, 12, 1, 'Z'), r.time(2015, 12, 10, 'Z'))", "tag": "during false"},
    {"query": "r.now().during(r.time(2013, 12, 1, 'Z'), r.time(2013, 12, 10, 'Z'))", "tag": "during true end"},
    "r.now().date()",
    "r.now().time_of_day()",
    "r.now().year()",
    "r.now().month()",
    "r.now().day()",
    "r.now().day_of_week()",
    "r.now().day_of_year()",
    "r.now().hours()",
    "r.now().minutes()",
    "r.now().seconds()",
    "r.now().to_iso8601()",
    "r.now().to_epoch_time()",
    "r.expr(1).do(lambda value: value+2)",
    "r.branch(r.expr(False), r.expr(1), r.expr(2))",
    "r.branch(r.expr(True), r.expr(1), r.expr(2))",
    "r.js('Math.random()')",
    "r.expr(1).coerce_to('STRING')",
    "r.expr('1').coerce_to('NUMBER')",
    "r.expr([['a', 'aaaaaa']]).coerce_to('OBJECT')",
    "r.expr({'a': 'aaaaaa'}).coerce_to('ARRAY')",
    "r.expr(1).type_of()",
    "r.expr('str').type_of()",
    "r.expr({'a': 'aaaa'}).type_of()",
    "r.expr([1,2,3]).type_of()",
    "r.expr(1).type_of()",
    "r.expr('str').type_of()",
    "r.expr({'a': 'aaaa'}).type_of()",
    "r.expr([1,2,3]).type_of()",
    "r.json('[1,2,3]').type_of()",
    {
        "query": "r.expr([1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1])",
        "tag": "big array"
    },
    "r.expr({'a': 'aaaaa'})['a'].default('default')",
    "r.expr({'a': 'aaaaa'})['b'].default('default')"
]
