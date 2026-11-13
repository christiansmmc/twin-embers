"""Bring docs/review-manifest.json up to date with the working tree before a commit.

Every added, modified or untracked file that Git reports (except the manifest itself) gets its record: an
existing record takes the file's new SHA-256 (of its LF-normalised bytes, the manifest's hash policy) and the
fork's credit once; a new file gets a full record. Deleted files lose their record. The manifest is written in
its own format (indent 1, UTF-8, no trailing newline). Run tools/source_audit.py afterwards.

    python tools/manifest_update.py [--basis TEXT]
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = 'docs/review-manifest.json'
CREDIT = 'christiansmmc (fork changes with disclosed AI assistance)'
LICENSE = 'MIT only for original owned contributions; third-party rights unchanged'
BASIS = 'Reviewed text-only source/documentation of the fork; no ROM/game payloads or generated media'


def changed_paths():
    out = subprocess.check_output(['git', '-C', str(ROOT), 'status', '--porcelain', '-uall', '-z']).decode()
    present, deleted = [], []
    entries = out.split('\0')
    i = 0
    while i < len(entries):
        line = entries[i]
        i += 1
        if not line:
            continue
        status, name = line[:2], line[3:]
        if status[0] == 'R':
            deleted.append(entries[i])  # a rename's second entry is its old name
            i += 1
        if name == MANIFEST:
            continue
        (deleted if 'D' in status else present).append(name)
    return present, deleted


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--basis', default=BASIS, help='basis for new records')
    args = ap.parse_args()
    path = ROOT / MANIFEST
    doc = json.loads(path.read_bytes())
    rows = {row['path']: row for row in doc['files']}
    present, deleted = changed_paths()
    for name in present:
        digest = hashlib.sha256((ROOT / name).read_bytes().replace(b'\r\n', b'\n')).hexdigest()
        row = rows.get(name)
        if row is None:
            row = {'path': name, 'sha256': digest, 'review_status': 'reviewed', 'credit': CREDIT,
                   'license': LICENSE, 'basis': args.basis}
            doc['files'].append(row)
            rows[name] = row
            print('added  ', name)
        else:
            row['sha256'] = digest
            if 'christiansmmc' not in row['credit']:
                row['credit'] += '; ' + CREDIT
            print('updated', name)
    gone = set(deleted)
    if gone:
        doc['files'] = [row for row in doc['files'] if row['path'] not in gone]
        for name in sorted(gone):
            print('removed', name)
    path.write_bytes(json.dumps(doc, indent=1, ensure_ascii=False).encode('utf-8'))


if __name__ == '__main__':
    main()
