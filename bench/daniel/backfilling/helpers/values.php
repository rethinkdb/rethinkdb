<?php

$smallValue = array('a' => 'foo', 'b' => 0.5);
$largeValue = array('a' => str_repeat('A', 16*1024), 'b' => 0.5);

$goalSize = 512 * 1024 * 1024;
$perValueOverhead = 128;
$smallValueSize = 16;
$numSmallValues = round($goalSize / ($perValueOverhead + $smallValueSize));
$largeValueSize = 16 * 1024 + 12;
$numLargeValues = round($goalSize / ($perValueOverhead + $largeValueSize));

?>
