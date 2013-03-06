r = require('../build/rethinkdb')

print = console.log

testEquals = (got, expected) ->
    if got?.toArray?
        got.toArray (got2) ->
            print got2, '==', expected
    else
        print got, '==', expected

withConn = (conn, cont) ->
    active = false
    prompt = ->
        if !active and queries.length > 0
            active = true
            [query, expected] = queries.shift()
            query.run conn, (err, res) ->
                if err
                    print ''+err
                else
                    testEquals res, expected
                active = false
                prompt()
        else if !active and queries.length is 0
            conn.close()

    queries = []
    cont (query, expected) ->
        queries.push [query, expected]
        prompt()

r.connect {host:'localhost', port: 28015}, (err, conn) ->
    if err then throw err

    withConn conn, (run) ->
        run r(1), 1
        run r('a'), 'a'
        run r(null), null
        run r(true), true
        run r(false), false
        run r([1,2]), [1,2]
        run r({a:1, b:2}), {a:1, b:2}
        run r.js('1 + 1'), 2
        run r.js('"abc" + 1'), "abc1"
        run r.error('err')
        run r(1).eq(1), true
        run r(1).eq(2), false
        run r(1).ne(1), false
        run r(1).ne(2), true
        run r(1).lt(2), true
        run r(1).lt(0), false
        run r(1).lt(1), false
        run r(1).le(2), true
        run r(1).le(0), false
        run r(1).le(1), true
        run r(1).gt(0), true
        run r(1).gt(2), false
        run r(1).gt(1), false
        run r(1).ge(0), true
        run r(1).ge(2), false
        run r(1).ge(1), true
        run r(true).not(), false
        run r(false).not(), true
        run r(true).not().not(), true
        run r(false).not().not(), false
        run r(1).add(1), 2
        run r(2).sub(1), 1
        run r(2).mul(2), 4
        run r(6).div(2), 3
        run r(4).mod(3), 1
        run r(4).mod(3, 2), 1
        run r([1,2]).append(3), [1,2,3]
        run r([1,2,3]).slice(1, 2), [2,3]
        run r({'a':1, 'b':2}).getAttr('a'), 1
        run r({'a':1, 'b':2})('a'), 1
        run r({'a':1, 'b':2}).contains('a'), true
        run r({'a':1, 'b':2}).contains('c'), false
        run (r(1).do (a) -> a.add(a)), 2
        run r.dbList(), ['bob', 'test']
        run r.db('bob').tableList(), ''
        run r.db('bob').tableList(), ''
        run r.db('bob').table('bob').insert([{id:0}]), ''
        run r.db('bob').table('bob'), ''
        #run r([1,2,3]).forEach (v) -> [r.db('bob').table('bob').insert({id:v})]
        run r.db('bob').table('bob'), ''

        # Error printing
        run r.error('bob')
        run r.error('bob').add(1)
        run r.error('bob').add(1)
        run r(1).add(r.error('bob')).add(4).add(5)
        run r(1).mul(r(2).div(2)).mod(1).sub(r.error('bob'))
        run r(1).do( (a) -> a.add(r.error('bob')) )
        run r([1,2, r.error('bob')])

        run r(2).do (a) -> a.mul(r.error('bob'))
        run r([1,2,3]).map (a)->a.mul(2).div(r.error('bob'))
        run r([1,2,3]).reduce (a,b)->a.add(r.error('bob').mul(b))

        # Type errors
        run r(1).add("a")
        run r(1).map((v) -> v.add(1))
        run r([1,2,3]).map((v) -> v.add([1,2,3]))
        run r([1,2,3]).map(1)
        run r([1,2,3]).append((a)->a)
