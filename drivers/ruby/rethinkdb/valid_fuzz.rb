#!/usr/bin/ruby
require 'pp'
require 'rethinkdb_shortcuts.rb'
require 'optparse'
include RethinkDB::Shortcuts_Mixin

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
  snew.each_index{|i| snew[i] = s2[i] if (rand < p and s2[i])}
end

$i=0
while true
  $templates.each { |t|
    #s = t
    s = crossover(t, $templates[rand($templates.length)])
    if $i >= options[:index]
      print "##{$i+=1}: sexp: #{s.inspect}\n"
      begin
        PP.pp((RethinkDB::S._ s).run)
      rescue Exception => e
        print "Failed with: #{e.class}\n"
      end
      sleep 1
    else
      $i += 1
    end
  }
end
