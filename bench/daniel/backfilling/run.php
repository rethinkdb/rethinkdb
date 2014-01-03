<?php

require_once('rdb/rdb.php');
require_once('helpers/setupTable.php');
require_once('helpers/values.php');
require_once('helpers/insert.php');
require_once('helpers/setNumReplicas.php');


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
    insert($conn, $table, $smallValue, $numSmallValues);
} else if ($argv[1] == 'l') {
    insert($conn, $table, $largeValue, $numLargeValues);
} else if ($argv[1] == 'm') {
    insert($conn, $table, $smallValue, round($numSmallValues / 2));
    $numValues = 16 * 1024;
    insert($conn, $table, $largeValue, round ($numLargeValues / 2));
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

?>
