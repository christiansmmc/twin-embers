"""Day-to-day loop for this fork on Windows: build, data pack, PC emulator, 3DS over Wi-Fi.

    python tools/dev.py build                  incremental build into build/dev
    python tools/dev.py pak [--rom FILE] [-- BUILDER OPTIONS]
                                               the data pack, made by this fork's Builder from your own ROM
    python tools/dev.py test [SUITE ...]       host test suites: sync build/host-tests, verify-host, python suites (or just SUITE ...)
    python tools/dev.py render [VIEW ...]      the B views as PNGs under build/renders, with your own pack (never commit them)
    python tools/dev.py run                    build, copy to Azahar's SD card and start Azahar
    python tools/dev.py send [--ip IP]         build and run it on the 3DS once (Homebrew Launcher: press Y)
    python tools/dev.py install [--ip IP] [--pak]
                                               build and copy it to the 3DS's SD card through ftpd

build/dev is made once by tools/bootstrap.py; after that only the files whose content changed are written into
it (repository files, and the external files the edit recipes touch), so make rebuilds only what changed. The ROM
path, the Builder options and the console's IP are remembered in .dev.local.json (ignored by Git). Nothing here
uploads a ROM, a pack or a build anywhere but your own emulator folder and console.
"""
import argparse
import ftplib
import hashlib
import json
import os
import re
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from apply_edits import apply  # noqa: E402
from bootstrap import inputs  # noqa: E402

APP = re.search(r'#define CTR_APP_NAME "([^"]+)"',
                (ROOT / '3ds_port/include/3ds_app_name.h').read_text(encoding='utf-8')).group(1)
DEV = ROOT / 'build/dev'
IMAGE = DEV / '3ds_port' / f'{APP}.3dsx'
PAK = ROOT / 'build/pak' / f'{APP}.pak'
CONFIG = ROOT / '.dev.local.json'
DEVKITPRO = Path(os.environ.get('DEVKITPRO_WIN', r'C:\devkitPro'))
AZAHAR = Path(os.environ.get('AZAHAR_EXE', r'C:\Program Files\Azahar\azahar.exe'))
FTP_PORT = 5000  # ftpd's default


def load_config():
    try:
        return json.loads(CONFIG.read_text(encoding='utf-8'))
    except FileNotFoundError:
        return {}


def save_config(config):
    CONFIG.write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')


def msys(path):
    """C:\\Users\\x -> /c/Users/x, for the devkitPro MSYS2 shell."""
    p = Path(path).resolve()
    if not p.drive:  # POSIX: already an MSYS-style path
        return p.as_posix()
    return '/' + p.drive[0].lower() + p.as_posix()[len(p.drive):]


# ---- build -------------------------------------------------------------------------------------------------

def recipe_paths(recipes):
    paths = set()
    for recipe in recipes:
        for row in json.loads(recipe.read_text(encoding='utf-8'))['files']:
            paths.add(row['path'])
    return paths


def pristine(dest, name):
    """The file as the pinned upstream commit has it in dest, or None when upstream has no such file."""
    probe = subprocess.run(['git', '-C', str(dest), 'cat-file', '-e', f'HEAD:{name}'], capture_output=True)
    if probe.returncode != 0:
        return None
    return subprocess.check_output(['git', '-C', str(dest), 'show', f'HEAD:{name}'])


def put(dest, name, raw):
    """Write dest/<name> only when its content changes, so make keeps every other timestamp."""
    target = dest / name
    if raw is None:
        if target.exists():
            target.unlink()
            return 1
        return 0
    if target.exists() and target.read_bytes() == raw:
        return 0
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(raw)
    return 1


def adopted_state(dest, lock, recipes, recipe_hashes):
    """The sync state of a tree that bootstrap.py made (no .dev-sync.json yet), or None when it cannot be reused."""
    try:
        made_from = json.loads((dest / '.twin-bootstrap.json').read_text(encoding='utf-8'))['inputs']
        head = subprocess.run(['git', '-C', str(dest), 'rev-parse', 'HEAD'], capture_output=True, text=True)
    except (OSError, ValueError, KeyError):
        return None
    made_recipes = {k: v for k, v in made_from.items() if k.startswith('patches/')}
    if head.returncode != 0 or head.stdout.strip() != lock['commit'] or made_recipes != recipe_hashes:
        return None
    return {'commit': lock['commit'], 'recipes': recipe_hashes, 'patched': sorted(recipe_paths(recipes)),
            'repo': {k: v for k, v in made_from.items() if not k.startswith('patches/')}}


