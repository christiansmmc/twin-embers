"""The red device (style B): its drawing kit, the B views and the rectangles they share.

Builds the host harness (3ds_port/tests/device_render_test.c: the production renderer with synthetic game bridges and
stub fonts, no ROM data) and runs its suites; builds the layout test (3ds_port/tests/device_layout_test.c: every
rectangle of 3ds_device_layout.h on the screen, keys big enough, no two keys of one state overlapping). The German and
Spanish builds run every suite with 6 px glyphs, the game font's usual width, and each suite fails when a port-authored
key label has to be cut. Also checks the PNG writer and the width parser of tools/render_device_views.py.
"""
import importlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / '3ds_port'
sys.path.insert(0, str(ROOT / 'tools'))


def cc():
    return os.environ.get('CC') or shutil.which('gcc') or 'gcc'


def exe(folder, name):
    return folder / (name + '.exe' if os.name == 'nt' else name)


def build(out, extra=()):
    subprocess.run([cc(), '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror', *extra, '-I', str(PORT / 'include'),
                    '-I', str(PORT / 'src'), str(PORT / 'tests/device_render_test.c'), str(PORT / 'src/3ds_pak.c'),
                    '-o', str(out), '-lm'], check=True)


def clean_env(**extra):
    """The environment without a pack or the viewing aids a developer may have set: the synthetic run."""
    env = {k: v for k, v in os.environ.items() if k != 'FIRERED_TEST_PAK' and not k.startswith('DEVICE_')}
    env.update(extra)
    return env


def run(binary, *args, **env):
    done = subprocess.run([str(binary), *args], capture_output=True, text=True, env=clean_env(**env))
    if done.returncode:
        raise AssertionError(f'{binary.name} {" ".join(args)} failed ({done.returncode}):\n{done.stdout}{done.stderr}')
    return done.stdout


class DeviceViews(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        folder = Path(cls.tmp.name)
        cls.harness = exe(folder, 'device_render_test')
        build(cls.harness)
        cls.layout = exe(folder, 'device_layout_test')
        subprocess.run([cc(), '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror', '-I', str(PORT / 'include'),
                        str(PORT / 'tests/device_layout_test.c'), '-o', str(cls.layout)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_layout(self):
        self.assertIn('PASS device layout', run(self.layout))

    def test_kit(self):
        self.assertIn('PASS device kit', run(self.harness, 'kit'))

    def test_home(self):
        self.assertIn('PASS device home', run(self.harness, 'home'))

    def test_battle(self):
        self.assertIn('PASS device battle', run(self.harness, 'battle'))

    def test_kit_shot(self):
        """DEVICE_OUT writes <suite>_<state>.ppm: the kit's sheet, 320x240."""
        with tempfile.TemporaryDirectory() as out:
            run(self.harness, 'kit', DEVICE_OUT=out)
            raw = (Path(out) / 'kit_sheet.ppm').read_bytes()
            self.assertTrue(raw.startswith(b'P6\n320 240\n255\n'))
            self.assertEqual(len(raw), len(b'P6\n320 240\n255\n') + 320 * 240 * 3)

    def labels_fit(self, lang):
        with tempfile.TemporaryDirectory() as tmp:
            binary = exe(Path(tmp), 'device_render_test_' + lang.lower())
            build(binary, ['-DCTR_LANG_' + lang])
            self.assertIn('PASS device all', run(binary, 'all', DEVICE_FONT_W='6'))

    def test_german_labels_fit(self):
        self.labels_fit('DE')

    def test_spanish_labels_fit(self):
        self.labels_fit('ES')


class RenderTool(unittest.TestCase):
    def setUp(self):
        self.tool = importlib.import_module('render_device_views')

    def test_ppm_to_png(self):
        with tempfile.TemporaryDirectory() as tmp:
            ppm, png = Path(tmp) / 'a.ppm', Path(tmp) / 'a.png'
            pixels = bytes([255, 0, 0, 0, 255, 0, 0, 0, 255, 10, 20, 30, 40, 50, 60, 70, 80, 90])
            ppm.write_bytes(b'P6\n3 2\n255\n' + pixels)
            self.tool.ppm_to_png(ppm, png)
            raw = png.read_bytes()
            self.assertEqual(raw[:8], b'\x89PNG\r\n\x1a\n')
            chunks, pos = {}, 8
            while pos < len(raw):
                length, kind = struct.unpack('>I4s', raw[pos:pos + 8])
                body = raw[pos + 8:pos + 8 + length]
                self.assertEqual(struct.unpack('>I', raw[pos + 8 + length:pos + 12 + length])[0],
                                 zlib.crc32(kind + body) & 0xffffffff)
                chunks[kind] = chunks.get(kind, b'') + body
                pos += 12 + length
            self.assertEqual(pos, len(raw))
            self.assertEqual(struct.unpack('>IIBBBBB', chunks[b'IHDR']), (3, 2, 8, 2, 0, 0, 0))
            self.assertEqual(zlib.decompress(chunks[b'IDAT']), b'\0' + pixels[:9] + b'\0' + pixels[9:])
            self.assertIn(b'IEND', chunks)

    def test_glyph_widths_from_text_c(self):
        text = ('static const u8 sFontSmallLatinGlyphWidths[] = \n{\n 5, 5,\n};\n'
                'static const u8 sFontNormalLatinGlyphWidths[] =\n{\n' + ', '.join(['6'] * 255) + ', 3, 9, 9,\n};\n')
        self.assertEqual(self.tool.glyph_widths(text, 'sFontNormalLatinGlyphWidths'), bytes([6] * 255 + [3]))
        with self.assertRaises(ValueError):
            self.tool.glyph_widths(text, 'sFontSmallLatinGlyphWidths')   # fewer than 256

    def test_out_must_be_under_build(self):
        """Renders are ROM-derived: they may only go under build/ (ignored by Git)."""
        self.assertEqual(self.tool.out_dir(ROOT / 'build/renders'), (ROOT / 'build/renders').resolve())
        for elsewhere in (ROOT / 'docs/renders', ROOT / 'build/../renders', Path(tempfile.gettempdir()) / 'renders'):
            with self.assertRaises(SystemExit) as stop:
                self.tool.out_dir(elsewhere)
            self.assertIn('build', str(stop.exception))
        with tempfile.TemporaryDirectory() as tmp:
            done = subprocess.run([sys.executable, str(ROOT / 'tools/render_device_views.py'), '--pak', 'x.pak',
                                   '--text', 'text.c', '--out', tmp], capture_output=True, text=True)
            self.assertNotEqual(done.returncode, 0)
            self.assertIn('build', done.stderr)
            self.assertEqual(list(Path(tmp).iterdir()), [])


if __name__ == '__main__':
    unittest.main()
