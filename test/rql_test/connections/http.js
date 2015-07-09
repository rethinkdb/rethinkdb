// Tests the http term

process.on('uncaughtException', function(err) {
    if (err.stack) {
        console.log(err.toString() + err.stack.toString());
    } else {
        console.log(err.toString());
    }
    process.exit(1)
});

var assert = require('assert');
var path = require('path');

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// --

var httpbinAddress = process.env.HTTPBIN_TEST_ADDRESS || 'httpbin.org';
var httpAddress = process.env.HTTP_TEST_ADDRESS || 'dev.rethinkdb.com';
var httpsAddress = process.env.HTTPS_TEST_ADDRESS || 'dev.rethinkdb.com';
var port = parseInt(process.env.RDB_DRIVER_PORT, 10);

var imageAddress = httpAddress;
if (imageAddress == 'dev.rethinkdb.com') {
    imageAddress = 'www.rethinkdb.com/assets/images/docs/api_illustrations';
}

var withConnection = function(f){
    return function(done){
        r.connect({port:port}, function(err, c) {
            assert(!err, 'Failed to connect to localhost:' + port.toString());
            f(done, c);
        });
    };
};

function expect_no_error(err) {
    if (err) {
        throw err;
    }
}

function expect_error(res, err, err_type, err_info) {
    assert(err, "Expected an error, but got " + JSON.stringify(res));
    assert.strictEqual(err['name'], err_type,
        "Expected an error of type " + err_type + ", but got " + err['name'] + ": " + err['msg']);
    assert.strictEqual(err['msg'].indexOf(err_info), 0,
        'Expected error: ' + err_info + '\nActual error: ' + err['msg'])
}

// These two functions taken from http://stackoverflow.com/questions/1068834/
// Because javascript sucks
function count_keys(obj) {
    var count = 0;
    for (var k in obj) {
        if (obj.hasOwnProperty(k)) {
            count++;
        }
    }
    return count;
};

function object_eq(v1, v2) {
    if (typeof(v1) !== typeof(v2)) {
        return false;
    }

    if (typeof(v1) === "function") {
        return v1.toString() === v2.toString();
    }

    if (v1 instanceof Object && v2 instanceof Object) {
        if (count_keys(v1) !== count_keys(v2)) {
            return false;
        }
        var r = true;
        for (var k in v1) {
            r = object_eq(v1[k], v2[k]);
            if (!r) {
                return false;
            }
        }
        return true;
    } else {
        return v1 === v2;
    }
}

function expect_eq(left, right) {
    assert(object_eq(left, right),
           "Actual result not equal to expected result:" +
           "\n  ACTUAL: " + JSON.stringify(left) +
           "\n  EXPECTED: " + JSON.stringify(right));
}

function err_string(method, url, msg) {
    return 'Error in HTTP ' + method + ' of `' + url + '`: ' + msg + '.';
}