def label(dest):
    try:
        return dest.relative_to(ROOT).as_posix()
    except ValueError:
        return str(dest)


def sync(dest=DEV):
    """Bring dest (build/dev or build/host-tests) up to date writing only the files whose content changed."""
    state_file = dest / '.dev-sync.json'
    lock = tomllib.loads((ROOT / 'upstream.lock').read_text())['pokefirered']
    current = inputs()
    recipes = sorted((ROOT / 'patches/pokefirered').glob('*.json'))
    recipe_hashes = {k: v for k, v in current.items() if k.startswith('patches/')}
    repo = {k: v for k, v in current.items() if not k.startswith('patches/')}
    state = json.loads(state_file.read_text()) if state_file.exists() else None
    if state is None and dest.exists():
        state = adopted_state(dest, lock, recipes, recipe_hashes)
    if state is None or state.get('commit') != lock['commit']:
        if dest.exists():
            remove_tree(dest)
        print(f'Preparing {label(dest)} (pinned upstream + recipes), about a minute...')
        subprocess.run([sys.executable, str(ROOT / 'tools/bootstrap.py'), '--dir', str(dest)], check=True)
        state = {'commit': lock['commit'], 'recipes': recipe_hashes,
                 'patched': sorted(recipe_paths(recipes)), 'repo': repo}
        state_file.write_text(json.dumps(state, indent=1) + '\n')
        return
    changed = 0
    if recipe_hashes != state['recipes']:
        patched = recipe_paths(recipes)
        paths = sorted((patched | set(state['patched'])) - set(repo))
        with tempfile.TemporaryDirectory(dir=ROOT / 'build') as tmp:
            stage = Path(tmp)
            for name in paths:
                raw = pristine(dest, name)
                if raw is not None:
                    (stage / name).parent.mkdir(parents=True, exist_ok=True)
                    (stage / name).write_bytes(raw)
            apply(stage, recipes)
            for name in paths:
                changed += put(dest, name, (stage / name).read_bytes() if (stage / name).exists() else None)
        state['recipes'], state['patched'] = recipe_hashes, sorted(patched)
    for name, digest in repo.items():
        if state['repo'].get(name) != digest or not (dest / name).exists():
            changed += put(dest, name, (ROOT / name).read_bytes())
    for name in set(state['repo']) - set(repo):
        changed += put(dest, name, pristine(dest, name))
    state['repo'] = repo
    state_file.write_text(json.dumps(state, indent=1) + '\n')
    print(f'{label(dest)}: {changed} file(s) updated')


def make(target='verify-game-3dsx'):
    bash = DEVKITPRO / 'msys2/usr/bin/bash.exe'
    if not bash.exists():
        raise SystemExit(f'devkitPro not found at {DEVKITPRO} (set DEVKITPRO_WIN)')
    command = (f'cd {shlex.quote(msys(DEV / "3ds_port"))} && make CLEAN_RELEASE=1 '
               f'PYTHON={shlex.quote(msys(sys.executable))} -j{os.cpu_count() or 4} {target}')
    subprocess.run([str(bash), '-lc', command], env={**os.environ, 'MSYSTEM': 'MSYS'}, check=True)


def build(args=None):
    sync()
    make()
    print(f'Built {IMAGE.relative_to(ROOT)}')


# ---- host tests --------------------------------------------------------------------------------------------

def remove_tree(path):
    """shutil.rmtree that also deletes read-only files (Git's pack files), which Windows will not delete as they are."""
    def writable(func, name, _error):
        os.chmod(name, stat.S_IWRITE)
        func(name)
    shutil.rmtree(path, **{'onexc' if sys.version_info >= (3, 12) else 'onerror': writable})


SUITES = ['test_poketouch_render', 'test_column_labels', 'test_option_labels', 'test_storage_touch',
          'test_storage_controller', 'test_easychat_plan', 'test_field_presentation', 'test_bag_pockets',
          'test_first_battle', 'test_device_views']


