#!/usr/bin/ruby
# Copyright 2010-2012 RethinkDB, all rights reserved.
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
  $opt[:log] = nil
  opts.on('-l', '--log LOG', 'Replay LOG') {|l| $opt[:log] = l}
  $opt[:relog] = nil
  opts.on('-r', '--relog', 'Re-log the packets') {$opt[:relog] = true}
  $opt[:verbose] = nil
  opts.on('-v', '--verbose', 'Verbose output') {$opt[:verbose] = true}
}.parse!

raise ArgumentError,"-l required" if not $opt[:log]
$log = $opt[:log]
$relog = $opt[:relog] && $opt[:log]+".relog"

class Slave
  @@STOP = "a8448469-0357-4d89-972c-bb28e4e5b2d1"
  def initialize(id)
    @id = id
    @r,@w = Socket.pair(Socket::AF_UNIX, Socket::SOCK_DGRAM, 0)
    @pid = fork do
      @w.close
      sock = TCPSocket.open($opt[:host], $opt[:port].to_i+28015)
      sock.send([0xaf61ba35].pack('L<'), 0)
      print "*** #{@id} starting ***\n"
      while (packet = @r.recv(2**30, 0)) != @@STOP
        str = [@id, Time.now.to_f, packet].inspect+"\n"
        File.open($relog, 'a') {|f| f.write str} if $relog
        print [@id, Time.now.to_f, packet].inspect+"\n" if $opt[:verbose]
        sock.send(packet, 0)
        res = len = sock.recv(4).unpack('L<')[0]
        res = sock.recv(len) if len
      end
      print "*** #{@id} stopping ***\n"
      @r.close
      sock.close
    end
    @r.close
  end

  def send(packet); @w.send(packet, 0); end
  def stop; @w.send(@@STOP, 0); @w.close; end
end

$slaves = Hash.new {|hash,val| hash[val] = Slave.new val}
File.open($opt[:log]) {|f|
  f.each {|l|
    process, time, packet = eval(l.chomp)
    $slaves[process].send(packet)
  }
}
$slaves.each{|_,slave| slave.stop}
Process.waitall
