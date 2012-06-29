{ Casper } = require('./casper-common')
casper = new Casper

casper.start casper._rdb_server+'#namespaces', ->
    # Save an image of the namespace list view
    @_captureSelector('namespace-list.png', 'html')
    @test.assertExists('.btn.add-namespace', 'namespace button exists')

casper.run -> @_run()
