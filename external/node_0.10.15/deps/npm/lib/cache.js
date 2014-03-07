// XXX lib/utils/tar.js and this file need to be rewritten.

// URL-to-cache folder mapping:
// : -> !
// @ -> _
// http://registry.npmjs.org/foo/version -> cache/http!/...
//

/*
fetching a url:
1. Check for url in inFlightUrls.  If present, add cb, and return.
2. create inFlightURL list
3. Acquire lock at {cache}/{sha(url)}.lock
   retries = {cache-lock-retries, def=3}
   stale = {cache-lock-stale, def=30000}
   wait = {cache-lock-wait, def=100}
4. if lock can't be acquired, then fail
5. fetch url, clear lock, call cbs

cache folders:
1. urls: http!/server.com/path/to/thing
2. c:\path\to\thing: file!/c!/path/to/thing
3. /path/to/thing: file!/path/to/thing
4. git@ private: git_github.com!isaacs/npm
5. git://public: git!/github.com/isaacs/npm
6. git+blah:// git-blah!/server.com/foo/bar

adding a folder:
1. tar into tmp/random/package.tgz
2. untar into tmp/random/contents/package, stripping one dir piece
3. tar tmp/random/contents/package to cache/n/v/package.tgz
4. untar cache/n/v/package.tgz into cache/n/v/package
5. rm tmp/random

Adding a url:
1. fetch to tmp/random/package.tgz
2. goto folder(2)

adding a name@version:
1. registry.get(name/version)
2. if response isn't 304, add url(dist.tarball)

adding a name@range:
1. registry.get(name)
2. Find a version that satisfies
3. add name@version

adding a local tarball:
1. untar to tmp/random/{blah}
2. goto folder(2)
*/

exports = module.exports = cache
cache.read = read
cache.clean = clean
cache.unpack = unpack
cache.lock = lock
cache.unlock = unlock

var mkdir = require("mkdirp")
  , spawn = require("child_process").spawn
  , exec = require("child_process").execFile
  , once = require("once")
  , fetch = require("./utils/fetch.js")
  , npm = require("./npm.js")
  , fs = require("graceful-fs")
  , rm = require("rimraf")
  , readJson = require("read-package-json")
  , registry = npm.registry
  , log = require("npmlog")
  , path = require("path")
  , sha = require("sha")
  , asyncMap = require("slide").asyncMap
  , semver = require("semver")
  , tar = require("./utils/tar.js")
  , fileCompletion = require("./utils/completion/file-completion.js")
  , url = require("url")
  , chownr = require("chownr")
  , lockFile = require("lockfile")
  , crypto = require("crypto")
  , retry = require("retry")
  , zlib = require("zlib")
  , chmodr = require("chmodr")
  , which = require("which")
  , isGitUrl = require("./utils/is-git-url.js")

cache.usage = "npm cache add <tarball file>"
            + "\nnpm cache add <folder>"
            + "\nnpm cache add <tarball url>"
            + "\nnpm cache add <git url>"
            + "\nnpm cache add <name>@<version>"
            + "\nnpm cache ls [<path>]"
            + "\nnpm cache clean [<pkg>[@<version>]]"

cache.completion = function (opts, cb) {

  var argv = opts.conf.argv.remain
  if (argv.length === 2) {
    return cb(null, ["add", "ls", "clean"])
  }

  switch (argv[2]) {
    case "clean":
    case "ls":
      // cache and ls are easy, because the completion is
      // what ls_ returns anyway.
      // just get the partial words, minus the last path part
      var p = path.dirname(opts.partialWords.slice(3).join("/"))
      if (p === ".") p = ""
      return ls_(p, 2, cb)
    case "add":
      // Same semantics as install and publish.
      return npm.commands.install.completion(opts, cb)
  }
}

function cache (args, cb) {
  var cmd = args.shift()
  switch (cmd) {
    case "rm": case "clear": case "clean": return clean(args, cb)
    case "list": case "sl": case "ls": return ls(args, cb)
    case "add": return add(args, cb)
    default: return cb(new Error(
      "Invalid cache action: "+cmd))
  }
}

// if the pkg and ver are in the cache, then
// just do a readJson and return.
// if they're not, then fetch them from the registry.
function read (name, ver, forceBypass, cb) {
  if (typeof cb !== "function") cb = forceBypass, forceBypass = true
  var jsonFile = path.join(npm.cache, name, ver, "package", "package.json")
  function c (er, data) {
    if (data) deprCheck(data)
    return cb(er, data)
  }

  if (forceBypass && npm.config.get("force")) {
    log.verbose("using force", "skipping cache")
    return addNamed(name, ver, c)
  }

  readJson(jsonFile, function (er, data) {
    er = needName(er, data)
    er = needVersion(er, data)
    if (er && er.code !== "ENOENT" && er.code !== "ENOTDIR") return cb(er)
    if (er) return addNamed(name, ver, c)
    deprCheck(data)
    c(er, data)
  })
}

