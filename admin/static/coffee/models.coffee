# Copyright 2010-2012 RethinkDB, all rights reserved.
#Models for Backbone.js

class Servers extends Backbone.Collection
    model: Server
    name: 'Servers'

class Server extends Backbone.Model

class Tables extends Backbone.Collection
    model: Table
    name: 'Tables'
    comparator: 'name'

class Table extends Backbone.Model

class Reconfigure extends Backbone.Model

class Databases extends Backbone.Collection
    model: Database
    name: 'Databases'
    comparator: 'name'

class Database extends Backbone.Model

class Indexes extends Backbone.Collection
    model: Index
    name: 'Indexes'
    comparator: 'index'

class Index extends Backbone.Model

class Distribution extends Backbone.Collection
    model: Shard
    name: 'Shards'

class Shard extends Backbone.Model

class ShardAssignments extends Backbone.Collection
    model: ShardAssignment
    name: 'ShardAssignment'
    comparator: (a, b) ->
        if a.get('shard_id') < b.get('shard_id')
            return -1
        else if a.get('shard_id') > b.get('shard_id')
            return 1
        else
            if a.get('start_shard') is true
                return -1
            else if b.get('start_shard') is true
                return 1
            else if a.get('end_shard') is true
                return 1
            else if b.get('end_shard') is true
                return -1
            else if a.get('primary') is true and b.get('replica') is true
                return -1
            else if a.get('replica') is true and b.get('primary') is true
                return 1
            else if a.get('replica') is true and  b.get('replica') is true
                if a.get('replica_position') < b.get('replica_position')
                    return -1
                else if a.get('replica_position') > b.get('replica_position')
                    return 1
                return 0

class ShardAssignment extends Backbone.Model

class Responsibilities extends Backbone.Collection
    model: Responsibility
    name: 'Responsibility'
    comparator: (a, b) ->
        if a.get('db') < b.get('db')
            return -1
        else if a.get('db') > b.get('db')
            return 1
        else
            if a.get('table') < b.get('table')
                return -1
            else if a.get('table') > b.get('table')
                return 1
            else
                if a.get('is_table') is true
                    return -1
                else if b.get('is_table') is true
                    return 1
                else if a.get('index') < b.get('index')
                    return -1
                else if a.get('index') > b.get('index')
                    return 1
                else
                    return 0

class Responsibility extends Backbone.Model

class Dashboard extends Backbone.Model

class Issue extends Backbone.Model

class Issues extends Backbone.Collection
    model: Issue
    name: 'Issues'

class Logs extends Backbone.Collection
    model: Log
    name: 'Logs'

class Log extends Backbone.Model

class Stats extends Backbone.Model
    initialize: (query) =>
        @set
            keys_read: 0
            keys_set: 0

    on_result: (err, result) =>
        if err?
            #TODO: Display warning? Can stats still timeout?
            console.log err
        else
            @set result


    get_stats: =>
        @toJSON()

# This module contains utility functions that compute and massage
# commonly used data.
module 'DataUtils', ->
    # The equivalent of a database view, but for our Backbone models.
    # Our models and collections have direct representations on the server. For
    # convenience, it's useful to pick data from several of these models: these
    # are computed models (and computed collections).

    @stripslashes = (str) ->
        str=str.replace(/\\'/g,'\'')
        str=str.replace(/\\"/g,'"')
        str=str.replace(/\\0/g,"\x00")
        str=str.replace(/\\\\/g,'\\')
        return str

    @is_integer = (data) ->
        return data.search(/^\d+$/) isnt -1

    # Deep copy. We do not copy prototype.
    @deep_copy = (data) ->
        if typeof data is 'boolean' or typeof data is 'number' or typeof data is 'string' or typeof data is 'number' or data is null or data is undefined
            return data
        else if typeof data is 'object' and Object.prototype.toString.call(data) is '[object Array]'
            result = []
            for value in data
                result.push @deep_copy value
            return result
        else if typeof data is 'object'
            result = {}
            for key, value of data
                result[key] = @deep_copy value
            return result
