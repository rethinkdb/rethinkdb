/////
// Tests the driver API for making connections and excercising the networking code
/////

console.log("Testing JS connection API")

process.on('uncaughtException', function(err) {
    if (cpp_server) cpp_server.kill();
    //if (js_server) js_server.kill();
});

var spawn = require('child_process').spawn

var cont = function() {
    var nextAction = actions.shift()
    if (!nextAction) throw "done";
    nextAction.apply({}, arguments);
};

var assertErr = function(err, type, msg) {
    if (!err) {
        throw new Error("No Error");
    }

    if (!(err.constructor.name == type) || !(err.message == msg)) {
        throw new Error("Error message "+err+" doesn't match "+msg);
    }
};

var assertNoError = function(err) {
    if (err) {
        throw new Error("Error "+err+" not expected")
    }
};

var r = require('../../../drivers/javascript/build/rethinkdb');
var build = process.argv[2] || 'debug'
var testDefault = process[3] || false

var port = Math.floor(Math.random()*(65535 - 1025)+1025)

var actions = [

    // All these fail as there is on server to connect to
    function() {
        r.connect({}, cont);
    },
    function(err, c) {
        assertErr(err, "RqlDriverError", "Could not connect to localhost:28015.")
        r.connect({port:11221}, cont);
    },
    function(err, c) {
        assertErr(err, "RqlDriverError", "Could not connect to localhost:11221.")
        r.connect({host:'0.0.0.0'}, cont);
    },
    function(err, c) {
        assertErr(err, "RqlDriverError", "Could not connect to 0.0.0.0:28015.")
        r.connect({host:'0.0.0.0', port:11221}, cont);
    },
    function(err, c) {
        assertErr(err, "RqlDriverError", "Could not connect to 0.0.0.0:11221.")
        cont();
    },

    // Run CPP server in default configuration
    function() {
        cpp_server = spawn('../../build/'+build+'/rethinkdb',
            ['--driver-port', port, '--http-port=0', '--cluster-port=0'],
            {stdio: 'inherit'});
        setTimeout(cont, 1000);
    },

    // Test differt ways of connecting to the server
    function() {
        testDefault ? r.connect({}, cont) : cont();
    },
    function(err, c) {
        assertNoError(err);
        testDefault ? c.reconnect(cont) : cont();
    },
    function(err, c) {
        assertNoError(err);
        testDefault ? r.connect({host:'localhost'}, cont) : cont();
    },
    function(err, c) {
        assertNoError(err);
        testDefault ? c.reconnect(cont) : cont();
    },
    function(err, c) {
        assertNoError(err);
        testDefault ? r.connect({host:'localhost', port:28015}, cont) : cont();
    },
    function(err, c) {
        assertNoError(err);
        testDefault ? c.reconnect(cont) : cont();
    },
    function(err, c) {
        assertNoError(err);
        testDefault ? r.connect({port:28015}, cont) : cont();
    },
    function(err, c) {
        assertNoError(err);
        testDefault ? c.reconnect(cont) : cont();
    },
    function(err, c) {
        assertNoError(err);
        cont();
    },

    /*
    // Kill CPP server, try again with JS server
    function() {
        cpp_server.kill();
        js_server = spawn('../../../build/release_clang/rethinkdb');
        setTimeout(cont, 100);
    },

    // Test differt ways of connecting to the server
    function() {
        r.connect({}, cont);
    },
    function(err, c) {
        assertNoError(err);
        c.reconnect(cont);
    },
    function(err, c) {
        assertNoError(err);
        r.connect({host:'localhost'}, cont);
    },
    function(err, c) {
        assertNoError(err);
        c.reconnect(cont);
    },
    function(err, c) {
        assertNoError(err);
        r.connect({host:'localhost', port:28015}, cont);
    },
    function(err, c) {
        assertNoError(err);
        c.reconnect(cont);
    },
    function(err, c) {
        assertNoError(err);
        r.connect({port:28015}, cont);
    },
    function(err, c) {
        assertNoError(err);
        c.reconnect(cont);
    },
    function(err, c) {
        assertNoError(err);
        cont();
    },
    */

    // Tests closing the connection
    function() {
        r.connect({port:port}, cont);
    },
    function(err, c) {
        assertNoError(err);
        r(1).run(c, function(err, res) {
            assertNoError(err);
            c.close(); 
            c.close();
            c.reconnect(cont);
        });
    },
    function(err, c) {
        assertNoError(err);
        r(1).run(c, cont);
    },
    function(err, res) {
        assertNoError(err);
        r.connect({port:port}, cont);
    },
    function(err, c) {
        c.close();
        try {
            r(1).run(c, cont);
        } catch(err) {
            assertErr(err, "RqlDriverError", "Connection is closed.");
        }
        r.connect({port:port}, cont);
    },

    // Test `use`
    function(err, c_l) {
        assertNoError(err);
        c = c_l; // make it global to use below
        r.db('test').table_create('t1').run(c, cont);
    },
    function(err, res) {
        assertNoError(err);
        r.db_create('db2').run(c, cont);
    },
    function(err, res) {
        assertNoError(err);
        r.db('db2').table_create('t2').run(c, cont);
    },
    function(err, res) {
        assertNoError(err);

        c.use('db2');
        r.table('t2').run(c, cont);
    },
    function(err, res) {
        assertNoError(err);

        c.use('test');
        r.table('t2').run(c, cont);
    },
    function(err, res) {
        assertErr(err, "RqlRuntimeError", "Table `t2` does not exist.");
        r.connect({port:port}, cont);
    },
    function(err, c) {
        cpp_server.kill()
        setTimeout(function() {
            try {
                r(1).run(c, cont);
            } catch(err) {
                assertErr(err, "RqlDriverError", "Connection is closed.");
                cont();
            }
        }, 100);
    },
    function(err, res) {
        console.log("done running tests");
        cont();
    }
];

cont();

// TODO: test cursors, streaming large values