// npm cache ls [<path>]
function ls (args, cb) {
  args = args.join("/").split("@").join("/")
  if (args.substr(-1) === "/") args = args.substr(0, args.length - 1)
  var prefix = npm.config.get("cache")
  if (0 === prefix.indexOf(process.env.HOME)) {
    prefix = "~" + prefix.substr(process.env.HOME.length)
  }
  ls_(args, npm.config.get("depth"), function (er, files) {
    console.log(files.map(function (f) {
      return path.join(prefix, f)
    }).join("\n").trim())
    cb(er, files)
  })
}

// Calls cb with list of cached pkgs matching show.
function ls_ (req, depth, cb) {
  return fileCompletion(npm.cache, req, depth, cb)
}

// npm cache clean [<path>]
function clean (args, cb) {
  if (!cb) cb = args, args = []
  if (!args) args = []
  args = args.join("/").split("@").join("/")
  if (args.substr(-1) === "/") args = args.substr(0, args.length - 1)
  var f = path.join(npm.cache, path.normalize(args))
  if (f === npm.cache) {
    fs.readdir(npm.cache, function (er, files) {
      if (er) return cb()
      asyncMap( files.filter(function (f) {
                  return npm.config.get("force") || f !== "-"
                }).map(function (f) {
                  return path.join(npm.cache, f)
                })
              , rm, cb )
    })
  } else rm(path.join(npm.cache, path.normalize(args)), cb)
}

// npm cache add <tarball-url>
// npm cache add <pkg> <ver>
// npm cache add <tarball>
// npm cache add <folder>
cache.add = function (pkg, ver, scrub, cb) {
  if (typeof cb !== "function") cb = scrub, scrub = false
  if (typeof cb !== "function") cb = ver, ver = null
  if (scrub) {
    return clean([], function (er) {
      if (er) return cb(er)
      add([pkg, ver], cb)
    })
  }
  log.verbose("cache add", [pkg, ver])
  return add([pkg, ver], cb)
}

function add (args, cb) {
  // this is hot code.  almost everything passes through here.
  // the args can be any of:
  // ["url"]
  // ["pkg", "version"]
  // ["pkg@version"]
  // ["pkg", "url"]
  // This is tricky, because urls can contain @
  // Also, in some cases we get [name, null] rather
  // that just a single argument.

  var usage = "Usage:\n"
            + "    npm cache add <tarball-url>\n"
            + "    npm cache add <pkg>@<ver>\n"
            + "    npm cache add <tarball>\n"
            + "    npm cache add <folder>\n"
    , name
    , spec

  if (args[1] === undefined) args[1] = null

  // at this point the args length must ==2
  if (args[1] !== null) {
    name = args[0]
    spec = args[1]
  } else if (args.length === 2) {
    spec = args[0]
  }

  log.verbose("cache add", "name=%j spec=%j args=%j", name, spec, args)


  if (!name && !spec) return cb(usage)

  // see if the spec is a url
  // otherwise, treat as name@version
  var p = url.parse(spec) || {}
  log.verbose("parsed url", p)

  // it could be that we got name@http://blah
  // in that case, we will not have a protocol now, but if we
  // split and check, we will.
  if (!name && !p.protocol && spec.indexOf("@") !== -1) {
    spec = spec.split("@")
    name = spec.shift()
    spec = spec.join("@")
    return add([name, spec], cb)
  }

  switch (p.protocol) {
    case "http:":
    case "https:":
      return addRemoteTarball(spec, null, name, cb)

    default:
      if (isGitUrl(p))
        return addRemoteGit(spec, p, name, false, cb)

      // if we have a name and a spec, then try name@spec
      // if not, then try just spec (which may try name@"" if not found)
      if (name) {
        addNamed(name, spec, cb)
      } else {
        addLocal(spec, cb)
      }
  }
}

function fetchAndShaCheck (u, tmp, shasum, cb) {
  fetch(u, tmp, function (er, response) {
    if (er) {
      log.error("fetch failed", u)
      return cb(er, response)
    }
    if (!shasum) return cb(null, response)
    // validate that the url we just downloaded matches the expected shasum.
    sha.check(tmp, shasum, function (er) {
      return cb(er, response, shasum)
    })
  })
}

// Only have a single download action at once for a given url
// additional calls stack the callbacks.
var inFlightURLs = {}
function addRemoteTarball (u, shasum, name, cb_) {
  if (typeof cb_ !== "function") cb_ = name, name = ""
  if (typeof cb_ !== "function") cb_ = shasum, shasum = null

  if (!inFlightURLs[u]) inFlightURLs[u] = []
  var iF = inFlightURLs[u]
  iF.push(cb_)
  if (iF.length > 1) return

  function cb (er, data) {
    if (data) {
      data._from = u
      data._resolved = u
    }
    unlock(u, function () {
      var c
      while (c = iF.shift()) c(er, data)
      delete inFlightURLs[u]
    })
  }

  var tmp = path.join(npm.tmp, Date.now()+"-"+Math.random(), "tmp.tgz")

  lock(u, function (er) {
    if (er) return cb(er)

    log.verbose("addRemoteTarball", [u, shasum])
    mkdir(path.dirname(tmp), function (er) {
      if (er) return cb(er)
      addRemoteTarball_(u, tmp, shasum, done)
    })
  })

  function done (er, resp, shasum) {
    if (er) return cb(er)
    addLocalTarball(tmp, name, shasum, cb)
  }
}

