{ Casper } = require('./casper-common')
casper = new Casper

casper.start casper._rdb_server, ->
    # Save an image of the dashboard
    @_captureSelector('dashboard.png', 'html')
    @test.assertExists('.dashboard-view', 'dashboard view showing')
    @test.assertDoesntExist('.problems-detected', 'cluster status reports no problems')
    @test.assertExists('asdasda', 'this is obviously bullshit')

casper.run -> @_run()