def test(args=None):
    """A subset of CI's host checks, one PASS/FAIL line per stage; exits non-zero if any stage fails."""
    bash = DEVKITPRO / 'msys2/usr/bin/bash.exe'
    gcc = DEVKITPRO / 'msys2/usr/bin/gcc.exe'
    if not bash.exists():
        raise SystemExit(f'devkitPro not found at {DEVKITPRO} (set DEVKITPRO_WIN)')
    hosts = ROOT / 'build/host-tests'
    results = []

    def stage(name, command, **options):
        print(f'--- {name}', flush=True)
        ok = subprocess.run(command, **options).returncode == 0
        results.append((name, ok))
        print(f'{"PASS" if ok else "FAIL"}  {name}', flush=True)

    suites = list(getattr(args, 'suites', None) or [])
    if suites:
        # Named suites: just those, in the MSYS shell (they call a bare gcc), against the repository's own tools/.
        command = f'cd {shlex.quote(msys(ROOT / "tools"))} && {shlex.quote(msys(sys.executable))} -m unittest {" ".join(shlex.quote(n) for n in suites)}'
        stage('python suites', [str(bash), '-lc', command], env={**os.environ, 'MSYSTEM': 'MSYS'})
        if not all(ok for _, ok in results):
            raise SystemExit(1)
        return
    sync(hosts)
    with tempfile.TemporaryDirectory() as shim:
        # The MSYS shell has no python3 (nor the Windows `py` the Makefile defaults to); the Makefile's helper steps call it.
        script = f'#!/bin/sh\nexec {shlex.quote(msys(sys.executable))} "$@"\n'
        Path(shim, 'python3').write_bytes(script.encode())
        command = f'export PATH={shlex.quote(msys(shim))}:$PATH && make -C {shlex.quote(msys(hosts / "3ds_port"))} PYTHON=python3 verify-host'
        stage('verify-host', [str(bash), '-lc', command], env={**os.environ, 'MSYSTEM': 'MSYS'})
    # These suites call a bare gcc; the MSYS compiler only finds its headers inside the MSYS shell.
    command = f'cd {shlex.quote(msys(ROOT / "tools"))} && {shlex.quote(msys(sys.executable))} -m unittest {" ".join(SUITES)}'
    stage('python suites', [str(bash), '-lc', command], env={**os.environ, 'MSYSTEM': 'MSYS'})
    stage('test_data_runtime', [sys.executable, 'test_data_runtime.py', '--cc', str(gcc)], cwd=ROOT / 'tools')
    print('\n'.join(f'{"PASS" if ok else "FAIL"}  {name}' for name, ok in results))
    if not all(ok for _, ok in results):
        raise SystemExit(1)


# ---- renders -----------------------------------------------------------------------------------------------

def render(args):
    """The B views drawn by the host harness with the pack's fonts, as PNGs in build/renders (ROM-derived: never committed)."""
    bash = DEVKITPRO / 'msys2/usr/bin/bash.exe'
    if not bash.exists():
        raise SystemExit(f'devkitPro not found at {DEVKITPRO} (set DEVKITPRO_WIN)')
    need_pak()
    text = DEV / 'src/text.c'
    if not text.exists():
        raise SystemExit('No build/dev yet (the glyph widths come from its src/text.c): python tools/dev.py build')
    views = ' '.join(shlex.quote(v) for v in (getattr(args, 'views', None) or []))
    # The harness is built by the MSYS gcc, which only finds its headers inside the MSYS shell.
    command = (f'cd {shlex.quote(msys(ROOT))} && {shlex.quote(msys(sys.executable))} tools/render_device_views.py '
               f'--pak {shlex.quote(msys(PAK))} --text {shlex.quote(msys(text))} '
               f'--out {shlex.quote(msys(ROOT / "build/renders"))}' + (f' {views}' if views else ''))
    subprocess.run([str(bash), '-lc', command], env={**os.environ, 'MSYSTEM': 'MSYS'}, check=True)


# ---- data pack ---------------------------------------------------------------------------------------------

def pak(args):
    config = load_config()
    rom = args.rom or config.get('rom')
    if not rom:
        raise SystemExit('Give the ROM once: python tools/dev.py pak --rom "C:/path/to/your-rom.gba"')
    options = args.builder_options if args.builder_options else config.get('builder_options', [])
    PAK.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, '-m', 'firered3ds_builder', 'build', '--rom', str(rom), '--output', str(PAK),
                    *options], cwd=ROOT / 'builder', check=True)
    config['rom'], config['builder_options'] = str(rom), options
    save_config(config)
    print(f'Pack: {PAK.relative_to(ROOT)}' + (f' (options: {" ".join(options)})' if options else ''))


