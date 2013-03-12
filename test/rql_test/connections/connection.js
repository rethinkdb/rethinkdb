/////
// Tests the driver API for making connections and excercising the networking code
/////

console.log("Testing JS connection API")

process.on('uncaughtException', function(err) {
    console.log("Uncaught exception:", err);
    console.log(err.toString() + err.stack.toString());
    if (cpp_server) cpp_server.kill();
    //if (js_server) js_server.kill();
});

var spawn = require('child_process').spawn

var r = require('../../../drivers/javascript/build/rethinkdb');
var build = process.env.BUILD || 'debug'
var testDefault = process.env.TEST_DEFAULT_PORT == "1"

var port = Math.floor(Math.random()*(65535 - 1025)+1025)

function sequence_tests(test, list){
    return (function (){
        #console.log("[" + list.length + "]")
        var next = list.shift();
        if(!next){
            test.done();
        }else{
            test.next.apply({}, arguments);
        }
    });
}

var assertErr = function(test, err, type, msg) {
    test.ok(err, "No Error");
    test.equal(err.constructor.name, type,
               "Error message type `"+err.constructor.name+"` doesn't match expected type `"+type+"`");
    var _msg = err.message.replace(/ in:\n([\r\n]|.)*/m, "");
    test.equal(_msg, msg,
               "Error message '"+_msg+"' doesn't match expected '"+msg+"'");
};

exports['no_server'] = function(test){

    // All these fail as there is no server to connect to

    test.expect(12);
    var cont;
    cont = sequence_tests(test, [
        function() {
            r.connect({}, cont);
        },
        function(err, c) {
            assertErr(test, err, "RqlDriverError", "Could not connect to localhost:28015.")
            r.connect({port:11221}, cont);
        },
        function(err, c) {
            assertErr(test, err, "RqlDriverError", "Could not connect to localhost:11221.")
            r.connect({host:'0.0.0.0'}, cont);
        },
        function(err, c) {
            assertErr(test, err, "RqlDriverError", "Could not connect to 0.0.0.0:28015.")
            r.connect({host:'0.0.0.0', port:11221}, cont);
        },
        function(err, c) {
            assertErr(test, err, "RqlDriverError", "Could not connect to 0.0.0.0:11221.")
            cont();
        }
    ]);
    cont();
};

exports['connect'] = function(test){

    // Run CPP server in default configuration

    //test.expect(12);
    var cont;
    var cpp_server;
    var c;
    cont = sequence_tests(test, [
        function() {
            cpp_server = spawn('../../build/'+build+'/rethinkdb',
                               ['--driver-port', port, '--http-port', '0', '--cluster-port', '0'],
                               {stdio: 'inherit'});
            setTimeout(cont, 1000);
        },

        // Test differt ways of connecting to the server
        function() {
            testDefault ? r.connect({}, cont) : cont();
        },
        function(err, c) {
            test.ifError(err);
            testDefault ? console.log("foo") : cont(); //c.reconnect(cont) : cont();
        },
        function(err, c) {
            test.ifError(err);
            testDefault ? r.connect({host:'localhost'}, cont) : cont();
        },
        function(err, c) {
            test.ifError(err);
            testDefault ? c.reconnect(cont) : cont();
        },
        function(err, c) {
            test.ifError(err);
            testDefault ? r.connect({host:'localhost', port:28015}, cont) : cont();
        },
        function(err, c) {
            test.ifError(err);
            testDefault ? c.reconnect(cont) : cont();
        },
        function(err, c) {
            test.ifError(err);
            testDefault ? r.connect({port:28015}, cont) : cont();
        },
        function(err, c) {
            test.ifError(err);
            testDefault ? c.reconnect(cont) : cont();
        },
        function(err, c) {
            test.ifError(err);
            cont();
        },

        // Tests closing the connection
        function() {
            r.connect({port:port}, cont);
        },
        function(err, c) {
            test.ifError(err);
            r(1).run(c, function(err, res) {
                test.ifError(err);
                c.close();
                c.close();
                c.reconnect(cont);
            });
        },
        function(err, c) {
            test.ifError(err);
            r(1).run(c, cont);
        },
        function(err, res) {
            test.ifError(err);
            r.connect({port:port}, cont);
        },
        function(err, c) {
            c.close();
            test.throws(function(){
                r(1).run(c, cont);
            }, "RqlDriverError");
            r.connect({port:port}, cont);
        },

        // Test `use`
        function(err, c_l) {
            test.ifError(err);
            c = c_l; // make it global to use below
            r.db('test').tableCreate('t1').run(c, cont);
        },
        function(err, res) {
            test.ifError(err);
            r.dbCreate('db2').run(c, cont);
        },
        function(err, res) {
            test.ifError(err);
            r.db('db2').tableCreate('t2').run(c, cont);
        },
        function(err, res) {
            test.ifError(err);
            c.use('db2');
            r.table('t2').run(c, cont);
        },
        function(err, res) {
            test.ifError(err);
            c.use('test');
            r.table('t2').run(c, cont);
        },
        function(err, res) {
            assertErr(test, err, "RqlRuntimeError", "Table `t2` does not exist.");
            r.connect({port:port}, cont);
        },
        function(err, c) {
            cpp_server.kill()
            setTimeout(function() {
                test.throws(function(){
                    r(1).run(c, cont);
                }, "RqlDriverError", "Connection is closed.");
            }, 100);
        },
        function(){ console.log("YAHOO!"); }
    ]);
    cont();
};

// TODO: test cursors, streaming large values
