var rethinkdb = require('./rethinkdb');

// This script is designed to run bare in node.js or included in a 
// html file to test in the browser.

var conn = new rethinkdb.net.TcpConnection(['bob',{host:'localhost', port:11211}],
    function() {
        console.log('connected'); 
    },
    function() {
        console.log('could not connect'); 
    }
);

console.log("done");
