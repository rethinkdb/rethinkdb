require 'eventmachine'
require 'pp'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

$gen = Random.new
puts "Random seed: #{$gen.seed}"

puts r.table_create('test').run rescue nil
$tbl = r.table('test')

# Use a small number of documents, so that a few hash shards will be
# initially empty.
$ndocs = 5;
$ntries = 5;

for try in 1..$ntries
    $docs = (0...$ndocs).map{|i| {id: i, ts: 0}}
    puts r.table('test').delete.run(durability: 'soft')
    puts r.table('test').insert($docs).run(durability: 'soft')

    puts "\nRUNNING:"

    $log = []
    $stop = false
    $state = {}
    class Handler < RethinkDB::Handler
      def on_open
        EM.defer {
          while !$stop
            id = $gen.rand($ndocs)
            res = $tbl.get(id).delete.run
            $wlog << [id, res]
            res = $tbl.insert({ts: 1}).run
            $wlog << [id, res]

            id = $gen.rand($ndocs)
            res = $tbl.get(id).update({ts: 0.5}).run
            $wlog << [id, res]
            id = $gen.rand($ndocs)
            res = $tbl.get(id).update({ts: 3}).run
            $wlog << [id, res]
          end
          # Wait an extra second to make sure that we've received any pending
          # changes.
          EM.add_timer(1) {
            $h.stop
            EM.stop
            state_size = $state.size
            actual_size = $tbl.count.run
            if state_size != actual_size
              raise RuntimeError, "Inconsistent state at end. state_size=#{state_size}, actual_size=#{actual_size}."
            end
          }
        }
      end
      def on_val(v)
        PP.pp v
        $log << v
        if v.key?('new_val') && v['new_val'] == nil && !v.key?('old_val')
          stop
          raise RuntimeError, "Bad val #{v}."
        elsif v.key?('old_val') && v['old_val'] == nil && !v.key?('new_val')
          stop
          raise RuntimeError, "Bad val #{v}."
        end
        $state.delete(v['old_val']['id']) if v['old_val']
        $state[v['new_val']['id']] = v['new_val'] if v['new_val']
      end
      def on_state(s)
        if s == 'ready'
          $stop = true
        end
      end
    end

    $wlog = []
    EM.run {
      $h = Handler.new
      $opts = {include_initial: true, include_states: true, max_batch_rows: 1}
      $tbl.changes.em_run($h, $opts)
    }
end
