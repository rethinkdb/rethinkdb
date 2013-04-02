system = require('system')
dir = system.env.RETHINKDB+'/test/webui'

{ Test } = require(dir+'/include/casper-common')
test = new Test
    dir: dir

casper = test.casper
casper.start test.rdb_server, ->
    @test.assertTitle('RethinkDB Administration Console')
    @test.assertExists '#nav-dashboard > a', 'Dashboard link available'
    @test.assertExists '#nav-namespaces > a', 'Tables link available'
    @test.assertExists '#nav-servers > a', 'Servers link available'
    @test.assertExists '#nav-dataexplorer > a', 'Dataexplorer link available'
    @test.assertExists '#nav-logs > a', 'Logs link available'
    @test.assertExists '#nav-search > .search-box', 'Search box available'
    # Testing the topbar (=sidebar)
    @test.assertMatch @evaluate(-> return $('.span3.client-connection-status').find('h4').html()), /^\w+$/, 'Connected machine\'s name available'
    @test.assertEvalEquals ( -> return $('.span3.issues').find('h4').html()), 'No issues', 'No issue on the top bar'
    @test.assertEval ( ->
        result = /(\d+)\/(\d+) reachable/.exec $('.span3.servers-connected').find('h4').html()
        return result[1]? and result[1] is result[2]
    ), 'All servers reported to be reachable'
    @test.assertEval ( ->
        result = /(\d+)\/(\d+) reachable/.exec $('.span3.datacenters-connected').find('h4').html()
        return result[1]? and result[1] is result[2]
    ), 'All datacenters reported to be reachable'
    # Testing dashboard
    @test.assertEval ( -> return $('.no-problems-detected').length is 4), 'No issue reported on the dashboard'
    # Testing graph
    @test.assertEval ( -> return $('canvas').length is 1), 'Canvas available'
    # Testing logs
    @test.assertEval ( -> return $('.log-entry').length > 0), 'Logs available'
    @test.assertExists '.btn.btn-primary.view-logs', 'Link to logs available'


casper.then ->
    # Go to the data explorer
    @click '#nav-dataexplorer > a'

    to_do = ->
        casper.test.assertExists '.CodeMirror', 'CodeMirror loaded'
        casper.test.assertEval (-> return r?), 'r is defined - Driver loaded'
        casper.test.assertEval (-> return !_.isEqual(DataExplorerView.Container.prototype.suggestions, {})), 'Suggestions loaded'
        casper.test.assertEval (-> return !_.isEqual(DataExplorerView.Container.prototype.descriptions, {})), 'Descriptions loaded'
        casper.test.assertEval (-> return !_.isEqual(DataExplorerView.Container.prototype.map_state, {'': ''})), 'Map state loaded'

    @waitFor (-> @evaluate(-> return !_.isEqual(DataExplorerView.Container.prototype.suggestions, {}))) , to_do , to_do , 2000

execute_queries = (casper, queries) ->
    that = @
    ###
    casper.then ->
        @echo @evaluate  ->
            query = 'r.expr(1)'
            window.router.current_view.codemirror.setValue(query)
            return window.router.current_view.codemirror.getValue()
            #return $('title').html()
        @echo '_____________'
    ###
 
    test.last_result = undefined
    for query, result of queries
        callbacks = generate_callbacks_for_queries casper, query, result
        casper.then callbacks.write

        casper.waitFor callbacks.wait, callbacks.then_do, callbacks.timeout_do, 2000
        first = false

generate_callbacks_for_queries = (casper, query, result) ->
    callbacks = {}
    callbacks.write = ->
        casper.evaluate ((query) ->
            window.router.current_view.codemirror.setValue(query)
            return window.router.current_view.codemirror.getValue()
        ), query

        casper.click '.btn.button_query.execute_query'


    callbacks.wait = ->
        if casper.evaluate( -> return $('.results.raw_view_container').css('display') is 'none')
            return casper.evaluate( -> return $('.result_view').children().length > 0)
        else
            return casper.evaluate(((last_result) -> return JSON.parse($('.raw_view_textarea').val()) isnt last_result), test.last_result)
        
    callbacks.then_do = ->
        if casper.evaluate( -> return $('.results.raw_view_container').css('display') is 'none')
            casper.click '.link_to_raw_view'
        new_result = JSON.parse(casper.evaluate -> return $('.raw_view_textarea').val())
        casper.test.assert casper.evaluate(((new_result, result) -> return _.isEqual(new_result, result)), new_result, result), "Execute query: #{query}"
        test.last_result = new_result

    callbacks.timeout_do = ->
        casper.test.fail "Timeout for query: #{query}"


    return callbacks



execute_queries casper,
    'r.expr(42)': 42
    'r.expr("hakuna matata")': 'hakuna matata'
    'r.expr(null)': null
    'r.expr(false)': false
    'r.expr({key: "value"})': {key: "value"}
    'r.expr([14, "hello", null, false])': [14, "hello", null, false]
    #TODO Add more tests

casper.then ->
    @test.assertEval (-> return $('.query_history').length > 1), 'History exists'

casper.run ->
    @exit(0)
