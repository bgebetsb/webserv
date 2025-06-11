#!/usr/bin/php-cgi
<?php

// CGI-Header ausgeben
header("Content-Type: text/plain");

// Body-Inhalt einlesen
$body = file_get_contents("php://input");

// Ergebnis ausgeben
if (strlen($body) > 0) {
    echo "Body wurde empfangen.\n\n";
    echo "Inhalt:\n";
    echo $body;
} else {
    echo "Kein Body empfangen.";
}
?>
