// Tests the http term

process.on('uncaughtException', function(err) {
    if (err.stack) {
        console.log(err.toString() + err.stack.toString());
    } else {
        console.log(err.toString());
    }
    process.exit(1)
});

var r = require('../../../build/packages/js/rethinkdb');

function expect_no_error(err) {
    if (err) {
        throw err;
    }
}

function pass(name, conn) {
    console.log(name + " - PASS");
    conn.close();
}

function expect_error(res, err, err_type, err_info) {
    if (err) {
        if (err['name'] !== err_type) {
            throw new Error("Expected an error of type " + err_type + ", but got " + err['name'] + ": " + err['msg']);
        }
        if (err['msg'] !== err_info) {
            throw new Error('expected error: ' + err_info + '\nactual error: ' + err['msg'])
        }
    } else {
        throw new Error("Expected an error, but got " + JSON.stringify(res));
    }
}

// These two functions taken from http://stackoverflow.com/questions/1068834/
// Because javascript sucks
function count_keys(obj) {
    var count = 0;
    for (k in obj) {
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
        for (k in v1) {
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
    if (!object_eq(left, right)) {
        throw new Error("Actual result not equal to expected result:" +
                        "\n  ACTUAL: " + JSON.stringify(left) +
                        "\n  EXPECTED: " + JSON.stringify(right));
    }
}

function err_string(method, url, msg) {
    return 'Error in HTTP ' + method + ' of `' + url + '`: ' + msg + '.';
}

function test_get(conn) {
    var url = 'httpbin.org/get'

    r.http(url).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['args'], {});
            expect_eq(res['headers']['Accept-Encoding'], 'deflate=1;gzip=0.5');
            expect_eq(res['headers']['User-Agent'].split('/')[0], 'RethinkDB');
            pass("get", conn);
        });
}

function test_params(conn) {
    var url = 'httpbin.org/get'

    r.http(url, {params:{fake:123,things:'stuff',nil:null}}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['args']['fake'], '123');
            expect_eq(res['args']['things'], 'stuff');
            expect_eq(res['args']['nil'], '');
        });

    r.http(url + '?dummy=true', {params:{fake:123}}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['args']['fake'], '123');
            expect_eq(res['args']['dummy'], 'true');
            pass("params", conn);
        });
}

function test_headers(conn) {
    var url = 'httpbin.org/headers'

    r.http(url, {header:{Test:'entry','Accept-Encoding':'override'}}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['headers']['Test'], 'entry');
            expect_eq(res['headers']['Accept-Encoding'], 'override');
        });

    r.http(url, {header:['Test: entry','Accept-Encoding: override']}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['headers']['Test'], 'entry');
            expect_eq(res['headers']['Accept-Encoding'], 'override');
            pass("headers", conn);
        });
}

function test_head(conn) {
    var url = 'httpbin.org/get'

    r.http(url, {method:'HEAD', resultFormat:'text'}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res, '');
        });

    r.http(url, {method:'HEAD', resultFormat:'json'}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res, null);
            pass("head", conn);
        });
}

function test_post(conn) {
    var url = 'httpbin.org/post'
    var post_1_data = {str:'%in fo+',number:135.5,nil:null}
    r.http(url, {method:'POST', data:post_1_data}).run(conn, function(err, res) {
            expect_no_error(err);
            var expected = {str:'%in fo+',number:'135.5',nil:''};
            expect_eq(res['form'], expected);
        });

    r.http(url, {method:'POST', data:r.expr(post_1_data).coerceTo('string'),
           header:{'Content-Type':'application/json'}}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['json'], post_1_data);
        });

    r.http(url, {method:'POST', data:r.expr(post_1_data).coerceTo('string')}).run(conn, function(err, res) {
            expect_no_error(err);
            // Default content type is x-www-form-encoded, which changes the '+' to a space
            var expected = {str:'%in fo ',number:135.5,nil:null};
            expect_eq(res['json'], expected)
        });

    var post_2_data = 'a=b&b=c'
    r.http(url, {method:'POST', data:post_2_data}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['form'], {a:'b',b:'c'});
        });

    var post_3_data = '<arbitrary>data</arbitrary>'
    r.http(url, {method:'POST', data:post_3_data}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['data'], post_3_data)
            pass("post", conn);
        });
}

function test_put(conn) {
    var url = 'httpbin.org/put'
    var put_1_data = {nested:{arr:[123.45, ['a', 555], 0.123],str:'info',number:135,nil:null},time:r.epochTime(1000)}
    r.http(url, {method:'PUT', data:put_1_data}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['json']['nested'], put_1_data['nested']);
            expected_time = new Date(1970, 1, 1, 0, 16, 40);
            expect_eq(res['json']['time'].getTime(), expected_time.getTime());
        });

    var put_2_data = '<arbitrary> +%data!$%^</arbitrary>'
    r.http(url, {method:'PUT', data:put_2_data}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['data'], put_2_data);
            pass("put", conn);
        });
}

function test_patch(conn) {
    var url = 'httpbin.org/patch'
    var patch_1_data = {nested:{arr:[123.45, ['a', 555], 0.123],str:'info',number:135,nil:null},time:r.epochTime(1000)}
    r.http(url, {method:'PATCH', data:patch_1_data}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['json']['nested'], patch_1_data['nested']);
            expected_time = new Date(1970, 1, 1, 0, 16, 40);
            expect_eq(res['json']['time'].getTime(), expected_time.getTime());
        });

    var patch_2_data = '<arbitrary> +%data!$%^</arbitrary>'
    r.http(url, {method:'PATCH', data:patch_2_data}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['data'], patch_2_data);
            pass("patch", conn);
        });
}

