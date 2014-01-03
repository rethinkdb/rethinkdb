<?php

function setupTable($conn, $tableName, $cacheSizeMb)
{
    $createQuery = r\tableCreate($tableName, array('cache_size' => $cacheSizeMb * 1024 * 1024));
    try {
        $result = $createQuery->run($conn)->toNative();
        if ($result['created'] != 1) {
            echo "Table creation failed\n";
        }
    } catch (r\RqlUserError $e) {
        // The table probably exists. delete and try again:
        r\tableDrop($tableName)->run($conn);
        $result = $createQuery->run($conn)->toNative();
        if ($result['created'] != 1) {
            echo "Table creation failed\n";
        }
    }

    return r\table($tableName);
}

?>
