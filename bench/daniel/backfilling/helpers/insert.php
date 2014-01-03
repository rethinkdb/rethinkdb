<?php

function insert($host, $port, $table, $value, $num, $batchSize)
{
    $numClients = 32;
    for ($i = 0; $i < $numClients; ++$i) {
        $numThisClient = round($num / $numClients);

        $pid = pcntl_fork();
        if ($pid != 0) continue;

        $conn = r\connect($host, $port);
        $batch = array_fill(0, $batchSize, $value);
        $insertBatchQuery = $table->insert($batch)->pluck(array('inserted', 'last_error'));
        $numRemaining = $numThisClient;
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
         die();
     }

     for ($i = 0; $i < $numClients; ++$i) {
         pcntl_wait($status);
     }
}

?>
