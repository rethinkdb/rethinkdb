require 'eventmachine'
require_relative './importRethinkDB.rb'

$state = []
class H < RethinkDB::Handler
  def initialize
    @opened = @closed = 0
  end
  def on_error(err)
    if err != RethinkDB::ReqlRuntimeError.new("Connection is closed.")
      raise err
    end
  end
  def on_open
    $state << [:o, @opened += 1]
    if @opened == 3
      $state << [3, RethinkDB::EM_Guard.class_eval("@@conns.size")]
      $c3.close
      $state << [2, RethinkDB::EM_Guard.class_eval("@@conns.size")]
      EM.stop
    end
  end
  def on_close
    $state << [:c, @closed += 1]
  end
end

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
$c1 = r.connect(port: $port)
$c2 = r.connect(port: $port)
$c3 = r.connect(port: $port)
r.table_create('test').run($c1) rescue nil
$p = H.new
EM.run {
  $f1 = r.table('test').changes.em_run($c1, $p)
  $f2 = r.table('test').changes.em_run($c2, $p)
  $f3 = r.table('test').changes.em_run($c3, $p)
}
$state << [0, RethinkDB::EM_Guard.class_eval("@@conns.size")]

# Either of these are legal depending on whether `$f3` is in
# `@waiters` or attempting to acquire `@@listener_mutex` inside
# `EM.tick`.
$exp1 = [[:o, 1], [:o, 2], [:o, 3], [3, 3], [:c, 1], [2, 2], [:c, 2], [:c, 3], [0, 0]]
$exp2 = [[:o, 1], [:o, 2], [:o, 3], [3, 3], [2, 2], [:c, 1], [:c, 2], [:c, 3], [0, 0]]

if $state != $exp1 && $state != $exp2
  raise RuntimeError, "Incorrect state: #{$state.inspect}."
end
if [$f1, $f2, $f3].any?{|f| !f.closed?}
  raise RuntimeError, "Not all handles closed: #{f1.inspect f2.inspect f3.inspect}."
end
