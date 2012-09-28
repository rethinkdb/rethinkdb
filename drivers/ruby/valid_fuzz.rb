#!/usr/bin/ruby
$LOAD_PATH.unshift('./rethinkdb')
require 'pp'
require 'rethinkdb.rb'
require 'optparse'
include RethinkDB::Shortcuts

options = {}
OptionParser.new {|opts|
  opts.banner = "Usage: fuzz.rb [options]"

  options[:port] = "0"
  opts.on('-p', '--port PORT', 'Fuzz on PORT+12346 (default 0)') {|p| options[:port] = p}

  options[:seed] = srand()
  opts.on('-s', '--seed SEED', 'Set the random seed to SEED') {|s| options[:seed] = s}

  options[:tfile] = nil
  opts.on('-t', '--templates FILE', 'Read templates from FILE') {|f| options[:tfile] = f}

  options[:index] = 0
  opts.on('-i', '--index INT', 'Start from index INT') {|i| options[:index] = i.to_i}
}.parse!

$templates = []
print "Using seed: #{options[:seed]}\n"
srand(options[:seed].to_i)
if options[:tfile]
  print "Using templates from file: #{options[:tfile]}\n"
  File.open(options[:tfile], "r").each {|l| $templates << eval(l.chomp)}
else
  print "ERROR: must provide templates with `-t`\n"
end

print "Connecting to cluster on port: #{options[:port]}+12346...\n"
c = RethinkDB::Connection.new('localhost', (options[:port].to_i)+12346)
print "Connection established.\n"

def crossover(s1, s2, p=0.3)
  snew = s1.dup
  snew.each_index {|i|
    rhs = s2[i]
    if rand < p and rhs != nil
      if rhs.class != Array || rand < p
        snew[i] = rhs
      else
        lhs = snew[i].class == Array ? snew[i] : [snew[i]]
        snew[i] = crossover(lhs, rhs, p)
      end
    end
  }
end

$i=0
while true
  $templates.each { |t|
    s = t
    $f = 0
    while s == t && $f < 20
      s = crossover(t, $templates[rand($templates.length)])
      begin
        q = (RethinkDB::RQL_Query.new s).query
      rescue Exception
        s = t
      end
      $f += 1
    end
    if $f < 20
      if $i >= options[:index]
        print "##{$i}: sexp: #{t.inspect}\n"
        print "##{$i}: sexp: #{s.inspect}\n"
        begin
          PP.pp((RethinkDB::RQL_Query.new s).run.to_a)
        rescue Exception => e
          print "Failed with: #{e}\n"
        end
        sleep 1
      end
    else
      print "##{$i}: SKIPPING\n"
    end
    $i += 1
  }
end
