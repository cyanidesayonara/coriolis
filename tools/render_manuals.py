#!/usr/bin/env python3
"""
Render the Coriolis manuals (docs/manual/*.html) to PDF with a headless
Chromium browser (Microsoft Edge or Google Chrome), which gives proper
typography, CSS layout and crisp embedded screenshots.

  python tools/render_manuals.py

Writes docs/Coriolis_User_Manual.pdf and docs/Coriolis_Build_Guide.pdf.
Regenerate the screenshots first if scenes changed:
  build\\Release\\coriolis_sim.exe --shots docs/screenshots
"""

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MANUALS = [
    ("docs/manual/user_manual.html", "docs/Coriolis_User_Manual.pdf"),
    ("docs/manual/build_guide.html", "docs/Coriolis_Build_Guide.pdf"),
]

CANDIDATES = [
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    "/usr/bin/google-chrome",
    "/usr/bin/chromium",
]


def find_browser():
    for c in CANDIDATES:
        if os.path.exists(c):
            return c
    for name in ("msedge", "chrome", "google-chrome", "chromium"):
        p = shutil.which(name)
        if p:
            return p
    sys.exit("No Edge/Chrome found; install one or add it to CANDIDATES.")


def render(browser, html, pdf):
    src = (ROOT / html).resolve()
    out = (ROOT / pdf).resolve()
    with tempfile.TemporaryDirectory() as profile:
        subprocess.run(
            [browser, "--headless=new", "--disable-gpu", "--no-first-run",
             f"--user-data-dir={profile}", "--no-pdf-header-footer",
             "--virtual-time-budget=4000",
             f"--print-to-pdf={out}", src.as_uri()],
            check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(f"wrote {pdf} ({out.stat().st_size // 1024} KB)")


def main():
    browser = find_browser()
    for html, pdf in MANUALS:
        if (ROOT / html).exists():
            render(browser, html, pdf)


if __name__ == "__main__":
    main()
