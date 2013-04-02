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
 
    log_view: ->
        @test.assertExists '.log-view', 'Logs view available'
        @test.assertEval ( -> return $('.log-entry').length > 0), 'Logs loaded and available'
 
    tables: ->
        that = @
        @test.assertExists '.databases_list-container', 'Database list available'
        @click '.btn.add-database'
        @test.assertEvalEquals ( -> $('.database.summary').length ), 0, 'No database'
        @test.assertEval ( -> $('#modal-dialog').html() isnt ''), 'Modal for new database available'
        @test.assertExists '.xlarge.focus_new_name', 'Input for database name available'
        db_name = Test::generate_string()
        Test::data.db = db_name
        #@evaluate -> return $('.focus_new_name').val db_name
        @fill '.form', {name: db_name}
        @click '.btn.btn-primary'
        @waitFor ( -> @evaluate( -> $('.focus_new_name').length is 0))
            , ( -> Test::tables_with_db.apply(that) )
            , ( ->
                @captureSelector('blabla.png', 'body')
                @test.assert(false, 'Time out creating a database'))
            , Test::query_waiting
 
    tables_with_db: ->
        db_name = Test::data.db
        @test.assertEvalEquals ( -> $('.database.summary').length ), 1, 'One database found'
        #@test.assertEval ( -> return @evaluate(-> $('.database.summary').length is 1)), 'Database created'
        #@wait(2000, -> @test.assertTextExists(db_name, 'Created database found'))

