class Casper
    constructor: ->
        casper = require('casper').create
            #verbose: true
            logLevel: 'debug'
            viewportSize:
                width: 940
                height: 400
            clientScripts: ["jquery.js"]
        @casper = casper
      
        if @casper.cli.options['rdb-server']?
            casper._rdb_server = @casper.cli.options['rdb-server']
        else casper._rdb_server = 'http://localhost:6001/'

        casper._captureSelector = (filename, selector) =>
            images = @casper.cli.options.images
            return if not images
            # If an output directory was specified, save the images to that directory (prepend the output filename with the directory)
            filename = "#{images}/#{filename}" if images isnt true

            @casper.captureSelector(filename, selector)

        casper._run = =>
            @casper.test.renderResults(true)

            if @casper.test.testResults.failed > 0
                @casper.exit(1)
            else @casper.exit(0)

            @casper.test.done()

        # To help debug, capture remote messages (console.log in the application)
        casper._logRemote = =>
            @casper.on "remote.message", (message) =>
                  @casper.echo("remote console.log: " + message)
            

        return casper

    print_options: =>
        @casper.echo('Casper CLI passed options:')
        require('utils').dump(@casper.cli.options)

exports.Casper = Casper
