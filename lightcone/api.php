<?php
    header('Content-Type: text/plain');

    $filename = "data.txt";

    $exerciseId=$_GET['exerciseId'];
    $sessionId=$_GET['sessionId'];
    $participantId=$_GET['participantId'];
    $step=$_GET['step'];  
    $time=$_GET['time'];  
    $state=$_GET['state'];  

    $data = $exerciseId.",".$sessionId.",".$participantId.",".$step.",".$time.",".$state."\n";
  
    $exists = file_exists($filename);
    $file = fopen($filename, "a") or die("Unable to open file!");    
    if (!$exists) {
        fwrite($file, "exerciseId,sessionId,participantId,step,time,state\n");    
    }
    fwrite($file, $data);
    fclose($file);    
?>
OK