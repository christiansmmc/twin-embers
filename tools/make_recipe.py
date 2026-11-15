"""Write an edit recipe from an edited copy of hash-pinned external files.

The recipe keeps unchanged lines as local line references and carries only the
changed lines as port additions, so no upstream context enters the repository.
It is checked by applying it with tools/apply_edits.py before it is written.

    python tools/make_recipe.py --before build/upstream --after build/work \
        --out patches/pokefirered/0237-name.json src/file.c [more paths]

--before is a bootstrapped tree (every committed recipe applied); --after holds
the edited copies at the same relative paths.
"""
import argparse
import difflib
import hashlib
import json
import shutil
import tempfile
from pathlib import Path

from apply_edits import apply

REFERENCE = '96f95e015af9f5f38e4a8aefca7cab29a78b030d'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def instructions(before, after):
    old = before.decode('utf-8').splitlines(keepends=True)
    new = after.decode('utf-8').splitlines(keepends=True)
    rows = []
    for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, old, new, autojunk=False).get_opcodes():
        if op == 'equal':
            rows.append({'line': i1} if i2 - i1 == 1 else {'span': [i1, i2 - i1]})
            continue
        for text in new[j1:j2]:
            if not text.endswith('\n') or '\r' in text:
                raise ValueError('Port additions must be LF-terminated lines')
            rows.append({'port': text})
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--before', type=Path, required=True)
    ap.add_argument('--after', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('paths', nargs='+')
    args = ap.parse_args()
    if args.out.exists():
        raise ValueError(f'Recipe already exists: {args.out}')
    files = []
    for name in args.paths:
        before = (args.before / name).read_bytes()
        after = (args.after / name).read_bytes()
        if before == after:
            raise ValueError(f'No change in {name}')
        files.append({'path': name, 'before_sha256': sha(before), 'after_sha256': sha(after),
                      'remove': False, 'lines': instructions(before, after)})
    recipe = {'schema': 1, 'technical_reference': REFERENCE, 'files': files}
    with tempfile.TemporaryDirectory() as tmp:
        tree = Path(tmp)
        for name in args.paths:
            (tree / name).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(args.before / name, tree / name)
        probe = tree / 'recipe.json'
        probe.write_text(json.dumps(recipe, separators=(',', ':')) + '\n', encoding='utf-8', newline='\n')
        apply(tree, [probe])
        for name in args.paths:
            if (tree / name).read_bytes() != (args.after / name).read_bytes():
                raise ValueError(f'Recipe does not reproduce {name}')
    args.out.write_text(json.dumps(recipe, separators=(',', ':')) + '\n', encoding='utf-8', newline='\n')
    ports = sum(1 for f in files for row in f['lines'] if 'port' in row)
    print(f'Wrote {args.out} ({len(files)} file(s), {ports} port line(s))')


if __name__ == '__main__':
    main()
