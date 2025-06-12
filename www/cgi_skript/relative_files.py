#!/usr/bin/env python3

import os

print("Content-Type: text/html")
print()

# Pfad des aktuellen Skripts
script_dir = os.path.dirname(os.path.abspath(__file__))

# Alle Einträge im aktuellen Verzeichnis auflisten
entries = os.listdir(script_dir)

# Nur Dateien (keine Verzeichnisse) herausfiltern
files = [f for f in entries if os.path.isfile(os.path.join(script_dir, f)) and f != os.path.basename(__file__)]

# HTML-Ausgabe
print("<html><body>")
if files:
    print(f"<p>Es wurden {len(files)} Datei(en) gefunden.</p>")
    print("<ul>")
    for file in files:
        print(f"<li>{file}</li>")
    print("</ul>")
else:
    print("<p>Keine Dateien im aktuellen Verzeichnis gefunden.</p>")
print("</body></html>")
