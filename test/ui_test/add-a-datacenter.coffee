# Copyright 2010-2012 RethinkDB, all rights reserved.
# add-a-datacenter: tests adding a datacenter to the cluster
# Tests performed:
#   - List datacenters
#   - Ensure the 'add datacenter' button exists
#   - Open the 'add datacenter' form
#   - Enter randomly generated details and submit the form
#   - Ensure an alert pops up acknowledging the dataenter has been created
#   - Ensure the new datacenterappears in the datacenter list
#   - Ensure a view is successfully rendered for the new datacenter
{ Casper } = require('./casper-common')
casper = new Casper

random = Math.floor Math.random()*10000
datacenter =
    name: "Test datacenter #{random}"

casper.start casper._rdb_server+'#servers', ->
    # Save an image of the datacenter list view
    @_captureSelector('add-a-datacenter_datacenter-list.png', 'html')
    @test.assertExists('.btn.add-datacenter', 'add datacenter button exists')

casper.then ->
    # Open the add datacenter form
    @click('.btn.add-datacenter')
    @test.assertExists('#modal-dialog .modal.add-datacenter', 'add datacenter modal appeared')
    # Define datacenter details
    @fill '.modal.add-datacenter form',
        name: datacenter.name
    , false
    # Capture the filled-out form and submit it
    @_captureSelector('add-a-datacenter_modal.png', 'html')
    @click('#modal-dialog .modal.add-datacenter .btn-primary')

# Wait for the modal to disappear (once the form has submitted successfully)
casper.waitWhileSelector('#modal-dialog .modal.add-datacenter',  ->
    # Make sure the alert shows up that indicates the datacenter was added successfully
    @_captureSelector('add-a-datacenter_datacenter-added-alert.png', 'html')
    @test.assertExists('#user-alert-space .alert', 'alert that confirms a new datacenter appeared')

    # Get the route of the new datacenter
    datacenter.route = @evaluate -> $('#user-alert-space .alert p a').attr('href')

    # Get the name of the new datacenter
    name = @evaluate -> $('#user-alert-space .alert span.name').text()

    @test.assert(datacenter.route? and name is datacenter.name, 'alert has correct info about the new datacenter')

    # Find the new datacenter in the datacenter list, make sure it has all the correct info
    @test.assertEval (datacenter_name, datacenter_route) ->
        for datacenter in $('.datacenter-list .datacenter.details .name a')
            if $(datacenter).text() is datacenter_name
                return true if $(datacenter).attr('href') is datacenter_route
        return false
    , 'new datacenter shows up in the list of datacenters'
    ,
        datacenter_name: name
        datacenter_route: datacenter.route

    # Since waitWhileSelector is nonblocking, we proceed to the next step inside of this navigation step-- once we know our new datacenter has been created
    casper.thenOpen casper._rdb_server+datacenter.route , ->
        # See if a datacenter view exists for the datacenter we've created
        @test.assertExists('.datacenter-view', 'datacenter view exists for the new datacenter')
        @_captureSelector('add-a-datacenter_datacenter-view.png', 'html')
)

casper.run -> @_run()
