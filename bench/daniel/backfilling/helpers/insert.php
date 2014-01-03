<?php

function insert($conn, $table, $value, $num)
{
    $batchSize = 100;
    $batch = array_fill(0, $batchSize, $value);
    $insertBatchQuery = $table->insert($batch)->pluck(array('inserted', 'last_error'));
    $numRemaining = $num;
    while ($numRemaining > 0) {
        if ($batchSize > $numRemaining) {
            $batch = array_fill(0, $numRemaining, $value);
            $insertBatchQuery = $table->insert($batch)->pluck(array('inserted', 'last_error'));
        }
        $result = $insertBatchQuery->run($conn, array('durability' => 'soft'))->toNative();
        if ($result['inserted'] != count($batch)) {
            echo "Error while inserting: " . @$result['last_error'] . "\n";
        }
        $numRemaining -= $result['inserted'];
    }
}

?>