function test_delete(conn) {
    var url = 'httpbin.org/delete'
    var delete_1_data = {nested:{arr:[123.45, ['a', 555], 0.123],str:'info',number:135,nil:null},time:r.epochTime(1000),nil:null}
    r.http(url, {method:'DELETE', data:delete_1_data}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['json']['nested'], delete_1_data['nested']);
            expected_time = new Date(1970, 1, 1, 0, 16, 40);
            expect_eq(res['json']['time'].getTime(), expected_time.getTime());
        });

    delete_2_data = '<arbitrary> +%data!$%^</arbitrary>'
    r.http(url, {method:'DELETE', data:delete_2_data}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['data'], delete_2_data);
            pass("delete", conn);
        });
}

function test_redirects(conn) {
    var url = 'httpbin.org/redirect/2'
    r.http(url).run(conn, function(err, res) {
            expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'status code 302'));
        });

    r.http(url, {redirects:1}).run(conn, function(err, res) {
            expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'Number of redirects hit maximum amount'));
        });

    r.http(url, {redirects:2}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['headers']['Host'], 'httpbin.org');
            pass("redirects", conn);
        });
}

function test_gzip(conn) {
    r.http('httpbin.org/gzip').run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res['gzipped'], true);
            pass("gzip", conn);
        });
}

function test_failed_json_parse(conn) {
    var url = 'httpbin.org/html'
    r.http(url, {resultFormat:'json'}).run(conn, function(err, res) {
            expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'failed to parse JSON response'));
            pass("failed json parse", conn);
        });
}

function test_basic_auth(conn) {
    var url = 'http://httpbin.org/basic-auth/azure/hunter2'

    // Wrong password
    r.http(url, {auth:{type:'basic',user:'azure',pass:'wrong'}}).run(conn, function(err, res) {
            expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'status code 401'));
        });

    // Wrong username
    r.http(url, {auth:{type:'basic',user:'fake',pass:'hunter2'}}).run(conn, function(err, res) {
            expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'status code 401'));
        });

    // Wrong authentication type
    r.http(url, {auth:{type:'digest',user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
            expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'status code 401'));
        });

    // Correct credentials
    r.http(url, {auth:{type:'basic',user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res, {authenticated:true,user:'azure'});
        });

    // Default auth type should be basic
    r.http(url, {auth:{user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res, {authenticated:true,user:'azure'});
            pass("basic auth", conn);
        });
}

// This test requires us to set a cookie (any cookie) due to a bug in httpbin.org
// See https://github.com/kennethreitz/httpbin/issues/124
function test_digest_auth(conn) {
    var url = 'http://httpbin.org/digest-auth/auth/azure/hunter2'

    // Wrong password
    r.http(url, {header:{Cookie:'dummy'}, redirects:5,
           auth:{type:'digest',user:'azure',pass:'wrong'}}).run(conn, function(err, res) {
            expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'status code 401'));
        });

    // httpbin apparently doesn't check the username, just the password
    // Wrong username
    //r.http(url, {header:{'Cookie':'dummy'}, redirects:5,
    //       auth:{type:'digest',user:'fake',pass:'hunter2'}}).run(conn, function(err, res) {
    //        expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'status code 401'));
    //    });

    // httpbin has a 500 error on this
    // Wrong authentication type
    //r.http(url, {header:{'Cookie':'dummy'}, redirects:5,
    //       auth:{type:'basic',user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
    //        expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'status code 401'));
    //    });

    // Correct credentials
    r.http(url, {header:{Cookie:'dummy'}, redirects:5,
           auth:{type:'digest',user:'azure',pass:'hunter2'}}).run(conn, function(err, res) {
            expect_no_error(err);
            expect_eq(res, {authenticated:true,user:'azure'});
            pass("digest auth", conn);
        });
}

function test_verify(conn) {
    function test_part(url, done) {
        r.http(url, {verify:true, redirects:5}).run(conn, function(err, res) {
                expect_error(res, err, 'RqlRuntimeError', err_string('GET', url, 'Peer certificate cannot be authenticated with given CA certificates'));
            });

        r.http(url, {verify:false, redirects:5}).split().nth(0).run(conn, function(err, res) {
                expect_no_error(err);
                expect_eq(res, '<html>');
                done();
            });
    }

    test_part('http://dev.rethinkdb.com', function() { });
    test_part('https://dev.rethinkdb.com', function() { pass("verify", conn); });
}

tests = {
         verify: test_verify,
         get: test_get,
         head: test_head,
         params: test_params,
         headers: test_headers,
         post: test_post,
         put: test_put,
         patch: test_patch,
         delete: test_delete,
         redirects: test_redirects,
         gzip: test_gzip,
         failed_json_parse: test_failed_json_parse,
         digest_auth: test_digest_auth,
         basic_auth: test_basic_auth
         };

var port = parseInt(process.argv[2], 10)
console.log('Connecting to localhost:' + port.toString());

for (var name in tests) {
    r.connect({port:port}, function(err, c) {
        if (err) {
            throw new Error('Failed to connect to localhost:' + port.toString());
        }

        tests[name](c);
    });
}

