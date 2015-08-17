#!/usr/bin/env ruby

# -- import the rethinkdb driver

require_relative '../importRethinkDB.rb'

# --

$port = (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
$httpbinAddress = ARGV[1] || ENV['HTTPBIN_TEST_ADDRESS'] || 'httpbin.org'
$httpAddress = ARGV[2] || ENV['HTTP_TEST_ADDRESS'] || 'dev.rethinkdb.com'
$httpsAddress = ARGV[3] || ENV['HTTPS_TEST_ADDRESS'] || 'dev.rethinkdb.com'

if $httpAddress == 'dev.rethinkdb.com'
    $imageAddress = 'www.rethinkdb.com/assets/images/docs/api_illustrations'
else
    $imageAddress = $httpAddress
end

puts 'RethinkDB Driver Port: ' + $port.to_s

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
        if ex.message.lines.first.rstrip != err_info
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
    url = 'http://' + $httpbinAddress + '/get'

    res = r.http(url).run()
    expect_eq(res['args'], {})
    expect_eq(res['headers']['Accept-Encoding'], 'deflate;q=1, gzip;q=0.5')
    expect_eq(res['headers']['User-Agent'].split('/')[0], 'RethinkDB')
end

def test_params()
    url = 'http://' + $httpbinAddress + '/get'

    res = r.http(url, {:params => {'fake' => 123, 'things' => 'stuff', 'nil' => nil}}).run()
    expect_eq(res['args']['fake'], '123')
    expect_eq(res['args']['things'], 'stuff')
    expect_eq(res['args']['nil'], '')

    res = r.http(url + '?dummy=true', :params => {'fake' => 123.5}).run()
    expect_eq(res['args']['fake'], '123.5')
    expect_eq(res['args']['dummy'], 'true')
end

def test_headers()
    url = 'http://' + $httpbinAddress + '/headers'

    res = r.http(url, {:header => {'Test' => 'entry', 'Accept-Encoding' => 'override'}}).run()
    expect_eq(res['headers']['Test'], 'entry')
    expect_eq(res['headers']['Accept-Encoding'], 'override')

    res = r.http(url, {:header => ['Test: entry','Accept-Encoding: override']}).run()
    expect_eq(res['headers']['Test'], 'entry')
    expect_eq(res['headers']['Accept-Encoding'], 'override')
end

def test_head()
    url = 'http://' + $httpbinAddress + '/get'

    res = r.http(url, {:method => 'HEAD', :result_format => 'text'}).run()
    expect_eq(res, nil)

    res = r.http(url, {:method => 'HEAD'}).run()
    expect_eq(res, nil)
end

def test_post()
    url = 'http://' + $httpbinAddress + '/post'
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

    post_data = 'a=b&b=c'
    res = r.http(url, {:method => 'POST',
                       :data => post_data}).run()
    expect_eq(res['form'], {'a' => 'b','b' => 'c'})

    post_data = '<arbitrary>data</arbitrary>'
    res = r.http(url, {:method => 'POST',
                       :data => post_data,
                       :header => {'Content-Type' => 'application/text'}}).run()
    expect_eq(res['data'], post_data)
end

$obj_data = {'nested' => {'arr' => [123.45, ['a', 555], 0.123],
                          'str' => 'info',
                          'number' => 135,
                          'nil' => nil},
             'time' => r.epoch_time(1000)}

$str_data = '<arbitrary> +%data!$%^</arbitrary>'

def test_put()
    url = 'http://' + $httpbinAddress + '/put'
    res = r.http(url, {:method => 'PUT', :data => $obj_data}).run()
    expect_eq(res['json']['nested'], $obj_data['nested'])
    expect_eq(res['json']['time'], Time.at(1000));

    res = r.http(url, {:method => 'PUT', :data => $str_data}).run()
    expect_eq(res['data'], $str_data)
end

def test_patch()
    url = 'http://' + $httpbinAddress + '/patch'
    res = r.http(url, {:method => 'PATCH', :data => $obj_data}).run()
    expect_eq(res['json']['nested'], $obj_data['nested'])
    expect_eq(res['json']['time'], Time.at(1000));

    res = r.http(url, {:method => 'PATCH', :data => $str_data}).run()
    expect_eq(res['data'], $str_data)
end

def test_delete()
    url = 'http://' + $httpbinAddress + '/delete'
    res = r.http(url, {:method => 'DELETE', :data => $obj_data}).run()
    expect_eq(res['json']['nested'], $obj_data['nested'])
    expect_eq(res['json']['time'], Time.at(1000));

    res = r.http(url, {:method => 'DELETE', :data => $str_data}).run()
    expect_eq(res['data'], $str_data)
end

def test_redirects()
    url = 'http://' + $httpbinAddress + '/redirect/2'
    expect_error(r.http(url, {:redirects => 0}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 302'))
    expect_error(r.http(url, {:redirects => 1}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'Number of redirects hit maximum amount'))
    res = r.http(url, {:redirects => 2}).run()
    expect_eq(res['headers']['Host'], $httpbinAddress)
end

def test_gzip()
    res = r.http('http://' + $httpbinAddress + '/gzip').run()
    expect_eq(res['gzipped'], true)
end

def test_failed_json_parse()
    url = 'http://' + $httpbinAddress + '/html'
    expect_error(r.http(url, {:result_format => 'json'}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'failed to parse JSON response: Invalid value.'))
end

def test_basic_auth()
    url = 'http://' + $httpbinAddress + '/basic-auth/azure/hunter2'

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

def test_digest_auth()
    url = 'http://' + $httpbinAddress + '/digest-auth/auth/azure/hunter2'

    # Wrong password
    expect_error(r.http(url, {:redirects => 5,
                              :auth => {:type => 'digest', :user => 'azure', :pass => 'wrong'}}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Wrong username
    expect_error(r.http(url, {:redirects => 5,
                              :auth => {:type => 'digest', :user => 'fake', :pass => 'hunter2'}}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Wrong authentication type
    expect_error(r.http(url, {:redirects => 5,
                              :auth => {:type => 'basic', :user => 'azure', :pass => 'hunter2'}}),
                 RethinkDB::RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Correct credentials
    res = r.http(url, {:redirects => 5,
                       :auth => {:type => 'digest', :user => 'azure', :pass => 'hunter2'}}).run()
    expect_eq(res, {'authenticated' => true, 'user' => 'azure'})
end

def test_verify()
    def test_part(url)
        expect_error(r.http(url, {:method => 'HEAD', :verify => true, :redirects => 5}),
                     RethinkDB::RqlRuntimeError, err_string('HEAD', url, 'Peer certificate cannot be authenticated with given CA certificates'))

        res = r.http(url, {:method => 'HEAD', :verify => false, :redirects => 5}).run()
        expect_eq(res, nil)
    end

    test_part('http://' + $httpAddress + '/redirect')
    test_part('https://' + $httpsAddress)
end

def test_binary()
    res = r.http('http://' + $imageAddress + '/quickstart.png') \
           .do{|row| [row.type_of(), row.count().gt(0)]} \
           .run()
    expect_eq(res, ['PTYPE<BINARY>', true])

    res = r.http('http://' + $httpbinAddress + '/get',{:result_format => 'binary'}) \
           .do{|row| [row.type_of(), row.slice(0,1).coerce_to("string")]} \
           .run()
    expect_eq(res, ['PTYPE<BINARY>', '{'])
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
    :test_verify => method(:test_verify),
    :test_binary => method(:test_binary)
}

$tests.each do |name, m|
     puts "Running test #{name}"
     m.call()
     puts " - PASS"
end