def need_pak():
    if not PAK.exists():
        raise SystemExit('No data pack yet: python tools/dev.py pak --rom "C:/path/to/your-rom.gba"')


def copy_if_changed(source, target):
    if target.exists() and hashlib.sha256(target.read_bytes()).digest() == hashlib.sha256(source.read_bytes()).digest():
        return False
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    return True


# ---- PC: Azahar --------------------------------------------------------------------------------------------

def run(args):
    build()
    need_pak()
    config = load_config()
    sdmc = Path(config.get('azahar_sdmc') or Path(os.environ['APPDATA']) / 'Azahar' / 'sdmc')
    folder = sdmc / '3ds' / APP
    copy_if_changed(IMAGE, folder / IMAGE.name)
    if copy_if_changed(PAK, folder / PAK.name):
        print(f'Pack copied to {folder}')
    if not AZAHAR.exists():
        raise SystemExit(f'Azahar not found at {AZAHAR} (set AZAHAR_EXE)')
    subprocess.Popen([str(AZAHAR), str(folder / IMAGE.name)], close_fds=True)
    print(f'Azahar started with {folder / IMAGE.name}')


# ---- 3DS: 3dslink and ftpd ---------------------------------------------------------------------------------

def console_ip(args, required):
    config = load_config()
    ip = args.ip or config.get('ip')
    if ip and ip != config.get('ip'):
        config['ip'] = ip
        save_config(config)
    if required and not ip:
        raise SystemExit('Give the 3DS address once with --ip (ftpd shows it on the top screen)')
    return ip


def send(args):
    build()
    ip = console_ip(args, required=False)
    command = [str(DEVKITPRO / 'tools/bin/3dslink.exe')] + (['-a', ip] if ip else []) + [str(IMAGE)]
    print('On the 3DS: open the Homebrew Launcher and press Y. The game starts when it arrives.')
    subprocess.run(command, check=True)


def install(args):
    build()
    ip = console_ip(args, required=True)
    folder = f'/3ds/{APP}'
    with ftplib.FTP() as ftp:
        ftp.connect(ip, FTP_PORT, timeout=15)
        ftp.login()
        for part in ('/3ds', folder):
            try:
                ftp.mkd(part)
            except ftplib.error_perm:
                pass  # already there
        ftp.cwd(folder)
        names = ftp.nlst()
        uploads = [IMAGE]
        if args.pak or PAK.name not in [Path(n).name for n in names]:
            need_pak()
            uploads.append(PAK)
        for path in uploads:
            print(f'Uploading {path.name} ({path.stat().st_size // 1024} KiB) to {folder}/')
            with path.open('rb') as stream:
                ftp.storbinary(f'STOR {path.name}', stream)
    print(f'Installed in sd:{folder}/ - start it from the Homebrew Launcher.')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='command', required=True)
    sub.add_parser('build', help='incremental build').set_defaults(func=build)
    p = sub.add_parser('pak', help='make the data pack from your own ROM')
    p.add_argument('--rom', type=Path)
    p.add_argument('builder_options', nargs=argparse.REMAINDER,
                   help='after --: Builder options such as --gen4-sprites (remembered)')
    p.set_defaults(func=pak)
    p = sub.add_parser('test', help='host test suites (a subset of the CI checks)')
    p.add_argument('suites', nargs='*', help='only these python suites (no sync, no verify-host)')
    p.set_defaults(func=test)
    p = sub.add_parser('render', help='the B views as PNGs with your own pack (build/renders, never committed)')
    p.add_argument('views', nargs='*', help='only these views (default: all)')
    p.set_defaults(func=render)
    sub.add_parser('run', help='build and start in Azahar').set_defaults(func=run)
    p = sub.add_parser('send', help='build and run once on the 3DS (3dslink)')
    p.add_argument('--ip')
    p.set_defaults(func=send)
    p = sub.add_parser('install', help='build and copy to the 3DS SD card (ftpd)')
    p.add_argument('--ip')
    p.add_argument('--pak', action='store_true', help='upload the data pack too')
    p.set_defaults(func=install)
    args = ap.parse_args()
    if getattr(args, 'builder_options', None) and args.builder_options[0] == '--':
        args.builder_options = args.builder_options[1:]
    args.func(args)


if __name__ == '__main__':
    main()
