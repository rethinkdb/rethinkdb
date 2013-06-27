# This file implements an abstraction layer designed to allow
# for multiple protobuf backends depending on what's avilable
# in the current envionrment (browser or server)

goog.require("Term")
goog.require("Datum")
goog.require("Query")
goog.require("VersionDummy")
goog.require("goog.proto2.WireFormatSerializer")

goog.provide("rethinkdb.protobuf")

class BrowserQuery
    AssocPair: class
        constructor: -> @ap = new Query.AssocPair()
        setKey: (key) -> @ap.setKey(key)
        setVal: (val) -> @ap.setVal(val.t)

    constructor: -> @q = new Query()
    setType: (type) -> @q.setType Query.QueryType[type]
    setQuery: (term) -> @q.setQuery term.t
    setToken: (token) -> @q.setToken token
    addGlobalOptargs: (ap) -> @q.addGlobalOptargs ap.ap
    serialize: -> ((new goog.proto2.WireFormatSerializer).serialize(@q)).buffer

class BrowserTerm
    AssocPair: class
        constructor: -> @ap = new Term.AssocPair()
        setKey: (key) -> @ap.setKey(key)
        setVal: (val) -> @ap.setVal(val.t)

    constructor: ->  @t = new Term()
    setType: (type) -> @t.setType Term.TermType[type]
    setDatum: (datum) -> @t.setDatum datum.d
    addArgs: (arg) -> @t.addArgs arg.t
    addOptargs: (ap) -> @t.addOptargs ap.ap

class BrowserDatum
    constructor: (d) -> @d = (if d? then d else new Datum())
    setType: (type) -> @d.setType Datum.DatumType[type]
    setRNum: (num) -> @d.setRNum num
    setRBool: (bool) -> @d.setRBool bool
    setRStr: (str) -> @d.setRStr str

    getType: -> @d.getType()
    getRNum: -> @d.getRNum()
    getRBool: -> @d.getRBool()
    getRStr: -> @d.getRStr()
    rArrayArray: -> @d.rArrayArray()
    rObjectArray: -> @d.rObjectArray()

class BrowserResponse
    # Static constructor
    parse: (data) ->
        br = new BrowserResponse
        br.r = (new goog.proto2.WireFormatSerializer).deserialize(Response.getDescriptor(), data)
        return br

    getToken: -> @r.getToken()
    getType: -> @r.getType()
    getResponse: (index) -> new BrowserDatum(@r.getResponse(index))
    responseArray: -> (new BrowserDatum(d) for d in @r.responseArray())

    getBacktraceArray: -> @r.getBacktrace().framesArray()




class NodeQuery
    AssocPair: class
        setKey: (key) -> @key = key
        setVal: (val) -> @val = val

    constructor: -> @global_optargs = []
    setType: (type) -> @type = type
    setQuery: (term) -> @query = term
    setToken: (token) -> @token = token
    addGlobalOptargs: (ap) -> @global_optargs.push(ap)
    serialize: -> toArrayBuffer(nodePB.Serialize(@, "Query"))

class NodeTerm
    AssocPair: class
        setKey: (key) -> @key = key
        setVal: (val) -> @val = val

    constructor: ->
        @args = []
        @optargs = []
    setType: (type) -> @type = type
    setDatum: (datum) -> @datum = datum
    addArgs: (arg) -> @args.push(arg)
    addOptargs: (ap) -> @optargs.push(ap)

class NodeDatum
    AssocPair: class
        constructor: (ap) ->
            if ap?
                ap.__proto__ = @__proto__
                return ap
            return @

        setKey: (key) -> @key = key
        setVal: (val) -> @val = val

        getKey: -> @key
        getVal: -> (new NodeDatum(@val))

    constructor: (d) ->
        if d?
            d.__proto__ = @__proto__
            return d
        return @

    setType: (type) -> @type = type
    setRNum: (num) -> @r_num = num
    setRBool: (bool) -> @r_bool = bool
    setRStr: (str) -> @r_str = str

    getType: -> Datum.DatumType[@type]
    getRNum: -> @r_num
    getRBool: -> @r_bool
    getRStr: -> @r_str
    rArrayArray: -> ((new NodeDatum(d)) for d in @r_array)
    rObjectArray: -> ((new NodeDatum::AssocPair(ap)) for ap in @r_object)

class NodeResponse
    Frame: class
        constructor: (f) ->
            f.__proto__ = @__proto__

        getType: -> Frame.FrameType[@type]
        getPos: -> @pos
        getOpt: -> @opt

    # Static constructor
    parse: (data) ->
        obj = nodePB.Parse(data, "Response")
        obj.__proto__ = new NodeResponse()
        return obj

    getToken: -> @token
    getType: -> Response.ResponseType[@type]
    getResponse: (index) -> (new NodeDatum(@response[index]))
    responseArray: -> (new NodeDatum(d) for d in @response)
    getBacktraceArray: -> (new NodeResponse::Frame(f) for f in @backtrace.frames)

# Test for avilablility of native backend and enable it if available
if require? and require('node-protobuf')
    # Initialize message serializer with
    desc = require('fs').readFileSync(__dirname + "/ql2.desc")
    protobuf = require('node-protobuf').Protobuf
    nodePB = new protobuf(desc)

    QueryPB = NodeQuery
    TermPB = NodeTerm
    DatumPB = NodeDatum
    ResponsePB = NodeResponse
else
    # Fallback on native JS backend
    QueryPB = BrowserQuery
    TermPB = BrowserTerm
    DatumPB = BrowserDatum
    ResponsePB = BrowserResponse
