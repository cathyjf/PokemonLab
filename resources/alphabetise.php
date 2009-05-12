<?php

$data = file_get_contents('moves2.xml');
$data = str_replace("\n", '$', $data);

preg_match_all('/<move name="(.*?)" id=".*?">(.*?<\/move>)/', $data, $matches, PREG_SET_ORDER);

$moves = array();

foreach ($matches as $i) {
	$name = $i[1];
	$match = $i[2];
	$moves[$name] = str_replace('$', "\n", $match);
}

ksort($moves);
$id = 0;

foreach ($moves as $i => $j) {
	echo '<move name="' . $i . '" id="' . $id . '">';
	++$id;
	echo $j;
	echo "\n\n";
}

?>