function addRemoteTarball_(u, tmp, shasum, cb) {
  // Tuned to spread 3 attempts over about a minute.
  // See formula at <https://github.com/tim-kos/node-retry>.
  var operation = retry.operation
    ( { retries: npm.config.get("fetch-retries")
      , factor: npm.config.get("fetch-retry-factor")
      , minTimeout: npm.config.get("fetch-retry-mintimeout")
      , maxTimeout: npm.config.get("fetch-retry-maxtimeout") })

  operation.attempt(function (currentAttempt) {
    log.info("retry", "fetch attempt " + currentAttempt
      + " at " + (new Date()).toLocaleTimeString())
    fetchAndShaCheck(u, tmp, shasum, function (er, response, shasum) {
      // Only retry on 408, 5xx or no `response`.
      var sc = response && response.statusCode
      var statusRetry = !sc || (sc === 408 || sc >= 500)
      if (er && statusRetry && operation.retry(er)) {
        log.info("retry", "will retry, error on last attempt: " + er)
        return
      }
      cb(er, response, shasum)
    })
  })
}

// 1. cacheDir = path.join(cache,'_git-remotes',sha1(u))
// 2. checkGitDir(cacheDir) ? 4. : 3. (rm cacheDir if necessary)
// 3. git clone --mirror u cacheDir
// 4. cd cacheDir && git fetch -a origin
// 5. git archive /tmp/random.tgz
// 6. addLocalTarball(/tmp/random.tgz) <gitref> --format=tar --prefix=package/
// silent flag is used if this should error quietly
function addRemoteGit (u, parsed, name, silent, cb_) {
  if (typeof cb_ !== "function") cb_ = name, name = null

  if (!inFlightURLs[u]) inFlightURLs[u] = []
  var iF = inFlightURLs[u]
  iF.push(cb_)
  if (iF.length > 1) return

  function cb (er, data) {
    unlock(u, function () {
      var c
      while (c = iF.shift()) c(er, data)
      delete inFlightURLs[u]
    })
  }

  var p, co // cachePath, git-ref we want to check out

  lock(u, function (er) {
    if (er) return cb(er)

    // figure out what we should check out.
    var co = parsed.hash && parsed.hash.substr(1) || "master"
    // git is so tricky!
    // if the path is like ssh://foo:22/some/path then it works, but
    // it needs the ssh://
    // If the path is like ssh://foo:some/path then it works, but
    // only if you remove the ssh://
    var origUrl = u
    u = u.replace(/^git\+/, "")
         .replace(/#.*$/, "")

    // ssh paths that are scp-style urls don't need the ssh://
    if (parsed.pathname.match(/^\/?:/)) {
      u = u.replace(/^ssh:\/\//, "")
    }

    var v = crypto.createHash("sha1").update(u).digest("hex").slice(0, 8)
    v = u.replace(/[^a-zA-Z0-9]+/g, '-') + '-' + v

    log.verbose("addRemoteGit", [u, co])

    p = path.join(npm.config.get("cache"), "_git-remotes", v)

    checkGitDir(p, u, co, origUrl, silent, function(er, data) {
      chmodr(p, npm.modes.file, function(erChmod) {
        if (er) return cb(er, data)
        return cb(erChmod, data)
      })
    })
  })
}

function checkGitDir (p, u, co, origUrl, silent, cb) {
  fs.stat(p, function (er, s) {
    if (er) return cloneGitRemote(p, u, co, origUrl, silent, cb)
    if (!s.isDirectory()) return rm(p, function (er){
      if (er) return cb(er)
      cloneGitRemote(p, u, co, origUrl, silent, cb)
    })

    var git = npm.config.get("git")
    var args = [ "config", "--get", "remote.origin.url" ]
    var env = gitEnv()

    // check for git
    which(git, function (err) {
      if (err) {
        err.code = "ENOGIT"
        return cb(err)
      }
      exec(git, args, {cwd: p, env: env}, function (er, stdout, stderr) {
        stdoutTrimmed = (stdout + "\n" + stderr).trim()
        if (er || u !== stdout.trim()) {
          log.warn( "`git config --get remote.origin.url` returned "
                  + "wrong result ("+u+")", stdoutTrimmed )
          return rm(p, function (er){
            if (er) return cb(er)
            cloneGitRemote(p, u, co, origUrl, silent, cb)
          })
        }
        log.verbose("git remote.origin.url", stdoutTrimmed)
        archiveGitRemote(p, u, co, origUrl, cb)
      })
    })
  })
}

function cloneGitRemote (p, u, co, origUrl, silent, cb) {
  mkdir(p, function (er) {
    if (er) return cb(er)

    var git = npm.config.get("git")
    var args = [ "clone", "--mirror", u, p ]
    var env = gitEnv()

    // check for git
    which(git, function (err) {
      if (err) {
        err.code = "ENOGIT"
        return cb(err)
      }
      exec(git, args, {cwd: p, env: env}, function (er, stdout, stderr) {
        stdout = (stdout + "\n" + stderr).trim()
        if (er) {
          if (silent) {
            log.verbose("git clone " + u, stdout)
          } else {
            log.error("git clone " + u, stdout)
          }
          return cb(er)
        }
        log.verbose("git clone " + u, stdout)
        archiveGitRemote(p, u, co, origUrl, cb)
      })
    })
  })
}

function archiveGitRemote (p, u, co, origUrl, cb) {
  var git = npm.config.get("git")
  var archive = [ "fetch", "-a", "origin" ]
  var resolve = [ "rev-list", "-n1", co ]
  var env = gitEnv()

  var errState = null
  var n = 0
  var resolved = null
  var tmp

  exec(git, archive, {cwd: p, env: env}, function (er, stdout, stderr) {
    stdout = (stdout + "\n" + stderr).trim()
    if (er) {
      log.error("git fetch -a origin ("+u+")", stdout)
      return cb(er)
    }
    log.verbose("git fetch -a origin ("+u+")", stdout)
    tmp = path.join(npm.tmp, Date.now()+"-"+Math.random(), "tmp.tgz")
    resolveHead()
  })

  function resolveHead () {
    exec(git, resolve, {cwd: p, env: env}, function (er, stdout, stderr) {
      stdout = (stdout + "\n" + stderr).trim()
      if (er) {
        log.error("Failed resolving git HEAD (" + u + ")", stderr)
        return cb(er)
      }
      log.verbose("git rev-list -n1 " + co, stdout)
      var parsed = url.parse(origUrl)
      parsed.hash = stdout
      resolved = url.format(parsed)

      // https://github.com/isaacs/npm/issues/3224
      // node incorrectly sticks a / at the start of the path
      // We know that the host won't change, so split and detect this
      var spo = origUrl.split(parsed.host)
      var spr = resolved.split(parsed.host)
      if (spo[1].charAt(0) === ':' && spr[1].charAt(0) === '/')
        spr[1] = spr[1].slice(1)
      resolved = spr.join(parsed.host)

      log.verbose('resolved git url', resolved)
      next()
    })
  }

  function next () {
    mkdir(path.dirname(tmp), function (er) {
      if (er) return cb(er)
      var gzip = zlib.createGzip({ level: 9 })
      var git = npm.config.get("git")
      var args = ["archive", co, "--format=tar", "--prefix=package/"]
      var out = fs.createWriteStream(tmp)
      var env = gitEnv()
      cb = once(cb)
      var cp = spawn(git, args, { env: env, cwd: p })
      cp.on("error", cb)
      cp.stderr.on("data", function(chunk) {
        log.silly(chunk.toString(), "git archive")
      })

      cp.stdout.pipe(gzip).pipe(out).on("close", function() {
        addLocalTarball(tmp, function(er, data) {
          if (data) data._resolved = resolved
          cb(er, data)
        })
      })
    })
  }
}

var gitEnv_
function gitEnv () {
  // git responds to env vars in some weird ways in post-receive hooks
  // so don't carry those along.
  if (gitEnv_) return gitEnv_
  gitEnv_ = {}
  for (var k in process.env) {
    if (!~['GIT_PROXY_COMMAND','GIT_SSH'].indexOf(k) && k.match(/^GIT/)) continue
    gitEnv_[k] = process.env[k]
  }
  return gitEnv_
}


// only have one request in flight for a given
// name@blah thing.
var inFlightNames = {}
function addNamed (name, x, data, cb_) {
  if (typeof cb_ !== "function") cb_ = data, data = null
  log.verbose("addNamed", [name, x])

  var k = name + "@" + x
  if (!inFlightNames[k]) inFlightNames[k] = []
  var iF = inFlightNames[k]
  iF.push(cb_)
  if (iF.length > 1) return

  function cb (er, data) {
    if (data && !data._fromGithub) data._from = k
    unlock(k, function () {
      var c
      while (c = iF.shift()) c(er, data)
      delete inFlightNames[k]
    })
  }

  log.verbose("addNamed", [semver.valid(x), semver.validRange(x)])
  lock(k, function (er, fd) {
    if (er) return cb(er)

    var fn = ( semver.valid(x, true) ? addNameVersion
             : semver.validRange(x, true) ? addNameRange
             : addNameTag
             )
    fn(name, x, data, cb)
  })
}

function addNameTag (name, tag, data, cb_) {
  if (typeof cb_ !== "function") cb_ = data, data = null
  log.info("addNameTag", [name, tag])
  var explicit = true
  if (!tag) {
    explicit = false
    tag = npm.config.get("tag")
  }

  function cb(er, data) {
    // might be username/project
    // in that case, try it as a github url.
    if (er && tag.split("/").length === 2) {
      return maybeGithub(tag, name, er, cb_)
    }
    return cb_(er, data)
  }

  registry.get(name, function (er, data, json, response) {
    if (er) return cb(er)
    engineFilter(data)
    if (data["dist-tags"] && data["dist-tags"][tag]
        && data.versions[data["dist-tags"][tag]]) {
      var ver = data["dist-tags"][tag]
      return addNamed(name, ver, data.versions[ver], cb)
    }
    if (!explicit && Object.keys(data.versions).length) {
      return addNamed(name, "*", data, cb)
    }

    er = installTargetsError(tag, data)
    return cb(er)
  })
}


function engineFilter (data) {
  var npmv = npm.version
    , nodev = npm.config.get("node-version")
    , strict = npm.config.get("engine-strict")

  if (!nodev || npm.config.get("force")) return data

  Object.keys(data.versions || {}).forEach(function (v) {
    var eng = data.versions[v].engines
    if (!eng) return
    if (!strict && !data.versions[v].engineStrict) return
    if (eng.node && !semver.satisfies(nodev, eng.node, true)
        || eng.npm && !semver.satisfies(npmv, eng.npm, true)) {
      delete data.versions[v]
    }
  })
}

function addNameRange (name, range, data, cb) {
  if (typeof cb !== "function") cb = data, data = null

  range = semver.validRange(range, true)
  if (range === null) return cb(new Error(
    "Invalid version range: "+range))

  log.silly("addNameRange", {name:name, range:range, hasData:!!data})

  if (data) return next()
  registry.get(name, function (er, d, json, response) {
    if (er) return cb(er)
    data = d
    next()
  })

  function next () {
    log.silly( "addNameRange", "number 2"
             , {name:name, range:range, hasData:!!data})
    engineFilter(data)

    log.silly("addNameRange", "versions"
             , [data.name, Object.keys(data.versions || {})])

    // if the tagged version satisfies, then use that.
    var tagged = data["dist-tags"][npm.config.get("tag")]
    if (tagged
        && data.versions[tagged]
        && semver.satisfies(tagged, range, true)) {
      return addNamed(name, tagged, data.versions[tagged], cb)
    }

    // find the max satisfying version.
    var versions = Object.keys(data.versions || {})
    var ms = semver.maxSatisfying(versions, range, true)
    if (!ms) {
      return cb(installTargetsError(range, data))
    }

    // if we don't have a registry connection, try to see if
    // there's a cached copy that will be ok.
    addNamed(name, ms, data.versions[ms], cb)
  }
}

function installTargetsError (requested, data) {
  var targets = Object.keys(data["dist-tags"]).filter(function (f) {
    return (data.versions || {}).hasOwnProperty(f)
  }).concat(Object.keys(data.versions || {}))

  requested = data.name + (requested ? "@'" + requested + "'" : "")

  targets = targets.length
          ? "Valid install targets:\n" + JSON.stringify(targets)
          : "No valid targets found.\n"
          + "Perhaps not compatible with your version of node?"

  return new Error( "No compatible version found: "
                  + requested + "\n" + targets)
}

function addNameVersion (name, v, data, cb) {
  if (typeof cb !== "function") cb = data, data = null

  var ver = semver.valid(v, true)
  if (!ver) return cb(new Error("Invalid version: "+v))

  var response

  if (data) {
    response = null
    return next()
  }
  registry.get(name + "/" + ver, function (er, d, json, resp) {
    if (er) return cb(er)
    data = d
    response = resp
    next()
  })

  function next () {
    deprCheck(data)
    var dist = data.dist

    if (!dist) return cb(new Error("No dist in "+data._id+" package"))

    if (!dist.tarball) return cb(new Error(
      "No dist.tarball in " + data._id + " package"))

    if ((response && response.statusCode !== 304) || npm.config.get("force")) {
      return fetchit()
    }

    // we got cached data, so let's see if we have a tarball.
    fs.stat(path.join(npm.cache, name, ver, "package.tgz"), function (er, s) {
      if (!er) readJson( path.join( npm.cache, name, ver
                                  , "package", "package.json" )
                       , function (er, data) {
          er = needName(er, data)
          er = needVersion(er, data)
          if (er && er.code !== "ENOENT" && er.code !== "ENOTDIR") return cb(er)
          if (er) return fetchit()
          return cb(null, data)
        })
      else return fetchit()
    })

    function fetchit () {
      if (!npm.config.get("registry")) {
        return cb(new Error("Cannot fetch: "+dist.tarball))
      }

      // use the same protocol as the registry.
      // https registry --> https tarballs.
      var tb = url.parse(dist.tarball)
      tb.protocol = url.parse(npm.config.get("registry")).protocol
      delete tb.href
      tb = url.format(tb)
      // only add non-shasum'ed packages if --forced.
      // only ancient things would lack this for good reasons nowadays.
      if (!dist.shasum && !npm.config.get("force")) {
        return cb(new Error("package lacks shasum: " + data._id))
      }
      return addRemoteTarball( tb
                             , dist.shasum
                             , name+"-"+ver
                             , cb )
    }
  }
}

function addLocal (p, name, cb_) {
  if (typeof cb_ !== "function") cb_ = name, name = ""

  function cb (er, data) {
    unlock(p, function () {
      if (er) {
        // if it doesn't have a / in it, it might be a
        // remote thing.
        if (p.indexOf("/") === -1 && p.charAt(0) !== "."
           && (process.platform !== "win32" || p.indexOf("\\") === -1)) {
          return addNamed(p, "", cb_)
        }
        log.error("addLocal", "Could not install %s", p)
        return cb_(er)
      }
      if (data && !data._fromGithub) data._from = p
      return cb_(er, data)
    })
  }

  lock(p, function (er) {
    if (er) return cb(er)
    // figure out if this is a folder or file.
    fs.stat(p, function (er, s) {
      if (er) {
        // might be username/project
        // in that case, try it as a github url.
        if (p.split("/").length === 2) {
          return maybeGithub(p, name, er, cb)
        }
        return cb(er)
      }
      if (s.isDirectory()) addLocalDirectory(p, name, cb)
      else addLocalTarball(p, name, cb)
    })
  })
}

function maybeGithub (p, name, er, cb) {
  var u = "git://github.com/" + p
    , up = url.parse(u)
  log.info("maybeGithub", "Attempting %s from %s", p, u)

  return addRemoteGit(u, up, name, true, function (er2, data) {
    if (er2) {
      var upriv = "git+ssh://git@github.com:" + p
        , uppriv = url.parse(upriv)

      log.info("maybeGithub", "Attempting %s from %s", p, upriv)

      return addRemoteGit(upriv, uppriv, false, name, function (er3, data) {
        if (er3) return cb(er)
        success(upriv, data)
      })
    }
    success(u, data)
  })

  function success (u, data) {
    data._from = u
    data._fromGithub = true
    return cb(null, data)
  }
}

function addLocalTarball (p, name, shasum, cb_) {
  if (typeof cb_ !== "function") cb_ = shasum, shasum = null
  if (typeof cb_ !== "function") cb_ = name, name = ""
  // if it's a tar, and not in place,
  // then unzip to .tmp, add the tmp folder, and clean up tmp
  if (p.indexOf(npm.tmp) === 0)
    return addTmpTarball(p, name, shasum, cb_)

  if (p.indexOf(npm.cache) === 0) {
    if (path.basename(p) !== "package.tgz") return cb_(new Error(
      "Not a valid cache tarball name: "+p))
    return addPlacedTarball(p, name, shasum, cb_)
  }

  function cb (er, data) {
    if (data) data._resolved = p
    return cb_(er, data)
  }

  // just copy it over and then add the temp tarball file.
  var tmp = path.join(npm.tmp, name + Date.now()
                             + "-" + Math.random(), "tmp.tgz")
  mkdir(path.dirname(tmp), function (er) {
    if (er) return cb(er)
    var from = fs.createReadStream(p)
      , to = fs.createWriteStream(tmp)
      , errState = null
    function errHandler (er) {
      if (errState) return
      return cb(errState = er)
    }
    from.on("error", errHandler)
    to.on("error", errHandler)
    to.on("close", function () {
      if (errState) return
      log.verbose("chmod", tmp, npm.modes.file.toString(8))
      fs.chmod(tmp, npm.modes.file, function (er) {
        if (er) return cb(er)
        addTmpTarball(tmp, name, shasum, cb)
      })
    })
    from.pipe(to)
  })
}

// to maintain the cache dir's permissions consistently.
var cacheStat = null
function getCacheStat (cb) {
  if (cacheStat) return cb(null, cacheStat)
  fs.stat(npm.cache, function (er, st) {
    if (er) return makeCacheDir(cb)
    if (!st.isDirectory()) {
      log.error("getCacheStat", "invalid cache dir %j", npm.cache)
      return cb(er)
    }
    return cb(null, cacheStat = st)
  })
}

function makeCacheDir (cb) {
  if (!process.getuid) return mkdir(npm.cache, cb)

  var uid = +process.getuid()
    , gid = +process.getgid()

  if (uid === 0) {
    if (process.env.SUDO_UID) uid = +process.env.SUDO_UID
    if (process.env.SUDO_GID) gid = +process.env.SUDO_GID
  }
  if (uid !== 0 || !process.env.HOME) {
    cacheStat = {uid: uid, gid: gid}
    return mkdir(npm.cache, afterMkdir)
  }

  fs.stat(process.env.HOME, function (er, st) {
    if (er) {
      log.error("makeCacheDir", "homeless?")
      return cb(er)
    }
    cacheStat = st
    log.silly("makeCacheDir", "cache dir uid, gid", [st.uid, st.gid])
    return mkdir(npm.cache, afterMkdir)
  })

  function afterMkdir (er, made) {
    if (er || !cacheStat || isNaN(cacheStat.uid) || isNaN(cacheStat.gid)) {
      return cb(er, cacheStat)
    }

    if (!made) return cb(er, cacheStat)

    // ensure that the ownership is correct.
    chownr(made, cacheStat.uid, cacheStat.gid, function (er) {
      return cb(er, cacheStat)
    })
  }
}




function addPlacedTarball (p, name, shasum, cb) {
  if (!cb) cb = name, name = ""
  getCacheStat(function (er, cs) {
    if (er) return cb(er)
    return addPlacedTarball_(p, name, cs.uid, cs.gid, shasum, cb)
  })
}

// Resolved sum is the shasum from the registry dist object, but
// *not* necessarily the shasum of this tarball, because for stupid
// historical reasons, npm re-packs each package an extra time through
// a temp directory, so all installed packages are actually built with
// *this* version of npm, on this machine.
//
// Once upon a time, this meant that we could change package formats
// around and fix junk that might be added by incompatible tar
// implementations.  Then, for a while, it was a way to correct bs
// added by bugs in our own tar implementation.  Now, it's just
// garbage, but cleaning it up is a pain, and likely to cause issues
// if anything is overlooked, so it's not high priority.
//
// If you're bored, and looking to make npm go faster, and you've
// already made it this far in this file, here's a better methodology:
//
// cache.add should really be cache.place.  That is, it should take
// a set of arguments like it does now, but then also a destination
// folder.
//
// cache.add('foo@bar', '/path/node_modules/foo', cb)
//
// 1. Resolve 'foo@bar' to some specific:
//   - git url
//   - local folder
//   - local tarball
//   - tarball url
// 2. If resolved through the registry, then pick up the dist.shasum
// along the way.
// 3. Acquire request() stream fetching bytes: FETCH
// 4. FETCH.pipe(tar unpack stream to dest)
// 5. FETCH.pipe(shasum generator)
// When the tar and shasum streams both finish, make sure that the
// shasum matches dist.shasum, and if not, clean up and bail.
//
// publish(cb)
//
// 1. read package.json
// 2. get root package object (for rev, and versions)
// 3. update root package doc with version info
// 4. remove _attachments object
// 5. remove versions object
// 5. jsonify, remove last }
// 6. get stream: registry.put(/package)
// 7. write trailing-}-less JSON
// 8. write "_attachments":
// 9. JSON.stringify(attachments), remove trailing }
// 10. Write start of attachments (stubs)
// 11. JSON(filename)+':{"type":"application/octet-stream","data":"'
// 12. acquire tar packing stream, PACK
// 13. PACK.pipe(PUT)
// 14. PACK.pipe(shasum generator)
// 15. when PACK finishes, get shasum
// 16. PUT.write('"}},') (finish _attachments
// 17. update "versions" object with current package version
// (including dist.shasum and dist.tarball)
// 18. write '"versions":' + JSON(versions)
// 19. write '}}' (versions, close main doc)

function addPlacedTarball_ (p, name, uid, gid, resolvedSum, cb) {
  // now we know it's in place already as .cache/name/ver/package.tgz
  // unpack to .cache/name/ver/package/, read the package.json,
  // and fire cb with the json data.
  var target = path.dirname(p)
    , folder = path.join(target, "package")

  lock(folder, function (er) {
    if (er) return cb(er)
    rmUnpack()
  })

  function rmUnpack () {
    rm(folder, function (er) {
      unlock(folder, function () {
        if (er) {
          log.error("addPlacedTarball", "Could not remove %j", folder)
          return cb(er)
        }
        thenUnpack()
      })
    })
  }

  function thenUnpack () {
    tar.unpack(p, folder, null, null, uid, gid, function (er) {
      if (er) {
        log.error("addPlacedTarball", "Could not unpack %j to %j", p, target)
        return cb(er)
      }
      // calculate the sha of the file that we just unpacked.
      // this is so that the data is available when publishing.
      sha.get(p, function (er, shasum) {
        if (er) {
          log.error("addPlacedTarball", "shasum fail", p)
          return cb(er)
        }
        readJson(path.join(folder, "package.json"), function (er, data) {
          er = needName(er, data)
          er = needVersion(er, data)
          if (er) {
            log.error("addPlacedTarball", "Couldn't read json in %j"
                     , folder)
            return cb(er)
          }

          data.dist = data.dist || {}
          data.dist.shasum = shasum
          deprCheck(data)
          asyncMap([p], function (f, cb) {
            log.verbose("chmod", f, npm.modes.file.toString(8))
            fs.chmod(f, npm.modes.file, cb)
          }, function (f, cb) {
            if (process.platform === "win32") {
              log.silly("chown", "skipping for windows", f)
              cb()
            } else if (typeof uid === "number"
                && typeof gid === "number"
                && parseInt(uid, 10) === uid
                && parseInt(gid, 10) === gid) {
              log.verbose("chown", f, [uid, gid])
              fs.chown(f, uid, gid, cb)
            } else {
              log.verbose("chown", "skip for invalid uid/gid", [f, uid, gid])
              cb()
            }
          }, function (er) {
            cb(er, data)
          })
        })
      })
    })
  }
}

// At this point, if shasum is set, it's something that we've already
// read and checked.  Just stashing it in the data at this point.
function addLocalDirectory (p, name, shasum, cb) {
  if (typeof cb !== "function") cb = shasum, shasum = ""
  if (typeof cb !== "function") cb = name, name = ""
  // if it's a folder, then read the package.json,
  // tar it to the proper place, and add the cache tar
  if (p.indexOf(npm.cache) === 0) return cb(new Error(
    "Adding a cache directory to the cache will make the world implode."))
  var strict = p.indexOf(npm.tmp) !== 0
                && p.indexOf(npm.cache) !== 0
  readJson(path.join(p, "package.json"), strict, function (er, data) {
    er = needName(er, data)
    er = needVersion(er, data)
    if (er) return cb(er)
    deprCheck(data)
    var random = Date.now() + "-" + Math.random()
      , tmp = path.join(npm.tmp, random)
      , tmptgz = path.resolve(tmp, "tmp.tgz")
      , placed = path.resolve( npm.cache, data.name
                             , data.version, "package.tgz" )
      , placeDirect = path.basename(p) === "package"
      , tgz = placeDirect ? placed : tmptgz
    getCacheStat(function (er, cs) {
      mkdir(path.dirname(tgz), function (er, made) {
        if (er) return cb(er)
        tar.pack(tgz, p, data, strict, function (er) {
          if (er) {
            log.error( "addLocalDirectory", "Could not pack %j to %j"
                     , p, tgz )
            return cb(er)
          }

          // if we don't get a cache stat, or if the gid/uid is not
          // a number, then just move on.  chown would fail anyway.
          if (!cs || isNaN(cs.uid) || isNaN(cs.gid)) return cb()

          chownr(made || tgz, cs.uid, cs.gid, function (er) {
            if (er) return cb(er)
            addLocalTarball(tgz, name, shasum, cb)
          })
        })
      })
    })
  })
}

function addTmpTarball (tgz, name, shasum, cb) {
  if (!cb) cb = name, name = ""
  getCacheStat(function (er, cs) {
    if (er) return cb(er)
    var contents = path.dirname(tgz)
    tar.unpack( tgz, path.resolve(contents, "package")
              , null, null
              , cs.uid, cs.gid
              , function (er) {
      if (er) {
        return cb(er)
      }
      addLocalDirectory(path.resolve(contents, "package"), name, shasum, cb)
    })
  })
}

function unpack (pkg, ver, unpackTarget, dMode, fMode, uid, gid, cb) {
  if (typeof cb !== "function") cb = gid, gid = null
  if (typeof cb !== "function") cb = uid, uid = null
  if (typeof cb !== "function") cb = fMode, fMode = null
  if (typeof cb !== "function") cb = dMode, dMode = null

  read(pkg, ver, false, function (er, data) {
    if (er) {
      log.error("unpack", "Could not read data for %s", pkg + "@" + ver)
      return cb(er)
    }
    npm.commands.unbuild([unpackTarget], true, function (er) {
      if (er) return cb(er)
      tar.unpack( path.join(npm.cache, pkg, ver, "package.tgz")
                , unpackTarget
                , dMode, fMode
                , uid, gid
                , cb )
    })
  })
}

var deprecated = {}
  , deprWarned = {}
function deprCheck (data) {
  if (deprecated[data._id]) data.deprecated = deprecated[data._id]
  if (data.deprecated) deprecated[data._id] = data.deprecated
  else return
  if (!deprWarned[data._id]) {
    deprWarned[data._id] = true
    log.warn("deprecated", "%s: %s", data._id, data.deprecated)
  }
}

function lockFileName (u) {
  var c = u.replace(/[^a-zA-Z0-9]+/g, "-").replace(/^-+|-+$/g, "")
    , h = crypto.createHash("sha1").update(u).digest("hex")
  h = h.substr(0, 8)
  c = c.substr(-32)
  log.silly("lockFile", h + "-" + c, u)
  return path.resolve(npm.config.get("cache"), h + "-" + c + ".lock")
}

var madeCache = false
var myLocks = {}
function lock (u, cb) {
  // the cache dir needs to exist already for this.
  if (madeCache) then()
  else mkdir(npm.config.get("cache"), function (er) {
    if (er) return cb(er)
    madeCache = true
    then()
  })
  function then () {
    var opts = { stale: npm.config.get("cache-lock-stale")
               , retries: npm.config.get("cache-lock-retries")
               , wait: npm.config.get("cache-lock-wait") }
    var lf = lockFileName(u)
    log.verbose("lock", u, lf)
    lockFile.lock(lf, opts, function(er) {
      if (!er) myLocks[lf] = true
      cb(er)
    })
  }
}

function unlock (u, cb) {
  var lf = lockFileName(u)
  if (!myLocks[lf]) return process.nextTick(cb)
  myLocks[lf] = false
  lockFile.unlock(lockFileName(u), cb)
}

function needName(er, data) {
  return er ? er
       : (data && !data.name) ? new Error("No name provided")
       : null
}

function needVersion(er, data) {
  return er ? er
       : (data && !data.version) ? new Error("No version provided")
       : null
}
