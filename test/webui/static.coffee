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



casper.run ->
    @exit(0)
