<?php
$row = 0;
if (($handle = fopen("data.txt", "r")) !== FALSE) {
    echo "<table>";
    while (($data = fgetcsv($handle, 1000, ",")) !== FALSE) {
        $num = count($data);
        $tag = ($row) ? "td" : "th";
        $row++;
        echo "<tr>";
        for ($c=0; $c < $num; $c++) {
            echo "<$tag>" . $data[$c] . "</$tag>";
        }
        echo "</tr>\n";
    }
    echo "</table>";
    fclose($handle);
}
?>