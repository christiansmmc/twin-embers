"""Day-to-day loop for this fork on Windows: build, data pack, PC emulator, 3DS over Wi-Fi.

    python tools/dev.py build                  incremental build into build/dev
    python tools/dev.py pak [--rom FILE] [-- BUILDER OPTIONS]
                                               the data pack, made by this fork's Builder from your own ROM
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
STATE = DEV / '.dev-sync.json'
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
    return '/' + p.drive[0].lower() + p.as_posix()[len(p.drive):]


# ---- build -------------------------------------------------------------------------------------------------

def recipe_paths(recipes):
    paths = set()
    for recipe in recipes:
        for row in json.loads(recipe.read_text(encoding='utf-8'))['files']:
            paths.add(row['path'])
    return paths


def pristine(name):
    """The file as the pinned upstream commit has it, or None when upstream has no such file."""
    probe = subprocess.run(['git', '-C', str(DEV), 'cat-file', '-e', f'HEAD:{name}'], capture_output=True)
    if probe.returncode != 0:
        return None
    return subprocess.check_output(['git', '-C', str(DEV), 'show', f'HEAD:{name}'])


def put(name, raw):
    """Write build/dev/<name> only when its content changes, so make keeps every other timestamp."""
    target = DEV / name
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


def sync():
    lock = tomllib.loads((ROOT / 'upstream.lock').read_text())['pokefirered']
    current = inputs()
    recipes = sorted((ROOT / 'patches/pokefirered').glob('*.json'))
    recipe_hashes = {k: v for k, v in current.items() if k.startswith('patches/')}
    repo = {k: v for k, v in current.items() if not k.startswith('patches/')}
    state = json.loads(STATE.read_text()) if STATE.exists() else None
    if state is None or state.get('commit') != lock['commit']:
        if DEV.exists():
            shutil.rmtree(DEV)
        print('Preparing build/dev (pinned upstream + recipes), about a minute...')
        subprocess.run([sys.executable, str(ROOT / 'tools/bootstrap.py'), '--dir', str(DEV)], check=True)
        state = {'commit': lock['commit'], 'recipes': recipe_hashes,
                 'patched': sorted(recipe_paths(recipes)), 'repo': repo}
        STATE.write_text(json.dumps(state, indent=1) + '\n')
        return
    changed = 0
    if recipe_hashes != state['recipes']:
        patched = recipe_paths(recipes)
        paths = sorted((patched | set(state['patched'])) - set(repo))
        with tempfile.TemporaryDirectory(dir=ROOT / 'build') as tmp:
            stage = Path(tmp)
            for name in paths:
                raw = pristine(name)
                if raw is not None:
                    (stage / name).parent.mkdir(parents=True, exist_ok=True)
                    (stage / name).write_bytes(raw)
            apply(stage, recipes)
            for name in paths:
                changed += put(name, (stage / name).read_bytes() if (stage / name).exists() else None)
        state['recipes'], state['patched'] = recipe_hashes, sorted(patched)
    for name, digest in repo.items():
        if state['repo'].get(name) != digest or not (DEV / name).exists():
            changed += put(name, (ROOT / name).read_bytes())
    for name in set(state['repo']) - set(repo):
        changed += put(name, pristine(name))
    state['repo'] = repo
    STATE.write_text(json.dumps(state, indent=1) + '\n')
    print(f'build/dev: {changed} file(s) updated')


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
