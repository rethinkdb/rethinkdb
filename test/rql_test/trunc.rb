require 'pp'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

# We do it this way because one of the bugs we're testing for is where
# the `N` prefix to the primary key accidentally gets compared to the
# content of the field.
def a_field(i)
  'A'*300 + sprintf('%04d', 1000 - i);
end
def z_field(i)
  'Z'*300 + sprintf('%04d', 1000 - i);
end
def num(field)
  field.match(/.*?(\d+)/)[1]
end
def doc(i)
  {'id' => 'a'*i, 'A' => a_field(i), 'Z' => z_field(i)}
end

$batchopts = [{max_batch_rows: 1}, {max_batch_rows: 5}, {}]
$batchopts.each {|bo|
  puts "Testing with batch options: #{bo}"
  r.table_create('test').run rescue nil
  r.table('test').wait.run
  r.table('test').index_create('A').run rescue nil
  r.table('test').index_wait('A').run
  r.table('test').index_create('Z').run rescue nil
  r.table('test').index_wait('Z').run
  r.table('test').delete.run
  (0...100).each{|i|
    puts "testing #{i}"
    di = doc(i)
    r.table('test').insert(di).run(bo)

    res1 = r.table('test').min(index: 'id').run(bo)
    res2 = r.table('test').orderby(index: 'id').run(bo).next
    res3 = r.table('test').orderby(index: 'id').limit(1).run(bo).next
    res4 = r.table('test').orderby(index: 'id')[0].run(bo)
    if [res1, res2, res3, res4].map{|x| x['id']} != ['']*4
      raise "ID MIN ERROR (i=#{i}, batch_options: #{bo})"
    end

    res1 = r.table('test').max(index: 'id').run(bo)
    res2 = r.table('test').orderby(index: r.desc('id')).run(bo).next
    res3 = r.table('test').orderby(index: r.desc('id')).limit(1).run(bo).next
    res4 = r.table('test').orderby(index: r.desc('id'))[0].run(bo)
    if [res1, res2, res3, res4].map{|x| x['id']} != [di['id']]*4
      raise "ID MAX ERROR (i=#{i}, batch_options: #{bo})"
    end

    ['A', 'Z'].each{|f|
      res1 = r.table('test').min(index: f).run(bo)
      res2 = r.table('test').orderby(index: f).run(bo).next
      res3 = r.table('test').orderby(index: f).limit(1).run(bo).next
      res4 = r.table('test').orderby(index: f)[0].run(bo)
      if [res1, res2, res3, res4].map{|x| num(x[f])} != [num(di[f])]*4
        raise "#{f} MIN ERROR (i=#{i}, batch_options: #{bo})"
      end

      res1 = r.table('test').max(index: f).run(bo)
      res2 = r.table('test').orderby(index: r.desc(f)).run(bo).next
      res3 = r.table('test').orderby(index: r.desc(f)).limit(1).run(bo).next
      res4 = r.table('test').orderby(index: r.desc(f))[0].run(bo)
      if [res1, res2, res3, res4].map{|x| num(x[f])} != ['1000']*4
        PP.pp [[res1, res2, res3, res4].map{|x| num(x[f])}, ['1000']*4]
        raise "#{f} MAX ERROR (i=#{i}, batch_options: #{bo})"
      end
    }
  }
}
