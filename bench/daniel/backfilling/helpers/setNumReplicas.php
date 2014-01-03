<?php

function setNumReplicas($conn, $table, $tableName, $host, $port, $numReplicas)
{
    $setCmd = "echo 'set acks $tableName 1' | ./rethinkdb admin -j $host:$port 2>&1 | grep -v 'info'";
    system($setCmd);
    $setCmd = "echo 'set replicas $tableName $numReplicas' | ./rethinkdb admin -j $host:$port 2>&1 | grep -v 'info'";
    system($setCmd);
    $setCmd = "echo 'set acks $tableName $numReplicas' | ./rethinkdb admin -j $host:$port 2>&1 | grep -v 'info'";
    system($setCmd);

    // Now wait until backfilling has completed. Because we have also set the acks to $numReplicas, we can do this
    // by waiting until writes can be accepted.
    while (true) {
        try {
            $result = $table->insert(array('id' => 'dummy'))->run($conn);
            $table->get('dummy')->delete()->run($conn);
            break;
        } catch (r\RqlUserError $e) {
        }
        sleep(1);
    }
}

?>
