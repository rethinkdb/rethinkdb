
module.exports = star

var npm = require("./npm.js")
  , registry = npm.registry
  , log = require("npmlog")
  , asyncMap = require("slide").asyncMap

star.usage = "npm star <package> [pkg, pkg, ...]\n"
           + "npm unstar <package> [pkg, pkg, ...]"

star.completion = function (opts, cb) {
  registry.get("/-/short", 60000, function (er, list) {
    return cb(null, list || [])
  })
}

function star (args, cb) {
  if (!args.length) return cb(star.usage)
  var s = npm.config.get("unicode") ? "\u2605 " : "(*)"
    , u = npm.config.get("unicode") ? "\u2606 " : "( )"
    , using = !(npm.command.match(/^un/))
  if (!using) s = u
  asyncMap(args, function (pkg, cb) {
    registry.star(pkg, using, function (er, data, raw, req) {
      if (!er) {
        console.log(s + " "+pkg)
        log.verbose("star", data)
      }
      cb(er, data, raw, req)
    })
  }, cb)
}
