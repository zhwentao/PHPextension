<?php
function foo() {
    $i = 1;
//    echo $i;
    //throw new Exception();
}
$start = microtime(true);
$count = 100000;
while($count--) {
    str_replace("a", "b", "abc");
    foo();
}
$spend = microtime(true) - $start;

echo "\r\nTime: $spend\r\n";
