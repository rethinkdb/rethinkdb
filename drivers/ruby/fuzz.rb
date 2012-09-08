#!/usr/bin/ruby
require 'pp'
require 'socket'
require 'optparse'

options = {}
OptionParser.new {|opts|
  opts.banner = "Usage: fuzz.rb [options]"

  options[:port] = "0"
  opts.on('-p', '--port PORT', 'Fuzz on PORT+12346 (default 0)') {|p| options[:port] = p}

  options[:seed] = srand()
  opts.on('-s', '--seed SEED', 'Set the random seed to SEED') {|s| options[:seed] = s}

  options[:tfile] = nil
  opts.on('-t', '--templates FILE', 'Read templates from FILE') {|f| options[:tfile] = f}
}.parse!

print "Using seed: #{options[:seed]}\n"
srand(options[:seed].to_i)

templates = []
if options[:tfile]
  print "Using templates from file: #{options[:tfile]}\n"
  File.open(options[:tfile], "r").each {|l| templates << eval(l.chomp)}
else
  print "Generating templates...\n"
  (0...100).each {|i|
    s = ""
    (0...i).each {s << rand(256)}
    templates << s
  }
end

print "Connecting to cluster on port: #{options[:port]}+12346...\n"
$sock = TCPSocket.open('localhost', (options[:port].to_i)+12346)
print "Connection established.\n"

def bsend s
  print "sending:  #{s.inspect}\n"
  packet = [s.length].pack('L<') + s
  $sock.send(packet, 0)
  res = len = $sock.recv(4).unpack('L<')[0]
  res = $sock.recv(len) if len
  abort "crashed with: #{packet.inspect}\n" if not res
end

def mangle_one_byte s
  return s if s == ""
  s2 = s.dup
  byte_to_mangle = rand(s2.length)
  s2[byte_to_mangle] ^= rand(256)
  return s2
end

def mangle_many_bytes(s,prob)
  s2 = ""
  s.chars.each{|c| s2 << (c[0] ^ ((rand() < prob) ? rand(256) : 0))}
  return s2
end

$i=0
while true
  templates.each { |s|
    print "template: #{s.inspect} (##{$i+=1})\n"
    bsend(s)
    bsend(mangle_one_byte(s))
    bsend(mangle_many_bytes(s, 0.3))
  }
end
