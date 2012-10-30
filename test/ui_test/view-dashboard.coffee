# Copyright 2010-2012 RethinkDB, all rights reserved.
{ Casper } = require('./casper-common')
casper = new Casper

casper.start casper._rdb_server, ->
    # Save an image of the dashboard
    @_captureSelector('dashboard.png', 'html')
    @test.assertExists('.dashboard-view', 'dashboard view exists')
    @test.assertDoesntExist('.problems-detected', 'cluster status reports no problems')

casper.run -> @_run()
