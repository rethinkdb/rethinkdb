# Copyright 2010-2012 RethinkDB, all rights reserved.
# add-a-namespace: tests adding a namespace to the cluster
# Tests performed:
#   - List namespaces
#   - Ensure the 'add namespace' button exists
#   - Open the 'add namespace' form
#   - Enter randomly generated details and submit the form
#   - Ensure an alert pops up acknowledging the namespace has been created
#   - Ensure the new namespace appears in the namespace list
#   - Ensure a view is successfully rendered for the new namespace

{ Casper } = require('./casper-common')
casper = new Casper

random = Math.floor Math.random()*10000
namespace =
    name: "Test namespace #{random}"
    port:  random

casper.start casper._rdb_server+'#namespaces', ->
    # Save an image of the namespace list view
    @_captureSelector('add-a-namespace_namespace-list.png', 'html')
    @test.assertExists('.btn.add-namespace', 'add namespace button exists')

casper.then ->
    # Open the add namespace form
    @click('.btn.add-namespace')
    @test.assertExists('#modal-dialog .modal.add-namespace', 'add namespace modal appeared')
    # Define namespace details
    @fill '.modal.add-namespace form',
        name: namespace.name
        port: namespace.port
    , false
    # Capture the filled-out form and submit it
    @_captureSelector('add-a-namespace_modal.png', 'html')
    @click('#modal-dialog .modal.add-namespace .btn-primary')

# Wait for the modal to disappear (once the form has submitted successfully)
casper.waitWhileSelector('#modal-dialog .modal.add-namespace',  ->
    # Make sure the alert shows up that indicates the namespace was added successfully
    @_captureSelector('add-a-namespace_namespace-added-alert.png', 'html')
    @test.assertExists('#user-alert-space .alert', 'alert that confirms a new namespace appeared')

    # Get the route of the new namespace
    namespace.route = @evaluate -> $('#user-alert-space .alert p a').attr('href')

    # Get the name of the new namespace
    name = @evaluate -> $('#user-alert-space .alert span.name').text()

    @test.assert(namespace.route? and name is namespace.name, 'alert has correct info about the new namespace')

    # Find the new namespace in the namespace list, make sure it has all the correct info
    @test.assertEval (namespace_name, namespace_route) ->
        for namespace in $('.namespace-list .namespace.details .name a')
            if $(namespace).text() is namespace_name
                return true if $(namespace).attr('href') is namespace_route
        return false
    , 'new namespace shows up in the list of namespaces'
    ,
        namespace_name: name
        namespace_route: namespace.route

    # Since waitWhileSelector is nonblocking, we proceed to the next step inside of this navigation step-- once we know our new namespace has been created
    casper.thenOpen casper._rdb_server+namespace.route , ->
        # See if a namespace view exists for the namespace we've created
        @test.assertExists('.namespace-view', 'namespace view exists for the new namespace')
        @_captureSelector('add-a-namespace_namespace-view.png', 'html')
)

casper.run -> @_run()
