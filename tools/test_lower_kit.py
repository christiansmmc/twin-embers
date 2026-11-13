"""The lower screen kit's host tests: the real renderer with synthetic text-window entries."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FLAGS = ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffunction-sections', '-fdata-sections',
         '-Wl,--gc-sections', '-I' + str(ROOT / '3ds_port/include'), '-I' + str(ROOT / '3ds_port/src')]


def build(folder, source, extra=()):
    exe = folder / (Path(source).stem + ('.exe' if os.name == 'nt' else ''))
    subprocess.run([os.environ.get('CC', 'gcc'), *FLAGS, str(ROOT / source), *map(str, extra), '-o', str(exe)],
                   check=True)
    return exe


class LowerKit(unittest.TestCase):
    def test_kit(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = build(Path(tmp), '3ds_port/tests/lower_kit_test.c', [ROOT / '3ds_port/src/3ds_pak.c'])
            run = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertIn('PASS lower kit', run.stdout)
            run = subprocess.run([str(exe), 'fallback'], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertIn('PASS lower kit fallback', run.stdout)


if __name__ == '__main__':
    unittest.main()
