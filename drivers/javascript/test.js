var rethinkdb = require('./rethinkdb');

var conn = new rethinkdb.net.TcpConnection({host:'localhost', port:11211},
    function(co) {
        console.log('connected'); 

        conn.run('other expr');
        conn.run('bob');
    },
    function() {
        console.log('could not connect'); 
    }
);
