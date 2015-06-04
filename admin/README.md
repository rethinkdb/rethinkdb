# RethinkDB web interface

## Build

The MAKE file for the web interface is in `/mk/webui.mk`.

The build dependencies are
- `node.js` and `npm` are needed at the system level
- `npm install` will install dependencies listed in `npm-shrinkwrap.json`

### Adding dependencies
- Add the dependency to the `dependencies` object in `package.json`
- Delete the file `npm-shrinkwrap.json`
- Delete `node_modules` in this directory if it exists
- `$ npm install` in this directory (admin)
- `$ npm dedupe` this will flatten dependencies as much as possible
- `$ npm shrinkwrap` this will generate a new `npm-shrinkwrap.json`
- Bump the version in `mk/support/pkg/admin-deps.sh` (this ensures make will download new dependencies if necessary)
- Check in the changes to `npm-shrinkwrap.json`, `package.json` and `admin-deps.sh`

## Organization
- `favicon.ico`: The favicon...
- `Makefile`: To build from `/admin`
- `/static`:
    - `coffee`: all the CoffeeScript files
    - `handlebars`: all the Handlebars templates
    - `less`: all the Less files
    - `images`: all the images
    - `js`: all the pure JavaScript files (dependencies like `underscore` etc.)
    - `fonts`: web fonts used by the interface
- `/template`: the HTML files


## CoffeeScript files

