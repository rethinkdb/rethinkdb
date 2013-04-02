# Copyright 2010-2012 RethinkDB, all rights reserved.
class Test
    constructor: (args) ->
        @casper = require('casper').create
            logLevel: 'debug'
            viewportSize:
                width: 1000
                height: 800
            clientScripts: [args.dir+"/include/jquery-1.9.1.js", args.dir+"/include/underscore.js"]

        @init()
        return @
      
    init: =>
        if @casper.cli.options['rdb-server']?
            @rdb_server = @casper.cli.options['rdb-server']
        else
            @rdb_server = 'http://localhost:8080/'
        @data = {}


    return: =>
        if @casper.test.testResults.failed > 0
            @casper.exit(1)
        else
            @casper.exit(0)

        @casper.test.done()
        @casper.exit(0)

        ###
        casper._captureSelector = (filename, selector) =>
            images = @casper.cli.options.images
            return if not images
            # If an output directory was specified, save the images to that directory (prepend the output filename with the directory)
            filename = "#{images}/#{filename}" if images isnt true

            @casper.captureSelector(filename, selector)
        ###

        casper.return = (fn) ->
            casper.run fn
            ###
            if @casper.test.testResults.failed > 0
                @casper.exit(1)
            else @casper.exit(0)

            @casper.test.done()
            ###

        # To help debug, capture remote messages (console.log in the application)
        casper._logRemote = =>
            @casper.on "remote.message", (message) =>
                  @casper.echo("remote console.log: " + message)
            

        return casper

    print_options: =>
        @casper.echo('Casper CLI passed options:')
        require('utils').dump(@casper.cli.options)

exports.Test = Test
