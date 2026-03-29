#!/usr/bin/env python3
"""
check_docs_engine.py
Scan AlphaEngine2.5D/Source/Engine for files missing a file-level DigiPen/Doxygen header.
Prints a summary and lists files missing headers.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'AlphaEngine2.5D' / 'Source' / 'Engine'

if not SRC.exists():
    print("Engine source folder not found:", SRC)
    sys.exit(1)

exts = ('.cpp', '.hpp', '.h')
missing = []
checked = []
for p in sorted(SRC.rglob('*')):
    if p.suffix.lower() in exts and p.is_file():
        checked.append(p)
        try:
            txt = p.read_text(encoding='utf-8')
        except Exception as e:
            print('ERROR reading', p, e)
            continue
        head = '\n'.join(txt.splitlines()[:20])
        # considered documented if it has \file or the DigiPen Start Header marker
        if ('\\file' not in head) and ('/* Start Header' not in head) and ('@file' not in head):
            missing.append(p)

print('Files checked:', len(checked))
print('Files missing file-level header:', len(missing))
for m in missing:
    print(' -', m.relative_to(ROOT))

# Exit with non-zero when missing found
sys.exit(0 if not missing else 2)
