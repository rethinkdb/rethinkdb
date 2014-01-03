<?php

require_once('rdb/rdb.php');
require_once('helpers/setupTable.php');
require_once('helpers/values.php');
require_once('helpers/insert.php');
require_once('helpers/setNumReplicas.php');
require_once('helpers/load.php');


// Arguments:
// - value size 's', 'l' or 'm' - small, large or mixed
// - cache size  in MB
// - host to connect to
// - port offset

if ($argc != 5) {
   die("Wrong number of arguments.\n");
}

echo "Connecting: ";
$conn = r\connect($argv[3], 28015 + $argv[4]);
echo "done\n";

echo "Setting up table: ";
$table = setupTable($conn, $argv[2]);
echo "done\n";

echo "Inserting values: ";
$t = microtime(true);
if ($argv[1] == 's') {
    insert($argv[3], 28015 + $argv[4], $table, $smallValue, $numSmallValues, 5000);
} else if ($argv[1] == 'l') {
    insert($argv[3], 28015 + $argv[4], $table, $largeValue, $numLargeValues, 200);
} else if ($argv[1] == 'm') {
    insert($argv[3], 28015 + $argv[4], $table, $smallValue, round($numSmallValues / 2), 5000);
    $numValues = 16 * 1024;
    insert($argv[3], 28015 + $argv[4], $table, $largeValue, round ($numLargeValues / 2), 200);
} else {
    die("Invalid value size argument.\n");
}
$t = microtime(true) - $t;
echo "done\n";
echo "  Inserting values took " . round($t) . " s\n";

echo "Backfilling (idle): ";
$t = microtime(true);
setNumReplicas($conn, $table, "backBench", $argv[3], 29015 + $argv[4], 2);
$t = microtime(true) - $t;
setNumReplicas($conn, $table, "backBench", $argv[3], 29015 + $argv[4], 1);
echo "done\n";
echo "  Backfilling (idle) took " . round($t) . " s\n";

// TODO: This is currently somewhat broken. Because we up the ack requirements in setNumReplicas,
// the queries fail while replicating.
/*echo "Backfilling (read load): ";
$pids = beginLoad($argv[3], 28015 + $argv[4], $table->sample(10));
$t = microtime(true);
setNumReplicas($conn, $table, "backBench", $argv[3], 29015 + $argv[4], 2);
$t = microtime(true) - $t;
killLoad($pids);
setNumReplicas($conn, $table, "backBench", $argv[3], 29015 + $argv[4], 1);
echo "done\n";
echo "  Backfilling (read load) took " . round($t) . " s\n";*/

?>
