/////
// Tests the driver API for making connections and excercising the networking code
/////

process.on('uncaughtException', function(err) {
    if (cpp_server) cpp_server.kill();
    if (js_server) js_server.kill();
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
        throw "Error message "+err+" doesn't match "+msg;
    }
};

var assertNoError = function(err) {
    if (err) {
        throw "Error not expected"
    }
};

var r = require('../../../drivers/javascript2/build/rethinkdb');

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
        cpp_server = spawn('../../../build/release_clang/rethinkdb');
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

    // Tests closing the connection
    function() {
        r.connect({}, cont);
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
        r.connect({}, cont);
    },
    function(err, c) {
        c.close();
        r(1).run(c, cont);
    },
    function(err, res) {
        assertErr(err, "RqlDriverError", "Connection is closed.");
        r.connect(c, cont);
    },
    function(err, c) {
        js_server.kill()
        setTimeout(function() {
            r(1).run(c, cont);
        }, 100);
    },
    function(err, res) {
        assertErr(err, "RqlDriverError", "Connection is closed.");
        cont();
    }
];

cont();

// TODO: test cursors, streaming large values
