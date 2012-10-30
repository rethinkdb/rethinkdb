# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/ruby
require 'pp'
require 'socket'
require 'optparse'

$opt = {}
OptionParser.new {|opts|
  opts.banner = "Usage: run.rb [options]"

  $opt[:host] = "localhost"
  opts.on('-h', '--host HOST', 'Fuzz on HOST (default localhost)') {|h| $opt[:host] = h}

  $opt[:port] = "0"
  opts.on('-p', '--port PORT', 'Fuzz on PORT+28015 (default 0)') {|p| $opt[:port] = p}

  $opt[:seed] = srand()
  opts.on('-s', '--seed SEED', 'Set the random seed to SEED') {|s| $opt[:seed] = s}

  $opt[:tfile] = 'sexp.txt'
  opts.on('-t', '--templates FILE', 'Read templates from FILE') {|f| $opt[:tfile] = f}

  $opt[:ind] = 0
  opts.on('-i', '--index INDEX', 'Start at index INDEX') {|i| $opt[:ind] = i.to_i}

  $opt[:log] = nil
  opts.on('-l', '--log LOG', 'Log packets to LOG') {|l| $opt[:log] = l}
}.parse!

print "Using seed: #{$opt[:seed]}\n"
srand($opt[:seed].to_i)

$templates = []
print "Using templates from file: #{$opt[:tfile]}\n"
File.open($opt[:tfile], "r").each {|l| $templates << eval(l.chomp)}

host = $opt[:host]
port = ($opt[:port].to_i)+28015
print "Connecting to #{host}:#{port}...\n"
$sock = TCPSocket.open(host, port)
$sock.send([0xaf61ba35].pack('L<'), 0)
print "Connection established.\n"

print "Setting up environment..."
$LOAD_PATH.unshift('../rethinkdb')
require 'rethinkdb.rb'
$c = RethinkDB::Connection.new(host, port)
include RethinkDB::Shortcuts
begin
  r.create_db('default_db').run
  r.db('test').create_table('Welcome_rdb').run
  r.db('default_db').create_table('Welcome_rdb').run
  print "Environment established.\n"
rescue Exception => e
end
load 'transformation.rb'
$transform = Transformer.new('transformation_conf.rb')

def maybe_log packet
  return if not $opt[:log]
  File.open($opt[:log], 'a') {|f| f.write([$$, Time.now.to_f, packet].inspect + "\n")}
end

def bsend s
  if s.nil?
    print "skipping\n"
  elsif $i >= $opt[:ind]
    print "sending:  #{s.inspect}\n"
    packet = [s.length].pack('L<') + s
    maybe_log packet
    $sock.send(packet, 0)
    res = len = $sock.recv(4).unpack('L<')[0]
    res = $sock.recv(len) if len
    if not res
      protob = Query.new.parse_from_string(s)
      abort "crashed with: #{packet.inspect} (#{protob.inspect})\n"
    else
      begin
        pres = Response.new.parse_from_string(res)
        PP.pp pres
      rescue Exception => e
        print "Unparsable response: #{pres}\n"
      end
    end
  end
end

$i=0
while true
  $templates.each {|sexp|
    print "template: #{sexp.inspect}\n"
    $i += 1
    payload = $transform[sexp]
    bsend(payload)
  }
end
