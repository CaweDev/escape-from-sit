#!/usr/bin/env python3
"""
add_doxygen_headers.py
Utility to prepend DigiPen-style Doxygen file headers to .cpp and .hpp files
under AlphaEngine2.5D/Source/Game. It is idempotent and will skip files that
already contain a "\file" tag in the first 8 lines.

Run from repository root:
    python tools/add_doxygen_headers.py

This script only modifies files when necessary.
"""
from pathlib import Path
import os

ROOT = Path(__file__).resolve().parents[1]
# Process both Game and Engine source trees
SRC_DIRS = [
    
    ROOT / 'AlphaEngine2.5D' / 'Source' / 'Engine'
]

HEADER_TMPL = '''/* Start Header ************************************************************************/\n/*!\n\\file\t\t{filename}\n\\author \t\n\\par\t\n\\date\tMarch, 29, 2026\n\\brief\t\t[AUTO] {brief}\n\nCopyright (C) 2026 DigiPen Institute of Technology.\nReproduction or disclosure of this file or its contents\nwithout the prior written consent of DigiPen Institute of\nTechnology is prohibited.\n*/\n/* End Header **************************************************************************/\n\n'''


def has_file_tag(text: str) -> bool:
    head = '\n'.join(text.splitlines()[:8])
    return '\\file' in head or '@file' in head


def process_file(p: Path) -> bool:
    try:
        txt = p.read_text(encoding='utf-8')
    except Exception:
        return False

    changed = False

    # Add file-level header if missing
    if not has_file_tag(txt):
        # Use filename and stem to populate the DigiPen header template
        filename = p.name
        brief = p.stem
        header = HEADER_TMPL.format(filename=filename, brief=brief)
        txt = header + txt
        changed = True

    # Additionally, add simple Doxygen function headers for class-methods
    # Only target definitions where the signature and opening brace are on the same line
    # and contain a '::' (to reduce false positives). This is conservative.
    import re

    lines = txt.splitlines()
    i = 0
    # regex for class method definition on single line: returntype N::F(params) [const] {
    method_re = re.compile(r"^(\s*)([^\n{};]+?)\s+([A-Za-z_][A-Za-z0-9_:<>]*)::([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)\s*(const)?\s*\{")
    while i < len(lines):
        m = method_re.match(lines[i])
        if m:
            # check previous 3 lines for existing doc
            prev_block = '\n'.join(lines[max(0, i-3):i])
            if ('/*!' in prev_block) or ('///' in prev_block) or ('@brief' in prev_block) or ('\\file' in prev_block):
                i += 1
                continue

            indent = m.group(1)
            return_type = m.group(2).strip()
            cls = m.group(3)
            func = m.group(4)
            params = m.group(5).strip()
            is_const = bool(m.group(6))

            # Build Doxygen block
            header_lines = []
            header_lines.append(indent + '/*!')
            header_lines.append(indent + ' * \\brief [AUTO] ' + func)

            if params:
                # parse parameters by comma
                parts = [p.strip() for p in params.split(',') if p.strip()]
                for part in parts:
                    # parameter name is last token (may include default values - ignore)
                    tokens = part.split()
                    name = tokens[-1] if tokens else ''
                    # strip default initializer if present
                    if '=' in name:
                        name = name.split('=')[0].strip()
                    # remove pointer/reference chars from name
                    name = name.strip('&*')
                    if name:
                        header_lines.append(indent + ' * \\param ' + name + ' [in]')

            # Add return if not void
            if return_type and 'void' not in return_type:
                header_lines.append(indent + ' * \\return ')

            header_lines.append(indent + ' */')

            # insert header above current line
            lines[i:i] = header_lines
            i += len(header_lines)  # skip over inserted lines
            changed = True
        i += 1

    if changed:
        newtxt = '\n'.join(lines) + '\n'
        p.write_text(newtxt, encoding='utf-8')
        return True
    return False


def main():
    exts = ('.cpp', '.hpp', '.h')
    modified = []
    for SRC_DIR in SRC_DIRS:
        if not SRC_DIR.exists():
            print('Source directory not found:', SRC_DIR)
            continue

        for dirpath, dirnames, filenames in os.walk(SRC_DIR):
            for fn in filenames:
                if fn.lower().endswith(exts):
                    p = Path(dirpath) / fn
                    if process_file(p):
                        modified.append(p)

    print('Files modified:', len(modified))
    for m in modified:
        print(' -', m.relative_to(ROOT))


if __name__ == '__main__':
    main()
