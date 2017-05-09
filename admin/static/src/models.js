// Copyright 2010-2012 RethinkDB, all rights reserved.
// Models for Backbone.js

class Servers extends Backbone.Collection {
  static initClass () {
    this.prototype.model = Server
    this.prototype.name = 'Servers'
  }
}
Servers.initClass()

class Server extends Backbone.Model {}

class Tables extends Backbone.Collection {
  static initClass () {
    this.prototype.model = Table
    this.prototype.name = 'Tables'
    this.prototype.comparator = 'name'
  }
}
Tables.initClass()

class Table extends Backbone.Model {}

class Reconfigure extends Backbone.Model {}

class Databases extends Backbone.Collection {
  static initClass () {
    this.prototype.model = Database
    this.prototype.name = 'Databases'
    this.prototype.comparator = 'name'
  }
}
Databases.initClass()

class Database extends Backbone.Model {}

class Indexes extends Backbone.Collection {
  static initClass () {
    this.prototype.model = Index
    this.prototype.name = 'Indexes'
    this.prototype.comparator = 'index'
  }
}
Indexes.initClass()

class Index extends Backbone.Model {}

class Distribution extends Backbone.Collection {
  static initClass () {
    this.prototype.model = Shard
    this.prototype.name = 'Shards'
  }
}
Distribution.initClass()

class Shard extends Backbone.Model {}

class ShardAssignments extends Backbone.Collection {
  static initClass () {
    this.prototype.model = ShardAssignment
    this.prototype.name = 'ShardAssignment'
  }
  comparator (a, b) {
    if (a.get('shard_id') < b.get('shard_id')) {
      return -1
    } else if (a.get('shard_id') > b.get('shard_id')) {
      return 1
    } else {
      if (a.get('start_shard') === true) {
        return -1
      } else if (b.get('start_shard') === true) {
        return 1
      } else if (a.get('end_shard') === true) {
        return 1
      } else if (b.get('end_shard') === true) {
        return -1
      } else if (a.get('primary') === true && b.get('replica') === true) {
        return -1
      } else if (a.get('replica') === true && b.get('primary') === true) {
        return 1
      } else if (a.get('replica') === true && b.get('replica') === true) {
        if (a.get('replica_position') < b.get('replica_position')) {
          return -1
        } else if (a.get('replica_position') > b.get('replica_position')) {
          return 1
        }
        return 0
      }
    }
  }
}
ShardAssignments.initClass()

class ShardAssignment extends Backbone.Model {}

class Responsibilities extends Backbone.Collection {
  static initClass () {
    this.prototype.model = Responsibility
    this.prototype.name = 'Responsibility'
  }
  comparator (a, b) {
    if (a.get('db') < b.get('db')) {
      return -1
    } else if (a.get('db') > b.get('db')) {
      return 1
    } else {
      if (a.get('table') < b.get('table')) {
        return -1
      } else if (a.get('table') > b.get('table')) {
        return 1
      } else {
        if (a.get('is_table') === true) {
          return -1
        } else if (b.get('is_table') === true) {
          return 1
        } else if (a.get('index') < b.get('index')) {
          return -1
        } else if (a.get('index') > b.get('index')) {
          return 1
        } else {
          return 0
        }
      }
    }
  }
}
Responsibilities.initClass()

class Responsibility extends Backbone.Model {}

class Dashboard extends Backbone.Model {}

class Issue extends Backbone.Model {}

class Issues extends Backbone.Collection {
  static initClass () {
    this.prototype.model = Issue
    this.prototype.name = 'Issues'
  }
}
Issues.initClass()

class Logs extends Backbone.Collection {
  static initClass () {
    this.prototype.model = Log
    this.prototype.name = 'Logs'
  }
}
Logs.initClass()

class Log extends Backbone.Model {}

class Stats extends Backbone.Model {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.on_result = this.on_result.bind(this)
    this.get_stats = this.get_stats.bind(this)
    super(...args)
  }

  initialize (query) {
    return this.set({
      keys_read: 0,
      keys_set : 0,
    })
  }

  on_result (err, result) {
    if (err != null) {
      // TODO: Display warning? Can stats still timeout?
      return console.log(err)
    } else {
      return this.set(result)
    }
  }

  get_stats () {
    return this.toJSON()
  }
}

// The equivalent of a database view, but for our Backbone models.
// Our models and collections have direct representations on the server. For
// convenience, it's useful to pick data from several of these models: these
// are computed models (and computed collections).

exports.Servers = Servers
exports.Server = Server
exports.Tables = Tables
exports.Table = Table
exports.Reconfigure = Reconfigure
exports.Databases = Databases
exports.Database = Database
exports.Indexes = Indexes
exports.Index = Index
exports.Distribution = Distribution
exports.Shard = Shard
exports.ShardAssignments = ShardAssignments
exports.ShardAssignment = ShardAssignment
exports.Responsibilities = Responsibilities
exports.Responsibility = Responsibility
exports.Dashboard = Dashboard
exports.Issue = Issue
exports.Issues = Issues
exports.Logs = Logs
exports.Log = Log
exports.Stats = Stats
