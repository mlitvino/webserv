#!/usr/bin/php-cgi

<?php
header("Content-Type: text/html");

$body = <<<'HTML'
<html>
<head><title>Hello CGI (PHP)</title></head>
<body>
<h2>Hello from PHP CGI! (hello.php)</h2>
</body>
</html>
HTML;

echo "\r\n";
echo $body;
?>
