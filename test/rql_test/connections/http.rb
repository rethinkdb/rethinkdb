#!/usr/bin/env ruby

$LOAD_PATH.unshift('../../drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

$port = ARGV[0].to_i
$c = r.connect(:host => 'localhost', :port => $port).repl

def expect_eq(left, right)
    if left != right
        raise RuntimeError, "Actual value #{left} not equal to expected value #{right}"
    end
end

def expect_error(query, err_type, err_info)
    begin
        res = query.run()
    rescue err_type => ex
        if ex.message.lines[0].rstrip != err_info
            raise
        end
        return
    end
    raise RuntimeError, "Expected an error, but got success with result: #{res}"
end

def err_string(method, url, msg)
    return "Error in HTTP #{method} of `#{url}`: #{msg}."
end

def test_get()
    url = 'httpbin.org/get'

    res = r.http(url).run()
    expect_eq(res['args'], {})
    expect_eq(res['headers']['Accept-Encoding'], 'deflate=1;gzip=0.5')
    expect_eq(res['headers']['User-Agent'].split('/')[0], 'RethinkDB')
end

def test_params()
    url = 'httpbin.org/get'

    res = r.http(url, {:params => {'fake' => 123, 'things' => 'stuff', 'nil' => nil}}).run()
    expect_eq(res['args']['fake'], '123')
    expect_eq(res['args']['things'], 'stuff')
    expect_eq(res['args']['nil'], '')

    res = r.http(url + '?dummy=true', :params => {'fake' => 123.5}).run()
    expect_eq(res['args']['fake'], '123.5')
    expect_eq(res['args']['dummy'], 'true')
end

def test_headers()
    url = 'httpbin.org/headers'

    res = r.http(url, {:header => {'Test' => 'entry', 'Accept-Encoding' => 'override'}}).run()
    expect_eq(res['headers']['Test'], 'entry')
    expect_eq(res['headers']['Accept-Encoding'], 'override')

    res = r.http(url, {:header => ['Test: entry','Accept-Encoding: override']}).run()
    expect_eq(res['headers']['Test'], 'entry')
    expect_eq(res['headers']['Accept-Encoding'], 'override')
end

def test_head()
    url = 'httpbin.org/get'

    res = r.http(url, {:method => 'HEAD', :result_format => 'text'}).run()
    expect_eq(res, '')

    res = r.http(url, {:method => 'HEAD'}).run()
    expect_eq(res, nil)
end

def test_post()
    url = 'httpbin.org/post'
    post_data = {'str' => '%in fo+', 'number' => 135.5,'nil' => nil}
    res = r.http(url, {:method => 'POST',
                       :data => post_data}).run()
    post_data['number'] = post_data['number'].to_s
    post_data['nil'] = ''
    expect_eq(res['form'], post_data)

    post_data = {'str' => '%in fo+','number' => 135.5,'nil' => nil}
    res = r.http(url, {:method => 'POST',
                       :data => r.expr(post_data).coerce_to('string'),
                       :header => {'Content-Type' => 'application/json'}}).run()
    expect_eq(res['json'], post_data)

    res = r.http(url, {:method => 'POST',
                       :data => r.expr(post_data).coerce_to('string')}).run()
    post_data['str'] = '%in fo ' # Default content type is x-www-form-encoded, which changes the '+' to a space
    expect_eq(res['json'], post_data)

    post_data = 'a=b&b=c'
    res = r.http(url, {:method => 'POST',
                       :data => post_data}).run()
    expect_eq(res['form'], {'a' => 'b','b' => 'c'})

    post_data = '<arbitrary>data</arbitrary>'
    res = r.http(url, {:method => 'POST',
                       :data => post_data}).run()
    expect_eq(res['data'], post_data)
end

$obj_data = {'nested' => {'arr' => [123.45, ['a', 555], 0.123],
                          'str' => 'info',
                          'number' => 135,
                          'nil' => nil},
             'time' => r.epoch_time(1000)}

$str_data = '<arbitrary> +%data!$%^</arbitrary>'

def test_put()
    url = 'httpbin.org/put'
    res = r.http(url, {:method => 'PUT', :data => $obj_data}).run()
    expect_eq(res['json']['nested'], $obj_data['nested'])
    expect_eq(res['json']['time'], Time.at(1000));

    res = r.http(url, {:method => 'PUT', :data => $str_data}).run()
    expect_eq(res['data'], $str_data)
end

def test_patch()
    url = 'httpbin.org/patch'
    res = r.http(url, {:method => 'PATCH', :data => $obj_data}).run()
    expect_eq(res['json']['nested'], $obj_data['nested'])
    expect_eq(res['json']['time'], Time.at(1000));

    res = r.http(url, {:method => 'PATCH', :data => $str_data}).run()
    expect_eq(res['data'], $str_data)
end

def test_delete()
    url = 'httpbin.org/delete'
    res = r.http(url, {:method => 'DELETE', :data => $obj_data}).run()
    expect_eq(res['json']['nested'], $obj_data['nested'])
    expect_eq(res['json']['time'], Time.at(1000));

    res = r.http(url, {:method => 'DELETE', :data => $str_data}).run()
    expect_eq(res['data'], $str_data)
end

def test_redirects()
    url = 'httpbin.org/redirect/2'
    expect_error(r.http(url),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 302'))
    expect_error(r.http(url, {:redirects => 1}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'Number of redirects hit maximum amount'))
    res = r.http(url, {:redirects => 2}).run()
    expect_eq(res['headers']['Host'], 'httpbin.org')
end

def test_gzip()
    res = r.http('httpbin.org/gzip').run()
    expect_eq(res['gzipped'], true)
end

def test_failed_json_parse()
    url = 'httpbin.org/html'
    expect_error(r.http(url, {:result_format => 'json'}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'failed to parse JSON response'))
end

def test_basic_auth()
    url = 'http://httpbin.org/basic-auth/azure/hunter2'

    # Wrong password
    expect_error(r.http(url, {:auth => {:type => 'basic', :user => 'azure', :pass => 'wrong'}}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Wrong username
    expect_error(r.http(url, {:auth => {:type => 'basic', :user => 'fake', :pass => 'hunter2'}}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Wrong authentication type
    expect_error(r.http(url, {:auth => {:type => 'digest', :user => 'azure', :pass => 'hunter2'}}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Correct credentials
    res = r.http(url, {:auth => {:type => 'basic', :user => 'azure', :pass => 'hunter2'}}).run()
    expect_eq(res, {'authenticated' => true, 'user' => 'azure'})

    # Default auth type should be basic
    res = r.http(url, {:auth => {:user => 'azure', :pass => 'hunter2'}}).run()
    expect_eq(res, {'authenticated' => true, 'user' => 'azure'})
end

# This test requires us to set a cookie (any cookie) due to a bug in httpbin.org
# See https://github.com/kennethreitz/httpbin/issues/124
def test_digest_auth()
    url = 'http://httpbin.org/digest-auth/auth/azure/hunter2'

    # Wrong password
    expect_error(r.http(url, {:header => {'Cookie' => 'dummy'},
                              :redirects => 5,
                              :auth => {:type => 'digest', :user => 'azure', :pass => 'wrong'}}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # httpbin apparently doesn't check the username, just the password
    # Wrong username
    #expect_error(r.http(url, {:header => {'Cookie' => 'dummy'},
    #                          :redirects => 5,
    #                          :auth => {:type => 'digest', :user => 'fake', :pass => 'hunter2'}}),
    #             RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # httpbin has a 500 error on this
    # Wrong authentication type
    #expect_error(r.http(url, {:header => {'Cookie' => 'dummy'},
    #                          :redirects => 5,
    #                          :auth => {:type => 'basic', :user => 'azure', :pass => 'hunter2'}}),
    #             RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Correct credentials
    res = r.http(url, {:header => {'Cookie' => 'dummy'},
                       :redirects => 5,
                       :auth => {:type => 'digest', :user => 'azure', :pass => 'hunter2'}}).run()
    expect_eq(res, {'authenticated' => true, 'user' => 'azure'})
end

def test_verify()
    def test_part(url)
        expect_error(r.http(url, {:verify => true, :redirects => 5}),
                     RethinkDB::RqlRuntimeError, err_string('GET', url, 'Peer certificate cannot be authenticated with given CA certificates'))

        res = r.http(url, {:verify => false, :redirects => 5}).split()[0].run()
        expect_eq(res, '<html>')
    end

    test_part('http://dev.rethinkdb.com')
    test_part('https://dev.rethinkdb.com')
end

$tests = {
    :test_get => method(:test_get),
    :test_params => method(:test_params),
    :test_headers => method(:test_headers),
    :test_head => method(:test_head),
    :test_post => method(:test_post),
    :test_put => method(:test_put),
    :test_patch => method(:test_patch),
    :test_delete => method(:test_delete),
    :test_redirects => method(:test_redirects),
    :test_gzip => method(:test_gzip),
    :test_failed_json_parse => method(:test_failed_json_parse),
    :test_basic_auth => method(:test_basic_auth),
    :test_digest_auth => method(:test_digest_auth),
    :test_verify => method(:test_verify)
}

$tests.each do |name, m|
     puts "Running test #{name}"
     m.call()
     puts " - PASS"
end