This is an overview of how the interface is built. The main framework used to build
the interface is [Backbone](http://backbonejs.org/)

We do not use the method `Backbone.sync` but do rely on
- `views`
- `models`
- `collections`
- `router`



### Initialization

The initialization is done in `app.coffee`.

### Router

The router defined in `/admin/static/coffee/router.coffee` creates all the routes.

Every time the route is changed, we will create a new view and render it.

### Fetching data

#### How to
Since the ReQL api to manage your cluster, all the data is retrieved via
ReQL queries via HTTP.

The class `Driver` in `/admin/static/coffee/app.coffee` wraps the driver with
convenient way to retrieve data.

__Note__: The class `Driver` will rename the `run` method to `private_run`
and will have `run` throw an error. This is to prevent users to call `run` in
the data explorer.

The class `Driver` implements two useful methods:
- `run_once(query, callback)`: Will run `query` once, used to update the cluster,
create an index etc.
- `run(query, delay, callback[, index])`: Will run `query` every `delay` ms. The method
returns a timer (integer) that you can use to stop the interval with `stop_timer(timer)`.
`index` is an optional number that can be used to overwrite a current timer.


#### Views fetch data, not Models

We do not delegate fetching data to Backbone.sync because we want to group queries. For
example for the table view, we want to retrieve the table (model `Table`), but also the
secondary indexes (collection `Indexes`), and the responsibilities (collection `Shards).

The following has to be done to open a connection, run a query and close it:
- send an HTTP request to `/ajax/reql/open-new-connection` to open a connection and retrieve a token
- send an HTTP request to `/ajax/reql` with the query
- send an HTTP request to `/ajax/reql/close-connection` to close a connection associated with a token

If time allows, the HTTP server embedded in the server should support websocket, and the JS driver should
use them instead. Another solution would be to have a more complicated HTTP server, see
[#2878](https://github.com/rethinkdb/rethinkdb/issues/2878#issuecomment-53913077)

If possible, only the main view should run queries, and a main view's `remove` method should call
`driver.stop_timer` if it previously called `driver.run`.
This prevent us from forgetting to remove listeners/intervals since we will always call `remove`
in `/admin/static/coffee/router.coffee`.

Having the queries run in `router.coffee` would have also work, but would have make the class a bit
cumbersome.

#### Tables used

The main tables used are:
- `rethinkdb.table_config`: Configuration of the tables
- `rethinkdb.table_status`: Statuses of the tables (including its shards)
- `rethinkdb.server_config`: Configuration of the servers
- `rethinkdb.server_status`: Statuses of the servers

Tables that are currently missing (at the time of writing):
- stats
- logs

### Models and collections

We use both `Model` and `Collection` functions like they are intended to.
The whole point of these two classes is:
- To be able to update a model by passing a hash of attributes, and triggers the
appropriate `change`/`change:<attribute>` events.
- To to able to add/remove elements from the collection by passing the whole
collection.

Note that for Backbone to be able to decide what models are the same, they must
have an `id` attribute. The shards at the time of writing do not have an `id`
exposed, so we manually build the property in the query.

### Views

#### Introduction
Each view should extend `Backbone.View` and therefore is represented by its attribute `$el`.
It must have a method `render` that returns itself such that it can be displayed the
following way:

```coffeescript
$("#container").html(view.render().$el)
```

Because `$el` is the root element of the view, you can then update what is inside `$el`
without having to call `html` on its container again.

#### Refreshing a view listening to a model

A view can listen to a Backbone model or a collection. The version of Backbone we use
`1.1.2` provides the methods `listenTo` and `stopListening`.


The `listenTo(model, event, fn)` method let the view listen to events triggered by the
model.
When a view is removed (with the `remove` method, it will automatically call
`stopListening` which will remove all the listeners used by this view.


Typical events are

```
# call render everytime something changes on the model
@listenTo @model, 'change', @render

# call render everytime the attribute name change
@listenTo @model, 'change:name', @render
```

#### Refreshing a view listening to a collection

##### Basic example
This is the basic way such view should work.

```coffeescript
initialize: =>
     @views = []
     @collection.each (element) =>
         view = new ElementView
             model: element
         @views.push view
         @$el.append view.render().$el

     # call render everytime a model is added to @collection
     @listenTo @collection, 'add', (element) =>
         view = new ElementView
             model: element
         @views.push view
         @$el.append view.render().$el

     # call render everytime a model is removed from @collection
     @listenTo @collection, 'remove', (element) =>
         for view, i in @views
              if view.model is element
                  view.model.destroy() # Destroy the model
                  view.remove()        # Remove the view
                  @views.splice i, 1   # Delete the reference to the view
```


For each `element` in the `collection`, we create a new view and append it to `$el`.
We also make sure to keep a reference to this view in `@views` to be able to later
remove it if a `remove` event is triggered.

##### Keeping views in order

You probably want to keep the views in a certain order. While Backbone lets you define
a field `comparator` in Backbone.Collection, and keep the collection in the appropriate
order (`collection[i] < collection[i+1]`) this doesn't give you the guarantee that
the `add` events will be triggered in order. Typically for

```coffeescript
@collection.set [model1, model2]
```

You could have `@views.length < @collection.length - 1` (Backbone could add the two
elements, the trigger `add` for `model1` and then for `model2` (or the opposite).


To keep your views ordered, you should keep `@views` ordered and don't always prepend
in `$el`, but `prepend`, `append`, or use
`@$('.element_class').eq(position-1).after view.render().$el`


Here is a full example (taken from `/admin/static/coffee/table/database.coffee`)
```coffeescript
initialize: =>
    @tables_views = []
    @$el.html @template()

    @collection.each (table) =>
        view = new DatabaseView.TableView
            model: table
        # The first time, the collection is sorted
        @tables_views.push view
        @$('.tables').append view.render().$el

        if @collection.length is 0
            @$('.no_element').show()
        else
            @$('.no_element').hide()

    @collection.on 'add', (table) =>
        new_view = new DatabaseView.TableView
            model: table
        if @tables_views.length is 0
            @tables_views.push new_view
            @$('.tables').html new_view.render().$el
        else
            added = false
            for view, position in @tables_views
                if view.model.get('name') > table.get('name')
                    added = true
                    @tables_views.splice position, 0, new_view
                    if position is 0
                        @$('.tables').prepend new_view.render().$el
                    else
                        @$('.table_container').eq(position-1).after new_view.render().$el
                    break
            if added is false
                @tables_views.push new_view
                @$('.tables').append new_view.render().$el

        if @tables_views.length is 1
            @$('.no_element').hide()

    @collection.on 'remove', (table) =>
        for view in @tables_views
            if view.model is table
                table.destroy()
                ((view) ->
                     view.$el.slideUp 'fast', =>
                         view.remove()
                )(view)
                break
        if @tables_views.length is 0
            @$('.no_element').show()
```

##### About views representing a collection of collections

We have a collection of collections in `/admin/static/coffee/tables/index.coffee` (a list of databases
where each database has a list of tables).

The proper way to handle this structure is done in `/admin/static/coffee/table/index.coffee`.
Each model of the main collection is going to create a new collection based on one of its
attributes. The method `DatabaseView.update_collection` is the method doing it.


While this is nice and is how things are (probably?) intended to be used, it makes the code
cumbersome. In the case of simple collection of collections like for the responsabilities of
a server in `admin/static/coffee/server/server.coffee` or the list of shard assignments in
`/admin/static/coffee/table/shard_assignments.coffee`, we flatten the elements to get one unique
collection.

#### Removing a view

To delete a view, you should call the `remove` method.

If you created views inside a view, you must extend `remove` this way:

```coffeescript
remove: =>
    @view1.remove() # Remove the view1
    @view2.remove() # Remove the view2
    # ...
    super() # Eventually remove this view
```

Do not forget to call `super()` or the view itself won't be removed.

### Handlebars templates

Handlebars can do simple if statement with `{{#if condition}}...{{else}}...{{/if}}` where
`condition` is a boolean (not that Handlebars use a double equal to check if a value
is `true` of `false`, so beware of values like `0`, `[]`, etc.
You cannot check if values are equal in handlebars, so you need to pre-compute the result
in the CoffeeScript file.

Handlebars also has a `{{#each list}}...{{/each}}` control structure.

You can also define helpers with `Handlebars.registerHelper`. See
`/admin/static/coffee/util.coffee` to see about helpers currently defined.
The most common one being probably `pluralize_noun`.

### The data explorer

The data explorer uses [CodeMirror](http://codemirror.net) for the input.
The main method is `handle_keypress`, which is triggered for every event
on CodeMirror.

For each key pressed, the data explorer will parse the content of CodeMirror
and display suggestions or a description.
Because the user can copy/paste things, select text before typing etc,, there
is no shortcut to parse the query, and we must parse the whole input every time.

The parsing is "poorly" done (apologies for that, I should have read about parsing
before doing it). The way it works is by recursing in each argument of a method.

The main parts of the data explorer are:
- parsing the query done in `extra_data_from_query`
- handle events and display/hide suggestions/descriptions in `handle_keypress`
- execute the query `execute_query`, `execute_portion` and the relevant methods
to generate callbacks `generate_get_result_callback`/`generate_rdb_global_callback`. We
could probably simplify things as now we do not allow more than one query to be run.
- display results with `render_result` and the ResultView


### Random notes that can help

[highcharts.js](http://www.highcharts.com/) is the only library I found that could do
a good job at displaying the key distribution. For licensing reasons, we cannot use it.
So we still use d3.js.

The performance graph relies on a fork of cubism. We should probably replace it at some
point, but for now it still works. The main issues with other libraries is that they do
not have a smooth sliding.
