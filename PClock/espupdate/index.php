<?PHP
header('Content-type: text/plain; charset=utf8', true);

$db = array(
    "" => $filenm // Put your ESP's MAC Address in between the ""
);

$headers = getallheaders();
$headers = array_change_key_case($headers, CASE_LOWER);

function sendFile($path) {
    header($_SERVER["SERVER_PROTOCOL"].' 200 OK', true, 200);
    header('Content-Type: application/octet-stream', true);
    header('Content-Disposition: attachment; filename='.basename($path));
    header('Content-Length: '.filesize($path), true);
    header('x-MD5: '.md5_file($path), true);
    readfile($path);
}

if(!isset($headers['user-agent']) OR $headers['user-agent'] != "ESP8266-http-Update") {
    header($_SERVER["SERVER_PROTOCOL"].' 403 Forbidden', true, 403);
    echo "Only for ESP8266 updater!\n";
    exit();
}

$compartment = "pclock-";

$files = glob("./bin/$compartment*.bin");

if (count($files) > 0)
foreach ($files as $file)
 {
    $info = pathinfo($file);
	$filenm =  $info["basename"];
 }

if (isset($db[$headers['x-esp8266-sta-mac']]))
{

$str = $db[$headers['x-esp8266-sta-mac']];
$arr=explode("-",$str);
$float = (float)$arr[1];
}

if(isset($db[$headers['x-esp8266-sta-mac']])) {
    if($db[$headers['x-esp8266-sta-mac']] AND $float > $headers['x-esp8266-version']) {
        sendFile("./bin/".$db[$headers['x-esp8266-sta-mac']]);
    } else {
        header($_SERVER["SERVER_PROTOCOL"].' 304 Not Modified', true, 304);
    }
    exit();
}

header($_SERVER["SERVER_PROTOCOL"].' 500 no version for ESP MAC', true, 500);
?>