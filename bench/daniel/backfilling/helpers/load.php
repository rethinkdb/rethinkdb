<?php

function beginLoad($host, $port, $query)
{
    $numClients = 32;
    $pids = array();
    for ($i = 0; $i < $numClients; ++$i) {
        $pid = pcntl_fork();
        if ($pid != 0) {
            $pids[] = $pid;
            continue;
        }

        $conn = r\connect($host, $port);
        while (true) {
            $result = $query->run($conn);
        }
        die();
    }
    sleep(2);
    return $pids;
}

function killLoad($pids) {
    foreach($pids as $pid) {
        posix_kill($pid, SIGTERM);
        pcntl_waitpid($pid, $status);
    }
}

?>
