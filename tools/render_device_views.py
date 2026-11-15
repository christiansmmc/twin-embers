"""Render the red device's B views (style B) as PNGs with the owner's own pack, for a visual check against the boards.

    python tools/render_device_views.py --pak PACK --text TEXT_C --out DIR [VIEW ...]

Builds the host harness (3ds_port/tests/device_render_test.c) in DIR, runs each view's suite with FIRERED_TEST_PAK (the
game's fonts come from the pack) and the glyph widths parsed from the game's text.c, and writes DIR/<view>/<state>.png.
Everything it writes is ROM-derived and stays under build/ (ignored by Git): never commit or upload a render. The suites
check what they draw as they do in test_device_views.py, so a failing check fails the render. `python tools/dev.py
render` runs this in the devkitPro MSYS shell with the usual paths.
"""
import argparse
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / '3ds_port'
VIEWS = ['kit', 'home', 'battle', 'party', 'summary']     # the harness's suites, in its order


def glyph_widths(text, name):
    """The first 256 entries of the u8 table `name` in the game's text.c, as bytes."""
    found = re.search(re.escape(name) + r'\s*\[\s*\]\s*=\s*\{([^}]*)\}', text)
    if not found:
        raise ValueError(f'{name} not found')
    values = [int(v, 0) for v in re.findall(r'0x[0-9a-fA-F]+|\d+', found.group(1))]
    if len(values) < 256:
        raise ValueError(f'{name} has {len(values)} entries, not 256')
    return bytes(values[:256])


def out_dir(path):
    """`path` resolved, when it lies under the repository's build/ (ignored by Git): a render is ROM-derived."""
    out, build = Path(path).resolve(), (ROOT / 'build').resolve()
    if out != build and build not in out.parents:
        raise SystemExit(f'--out {path}: renders are ROM-derived and go only under {build} (ignored by Git)')
    return out


def ppm_to_png(ppm: Path, png: Path) -> None:
    """A binary PPM (P6, 8-bit) as an RGB8 PNG."""
    raw = Path(ppm).read_bytes()
    head = re.match(rb'P6\s+(\d+)\s+(\d+)\s+(\d+)\s', raw)
    if not head or int(head.group(3)) != 255:
        raise ValueError(f'{ppm}: not an 8-bit P6 image')
    width, height = int(head.group(1)), int(head.group(2))
    pixels = raw[head.end():head.end() + width * height * 3]
    if len(pixels) != width * height * 3:
        raise ValueError(f'{ppm}: truncated')
    rows = b''.join(b'\0' + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))

    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body) & 0xffffffff)

    Path(png).write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
                          + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--pak', type=Path, required=True, help="the owner's data pack")
    ap.add_argument('--text', type=Path, required=True, help="the game's src/text.c (build/dev/src/text.c)")
    ap.add_argument('--out', type=Path, required=True,
                    help='where the harness, the widths and the PNGs go (under build/ only)')
    ap.add_argument('views', nargs='*', help=f'views to render (default: all of {", ".join(VIEWS)})')
    args = ap.parse_args()
    views = args.views or VIEWS
    unknown = [v for v in views if v not in VIEWS]
    if unknown:
        raise SystemExit(f'No such view: {", ".join(unknown)} (known: {", ".join(VIEWS)})')
    out = out_dir(args.out)
    out.mkdir(parents=True, exist_ok=True)
    text = args.text.read_text(encoding='utf-8', errors='replace')
    widths, small = out / 'widths.bin', out / 'small_widths.bin'
    widths.write_bytes(glyph_widths(text, 'sFontNormalLatinGlyphWidths'))
    small.write_bytes(glyph_widths(text, 'sFontSmallLatinGlyphWidths'))
    harness = out / ('device_render_test.exe' if os.name == 'nt' else 'device_render_test')
    subprocess.run([os.environ.get('CC') or shutil.which('gcc') or 'gcc', '-std=c11', '-O1', '-Wall', '-Wextra',
                    '-Werror', '-I', str(PORT / 'include'), '-I', str(PORT / 'src'),
                    str(PORT / 'tests/device_render_test.c'), str(PORT / 'src/3ds_pak.c'), '-o', str(harness), '-lm'],
                   check=True)
    env = {**os.environ, 'FIRERED_TEST_PAK': str(args.pak.resolve()), 'DEVICE_WIDTHS': str(widths),
           'DEVICE_SMALL_WIDTHS': str(small)}
    env.pop('DEVICE_FONT_W', None)
    for view in views:
        with tempfile.TemporaryDirectory(dir=out) as shots:
            env['DEVICE_OUT'] = shots
            done = subprocess.run([str(harness), view], capture_output=True, text=True, env=env)
            sys.stdout.write(done.stdout)
            sys.stderr.write(done.stderr)
            if done.returncode:
                raise SystemExit(f'{view}: the harness failed ({done.returncode})')
            folder = out / view
            folder.mkdir(exist_ok=True)
            for ppm in sorted(Path(shots).glob(f'{view}_*.ppm')):
                png = folder / (ppm.stem + '.png')
                ppm_to_png(ppm, png)
                try:
                    print(png.relative_to(Path.cwd()).as_posix())
                except ValueError:
                    print(png)


if __name__ == '__main__':
    main()