describe('Javascript HTTP test - ', function() {
    describe('GET', function() {
        var url = 'http://' + httpbinAddress + '/get'
        it('check basic GET result', withConnection(function(done, conn) {
            r.http(url).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['args'], {});
                expect_eq(res['headers']['Accept-Encoding'], 'deflate;q=1, gzip;q=0.5');
                expect_eq(res['headers']['User-Agent'].split('/')[0], 'RethinkDB');
                done();
            });
        }));
    });

    describe('params', function() {
        var url = 'http://' + httpbinAddress + '/get'
        it('with null', withConnection(function(done, conn) {
            r.http(url, {params:{fake:123,things:'stuff',nil:null}}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['args']['fake'], '123');
                expect_eq(res['args']['things'], 'stuff');
                expect_eq(res['args']['nil'], '');
                done();
            });
        }));
        it('with existing url params', withConnection(function(done, conn) {
            r.http(url + '?dummy=true', {params:{fake:123}}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['args']['fake'], '123');
                expect_eq(res['args']['dummy'], 'true');
                done();
            });
        }));
    });

    describe('header', function() {
        var url = 'http://' + httpbinAddress + '/headers'
        it('object', withConnection(function(done, conn) {
            r.http(url, {header:{Test:'entry','Accept-Encoding':'override'}}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['headers']['Test'], 'entry');
                expect_eq(res['headers']['Accept-Encoding'], 'override');
                done();
            });
        }));
        it('array', withConnection(function(done, conn) {
            r.http(url, {header:['Test: entry','Accept-Encoding: override']}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['headers']['Test'], 'entry');
                expect_eq(res['headers']['Accept-Encoding'], 'override');
                done();
            });
        }));
    });

    describe('HEAD', function() {
        var url = 'http://' + httpbinAddress + '/get'
        it('with text resultFormat', withConnection(function(done, conn) {
            r.http(url, {method:'HEAD', resultFormat:'text'}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res, null);
                done();
            });
        }));
        it('with json resultFormat', withConnection(function(done, conn) {
            r.http(url, {method:'HEAD', resultFormat:'json'}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res, null);
                done();
            });
        }));
    });

    describe('POST', function() {
        var url = 'http://' + httpbinAddress + '/post'
        it('form-encoded object', withConnection(function(done, conn) {
            var post_data = {str:'%in fo+',number:135.5,nil:null}
            r.http(url, {method:'POST', data:post_data}).run(conn, function(err, res) {
                expect_no_error(err);
                var expected = {str:'%in fo+',number:'135.5',nil:''};
                expect_eq(res['form'], expected);
                done();
            });
        }));
        it('json object', withConnection(function(done, conn) {
            var post_data = {str:'%in fo+',number:135.5,nil:null}
            r.http(url, {method:'POST', data:r.expr(post_data).coerceTo('string'),
                   header:{'Content-Type':'application/json'}}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['json'], post_data);
                done();
            });
        }));
        it('form-encoded string', withConnection(function(done, conn) {
            var post_data = 'a=b&b=c'
            r.http(url, {method:'POST', data:post_data}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['form'], {a:'b',b:'c'});
                done();
            });
        }));
        it('arbitrary string', withConnection(function(done, conn) {
            var post_data = '<arbitrary>data</arbitrary>'
            r.http(url, {method:'POST',
                         data:post_data,
                         header:{'Content-Type':'application/text'}}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['data'], post_data)
                done();
            });
        }));
    });

    describe('PUT', function() {
        var url = 'http://' + httpbinAddress + '/put'
        it('object', withConnection(function(done, conn) {
            var put_data = {nested:{arr:[123.45, ['a', 555], 0.123],str:'info',number:135,nil:null},time:r.epochTime(1000)}
            r.http(url, {method:'PUT', data:put_data}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['json']['nested'], put_data['nested']);
                expect_eq(res['json']['time'].getTime(), 1000000);
                done();
            });
        }));
        it('arbitrary string', withConnection(function(done, conn) {
            var put_data = '<arbitrary> +%data!$%^</arbitrary>'
            r.http(url, {method:'PUT', data:put_data}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['data'], put_data);
                done();
            });
        }));
    });

    describe('PATCH', function() {
        var url = 'http://' + httpbinAddress + '/patch'
        it('object', withConnection(function(done, conn) {
            var patch_data = {nested:{arr:[123.45, ['a', 555], 0.123],str:'info',number:135,nil:null},time:r.epochTime(1000)}
            r.http(url, {method:'PATCH', data:patch_data}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['json']['nested'], patch_data['nested']);
                expect_eq(res['json']['time'].getTime(), 1000000);
                done();
            });
        }));
        it('arbitrary string', withConnection(function(done, conn) {
            var patch_data = '<arbitrary> +%data!$%^</arbitrary>'
            r.http(url, {method:'PATCH', data:patch_data}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['data'], patch_data);
                done();
            });
        }));
    });

    describe('DELETE', function() {
        var url = 'http://' + httpbinAddress + '/delete'
        it('object', withConnection(function(done, conn) {
            var delete_data = {nested:{arr:[123.45, ['a', 555], 0.123],str:'info',number:135,nil:null},time:r.epochTime(1000)}
            r.http(url, {method:'DELETE', data:delete_data}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['json']['nested'], delete_data['nested']);
                expect_eq(res['json']['time'].getTime(), 1000000);
                done();
            });
        }));
        it('arbitrary string', withConnection(function(done, conn) {
            var delete_data = '<arbitrary> +%data!$%^</arbitrary>'
            r.http(url, {method:'DELETE', data:delete_data}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['data'], delete_data);
                done();
            });
        }));
    });

    describe('redirect', function() {
        var url = 'http://' + httpbinAddress + '/redirect/2'
        it('with no redirects allowed', withConnection(function(done, conn) {
            r.http(url, {redirects:0}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'status code 302'));
                done();
            });
        }));
        it('with not enough redirects allowed', withConnection(function(done, conn) {
            r.http(url, {redirects:1}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'Number of redirects hit maximum amount'));
                done();
            });
        }));
        it('with enough redirects allowed', withConnection(function(done, conn) {
            r.http(url, {redirects:2}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['headers']['Host'], httpbinAddress);
                done();
            });
        }));
    });

    describe('gzip', function() {
        var url = 'http://' + httpbinAddress + '/gzip';
        it('check decompression', withConnection(function(done, conn) {
            r.http(url).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res['gzipped'], true);
                done();
            });
        }));
    });

    describe('errors - ', function() {
        var url = 'http://' + httpbinAddress + '/html'
        it('failed json parse', withConnection(function(done, conn) {
            r.http(url, {resultFormat:'json'}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'failed to parse JSON response: Invalid value.'));
                done();
            });
        }));
    });

    describe('basic auth', function() {
        var url = 'http://' + httpbinAddress + '/basic-auth/azure/hunter2'
        it('wrong password', withConnection(function(done, conn) {
            r.http(url, {auth:{type:'basic',user:'azure',pass:'wrong'}}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'status code 401'));
                done();
            });
        }));
        it('wrong username', withConnection(function(done, conn) {
            r.http(url, {auth:{type:'basic',user:'fake',pass:'hunter2'}}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'status code 401'));
                done();
            });
        }));
        it('wrong auth type', withConnection(function(done, conn) {
            r.http(url, {auth:{type:'digest',user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'status code 401'));
                done();
            });
        }));
        it('correct credentials', withConnection(function(done, conn) {
            r.http(url, {auth:{type:'basic',user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res, {authenticated:true,user:'azure'});
                done();
            });
        }));
        it('default auth type', withConnection(function(done, conn) {
            r.http(url, {auth:{user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res, {authenticated:true,user:'azure'});
                done();
            });
        }));
    });

    describe('digest auth', function() {
        var url = 'http://' + httpbinAddress + '/digest-auth/auth/azure/hunter2'
        it('wrong password', withConnection(function(done, conn) {
            r.http(url, {redirects:5,
                auth:{type:'digest',user:'azure',pass:'wrong'}}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'status code 401'));
                done();
            });
        }));
        it('wrong username', withConnection(function(done, conn) {
            r.http(url, {redirects:5,
                auth:{type:'digest',user:'fake',pass:'hunter2'}}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'status code 401'));
                done();
            });
        }));
        it('wrong auth type', withConnection(function(done, conn) {
            r.http(url, {redirects:5,
                auth:{type:'basic',user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError', err_string('GET', url, 'status code 401'));
                done();
            });
        }));
        it('correct credentials', withConnection(function(done, conn) {
            r.http(url, {redirects:5,
                   auth:{type:'digest',user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res, {authenticated:true,user:'azure'});
                done();
            });
        }));
    });

    describe('verify', function() {
        function test_part(url, done, conn) {
            r.http(url, {method:'HEAD', verify:true, redirects:5}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlNonExistenceError',
                             err_string('HEAD', url, 'Peer certificate cannot be ' +
                                        'authenticated with given CA certificates'));
            });

            r.http(url, {method:'HEAD', verify:false, redirects:5}).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res, null);
                done();
            });
        }
        it('HTTP', withConnection(function(done, conn) {
            test_part('http://' + httpAddress + '/redirect', done, conn);
        }));
        it('HTTPS', withConnection(function(done, conn) {
            test_part('https://' + httpsAddress, done, conn);
        }));
    });

    describe('binary', function() {
        it('resultFormat: auto', withConnection(function(done, conn) {
            r.http('http://' + imageAddress + '/quickstart.png')
             .do(function(row){return [row.typeOf(), row.count().gt(0)]})
             .run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res[0], "PTYPE<BINARY>");
                expect_eq(res[1], true);
                done();
            });
        }));
        it('resultFormat: binary', withConnection(function(done, conn) {
            r.http('http://' + httpbinAddress + '/get',{resultFormat:"binary"})
             .do(function(row){return [row.typeOf(), row.slice(0,1).coerceTo("string")]})
             .run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res[0], "PTYPE<BINARY>");
                expect_eq(res[1], "{");
                done();
            });
        }));
    });
});
