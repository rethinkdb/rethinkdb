require 'eventmachine'
require 'pp'
require_relative '../importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

$gen = Random.new
puts "Random seed: #{$gen.seed}"

dbName = 'test'
tableName = File.basename(__FILE__).gsub('.', '_')

puts "Creating table #{dbName}.#{tableName}"
r.expr([dbName]).set_difference(r.db_list()) \
 .for_each{|row| r.db_create(row)}.run
r.expr([tableName]).set_difference(r.db(dbName).table_list()) \
 .for_each{|row| r.db(dbName).table_create(row)}.run
$tbl = r.db(dbName).table(tableName)

$tbl.index_create('ts').run rescue nil
$tbl.index_wait.run

$ndocs = 20;
$ntries = 3;

$queries = [
  $tbl,
  $tbl.order_by(index: :id).limit(5),
  $tbl.order_by(index: :id),
  $tbl.map{|x| x.merge({'foo' => 'bar'})},
  $tbl.filter{|x| false},
  $tbl.filter{|x| r.branch(x[:id].type_of.eq("NUMBER"), x[:id].mod(2).eq(1), true)},
  $tbl.pluck(:id),
  $tbl.get_all(1),
  $tbl.get_all(1, 2, 3, 4, 5, 6, 7, 8, 9, 10),
  $tbl.get_all(0, index: :ts),
  $tbl.between(5, 15),
  $tbl.between(1, 10, index: :ts),
  $tbl.between(0, 2, index: :ts)]

for nshards in [1, 2, 5]
  $tbl.reconfigure(replicas: 1, shards: nshards).run
  $tbl.wait.run
  for try in 1..$ntries
    $docs = (0...$ndocs).map{|i| {id: i, ts: 0}}
    puts $tbl.delete.run(durability: 'soft')
    puts $tbl.insert($docs).run(durability: 'soft')
    $tbl.rebalance.run
    $tbl.wait.run
    

    puts "\nRUNNING:"

    $num_running = $queries.size
    class Handler < RethinkDB::Handler
      def initialize(query)
        @query = query
        @log = []
        @state = {}
        @start_time = Time.now
      end
      def query
        @query
      end
      def state
        @state
      end
      def log
        @log
      end
      def on_val(v)
        @log << v
        if v.key?('new_val') && v['new_val'] == nil && !v.key?('old_val')
          stop
          raise RuntimeError, "Bad val #{v}."
        elsif v.key?('old_val') && v['old_val'] == nil && !v.key?('new_val')
          stop
          raise RuntimeError, "Bad val #{v}."
        end
        @state.delete(v['old_val']['id']) if v['old_val']
        @state[v['new_val']['id']] = v['new_val'] if v['new_val']

        # Add a delay to ensure that enough writes get through before we reach the
        # 'ready' state. This is just to increase the coverage of this test.
        # We're targeting a time of 4 seconds per run, after which we read out the
        # remaining changes as quickly as possible.
        elapsed_time = Time.now.to_f - @start_time.to_f
        if elapsed_time > 30
          stop
          raise RuntimeError, "Reached time limit"
        elsif elapsed_time < 4
          sleep(0.2)
        end
      end
      def on_state(s)
        @log << s
        if s == 'ready'
          $num_running = $num_running - 1
        end
      end
    end

    $wlog = []
    EM.run {
      $opts = {include_initial: true, include_states: true, max_batch_rows: 1}
      $handlers = []
      for q in $queries
        $handlers.push(Handler.new(q))
        q.changes.em_run($handlers.last, $opts)
      end
      EM.defer {
        start_time = Time.now
        while $num_running != 0
          elapsed_time = Time.now.to_f - start_time.to_f
          execution_timeout = 60
          if elapsed_time > execution_timeout
            for h in $handlers
              h.stop
            end
            EM.stop
            raise RuntimeError, "Reached time limit of #{execution_timeout}s in write loop"
          end

          id = $gen.rand($ndocs)
          res = $tbl.get(id).delete(:return_changes => true).run
          $wlog << [id, res]
          res = $tbl.insert({ts: 1}, :return_changes => true).run
          $wlog << [id, res]

          id = $gen.rand($ndocs)
          res = $tbl.get(id).update({ts: 0.5}, :return_changes => true).run
          $wlog << [id, res]
          id = $gen.rand($ndocs)
          res = $tbl.get(id).update({ts: 3}, :return_changes => true).run
          $wlog << [id, res]

          sleep(0.05)
        end
        # Wait an extra 2 seconds to make sure that any pending changes have
        # been received.
        EM.add_timer(2) {
          for h in $handlers
            h.stop
          end
          EM.stop

          # Compare the state of each handler
          for h in $handlers
            actual = h.query.coerce_to(:array).run
            actual = actual.sort_by{|x| "#{x["id"]}"}
            state = h.state.values.sort_by{|x| "#{x["id"]}"}
            if "#{actual}" != "#{state}"
              print "Failed changefeed log:\n"
              PP.pp h.log
              print "\nFailed write log:\n"
              PP.pp $wlog
              raise RuntimeError, "State did not match query result.\n Query: #{h.query.pp}\n State: #{state}\n Actual: #{actual}"
            end
          end
        }
      }
    }
  end
end
