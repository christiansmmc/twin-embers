"""The battle Bag keeps the "Llama" look (the owner's mockup, llama2_mockup.html) until it is redone as the red
device; the rest of the battle's lower screen, the Party and the Summary are the red device's (test_device_views.py's
battle, party and summary suites).

Draws the Bag through the production renderer with synthetic bridges
(3ds_port/tests/battle_ui_render_test.c): incremental updates equal full redraws, no colour of the shared navy theme
remains, the touch areas answer where the cards are drawn, and the fallback icons (no pack) move nothing. Up to the
Ascuas restyle this test also compared the buttons' hit areas and every state's silhouette with the renderer before it
(BASE); the Llama look moved the cards on purpose, so that comparison is gone and the rectangles are asserted in the C
test instead.
"""
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def cc():
    return os.environ.get('CC') or shutil.which('gcc') or 'gcc'


def build(out, extra, includes):
    cmd = [cc(), '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror'] + extra
    for inc in includes:
        cmd.append('-I' + str(inc))
    cmd += [str(ROOT / '3ds_port/tests/battle_ui_render_test.c'), '-o', str(out), '-lm']
    subprocess.run(cmd, check=True)


class BattleUi(unittest.TestCase):
    def test_llama_states_draw_and_update(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            exe = tmp / ('battle-ui.exe' if os.name == 'nt' else 'battle-ui')
            build(exe, [], [ROOT / '3ds_port/include', ROOT / '3ds_port/src'])
            now = subprocess.run([str(exe), 'print'], check=True, capture_output=True, text=True).stdout
            self.assertIn('PASS battle ui', now)
            # No pack: the 12x12 menu samples stand in for the full-size icons; the same geometry either way.
            bare = subprocess.run([str(exe), 'print'], check=True, capture_output=True, text=True,
                                  env=dict(os.environ, BATTLE_UI_NO_PACK='1')).stdout
            self.assertIn('PASS battle ui', bare)
            self.assertEqual(now, bare, 'the fallback icons moved something')


if __name__ == '__main__':
    unittest.main()
