#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");

// Setze ein Cookie, wenn es noch nicht existiert
if (!isset($_COOKIE['test_cookie'])) {
    setcookie("test_cookie", "cookie_value", time() + 3600, "/");
    echo "<p>Cookie wurde gesetzt. Bitte lade die Seite neu.</p>";
} else {
    echo "<p>Cookie empfangen: " . htmlspecialchars($_COOKIE['test_cookie']) . "</p>";
}
?>
