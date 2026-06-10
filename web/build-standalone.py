#!/usr/bin/env python3
# Rakit semua aset web jadi SATU file HTML mandiri (offline, tanpa internet).
# Output: smarthome-standalone.html  — tinggal dibagikan & dibuka (dobel-klik).
import pathlib

d = pathlib.Path(__file__).parent
html   = (d / "index.html").read_text(encoding="utf-8")
style  = (d / "style.css").read_text(encoding="utf-8")
config = (d / "config.js").read_text(encoding="utf-8")
app    = (d / "app.js").read_text(encoding="utf-8")
mqttjs = (d / "mqtt.min.js").read_text(encoding="utf-8")

# Buang font Google (butuh internet) — CSS sudah fallback ke system-ui.
html = html.replace(
    '  <link rel="preconnect" href="https://fonts.googleapis.com" />\n', "")
html = html.replace(
    '  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet" />\n', "")

# Inline CSS
html = html.replace(
    '<link rel="stylesheet" href="style.css" />',
    "<style>\n" + style + "\n</style>")

# Inline library mqtt.js (lokal, sudah di-vendor)
html = html.replace(
    '<script src="mqtt.min.js"></script>',
    "<script>\n" + mqttjs + "\n</script>")

# Inline config + app
html = html.replace(
    '<script src="config.js"></script>',
    "<script>\n" + config + "\n</script>")
html = html.replace(
    '<script src="app.js"></script>',
    "<script>\n" + app + "\n</script>")

out = d / "smarthome-standalone.html"
out.write_text(html, encoding="utf-8")
print("OK ->", out, f"({out.stat().st_size // 1024} KB)")
