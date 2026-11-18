"""The "B — como ficou" boards (tools/make_device_boards.py): made from what the host harness draws, never from a pixel.

The harness (3ds_port/tests/device_render_test.c, built as test_device_views builds it: synthetic fonts, no pack) traces
what the kit is told each view draws; the tool turns a trace into a board in the structure of the owner's canvas. Runs
on Linux CI too.
"""
import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import make_device_boards as tool  # noqa: E402
from test_device_views import build, run  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]


class DeviceBoards(unittest.TestCase):
    def test_rect_becomes_an_element(self):
        html = tool.board_html({'suite': 'options', 'state': 'x', 'shell': '#c23a2e', 'items': [
            {'op': 'key', 'x': 8, 'y': 8, 'w': 73, 'h': 24, 'look': 'key', 'sunk': 1},
            {'op': 'text', 'x': 20, 'y': 12, 'w': 40, 'font': 'normal', 'ink': '#3a3a3a', 'text': 'SETTINGS'},
            {'op': 'art', 'x': 20, 'y': 124, 'w': 32, 'h': 32, 'kind': 'sprite'}]})
        self.assertIn('left: 8px; top: 10px; width: 73px; height: 24px', html)          # sunk: 2 px lower, as the kit draws it
        for want in ('SETTINGS', 'repeating-linear-gradient', '#58e070'):
            self.assertIn(want, html)
        for banned in ('data:', 'base64', '<img', '<canvas'):
            self.assertNotIn(banned, html)

    def test_harness_trace_matches_the_layout(self):    # build() and run() are test_device_views' (synthetic fonts, clean env)
        with tempfile.TemporaryDirectory() as tmp:
            harness = Path(tmp) / ('device_render_test.exe' if os.name == 'nt' else 'device_render_test')
            build(harness)
            run(harness, 'options', DEVICE_TRACE=tmp)
            items = json.loads((Path(tmp) / 'options_settings.json').read_text())['items']
            self.assertIn({'x': 8, 'y': 8, 'w': 73, 'h': 24}, [{k: i[k] for k in 'xywh'} for i in items if i['op'] == 'key'])
            self.assertTrue(any(i['op'] == 'text' and i['text'] == 'SETTINGS' for i in items))

    def test_every_boarded_state_is_traced(self):
        """Each state of the default suites is a trace (JSON, whatever words it draws) whose every piece is a board's."""
        with tempfile.TemporaryDirectory() as tmp:
            harness = Path(tmp) / ('device_render_test.exe' if os.name == 'nt' else 'device_render_test')
            build(harness)
            for suite in tool.SUITES:
                run(harness, suite, DEVICE_TRACE=tmp)
                traces = [tool.read_trace(p) for p in Path(tmp).glob(f'{suite}_*.json')]
                self.assertTrue(traces, suite)
                for trace in traces:
                    self.assertEqual(trace['items'][0]['op'], 'shell', f'{suite} {trace["state"]}')
                    self.assertIn('<x-dc>', tool.board_html(trace))

    def test_out_must_be_under_build(self):
        with tempfile.TemporaryDirectory() as tmp, self.assertRaises(SystemExit):
            tool.main(['--trace', tmp, '--out', str(Path(tmp) / 'boards')])

    def test_every_piece_has_an_element(self):
        """Each op the kit traces becomes its own element, in draw order, with the trace's words and colours."""
        items = [
            {'op': 'shell', 'screws': 3},
            {'op': 'screen', 'x': 8, 'y': 40, 'w': 260, 'h': 144},
            {'op': 'row', 'x': 14, 'y': 46, 'w': 248, 'h': 18, 'chosen': 1},
            {'op': 'rect', 'x': 20, 'y': 70, 'w': 92, 'h': 1, 'fill': '#d5d5cd'},
            {'op': 'key', 'x': 244, 'y': 190, 'w': 68, 'h': 40, 'look': 'dark', 'sunk': 0},
            {'op': 'key', 'x': 8, 'y': 190, 'w': 68, 'h': 40, 'look': 'faint', 'sunk': 1},
            {'op': 'dim', 'x': 244, 'y': 190, 'w': 68, 'h': 43},
            {'op': 'hole', 'x': 164, 'y': 120, 'w': 148, 'h': 50, 'radius': 6},
            {'op': 'led', 'x': 300, 'y': 12, 'w': 4, 'h': 4},
            {'op': 'chip', 'x': 20, 'y': 100, 'w': 40, 'h': 12, 'fill': '#e86e28', 'shadow': '#8a3a10', 'text': 'FIRE'},
            {'op': 'bar', 'x': 20, 'y': 120, 'w': 56, 'h': 5, 'fill': '#48c060', 'filled': 30},
            {'op': 'tri', 'x': 290, 'y': 70, 'w': 9, 'h': 5, 'ink': '#4a4a4a', 'dir': 'up'},
            {'op': 'text', 'x': 30, 'y': 50, 'w': 30, 'font': 'small', 'ink': '#747474', 'text': 'A < B & "C"'},
            {'op': 'art', 'x': 50, 'y': 60, 'w': 64, 'h': 64, 'kind': 'picture'},
        ]
        html = tool.board_html({'suite': 'dex', 'state': 'page', 'shell': '#3e4c6b', 'items': items})
        self.assertIn('<x-dc>', html)
        self.assertIn('Pixelify Sans', html)
        self.assertIn('width: 640px; height: 480px', html)
        self.assertIn('transform: scale(2)', html)
        self.assertIn('"default":"#3E4C6B"', html)                   # the trace's shell, the boards' Shell editor
        self.assertEqual(html.count('border-radius: 50%; background: rgba(0,0,0,0.35)'), 4)     # four screws
        for want in ('background: #2a2a30; box-shadow: inset 0 0 0 1px #4a4a54',            # the screen's bezel
                     'left: 14px; top: 46px; width: 248px; height: 132px; background: #f8f8f4',   # its paper
                     'left: 14px; top: 46px; width: 248px; height: 18px; background: #2a2a30',    # the chosen row
                     'width: 92px; height: 1px; background: #d5d5cd',
                     'left: 244px; top: 190px; width: 68px; height: 40px',                   # the dark key, up
                     'left: 8px; top: 192px; width: 68px; height: 40px',                     # the faint key, sunk
                     'background: {{shell}}; opacity: 0.5',
                     'background: rgba(0,0,0,0.28)', 'background: #e86e28', 'FIRE', 'text-shadow: 1px 1px 0 #8a3a10',
                     'width: 30px; height: 100%; background: #48c060', '▲', 'A &lt; B &amp; &quot;C&quot;',
                     'repeating-linear-gradient(45deg, #ececec 0 3px, #f6f6f6 3px 6px)'):
            self.assertIn(want, html)
        order = [html.index(s) for s in ('#d5d5cd', 'FIRE', '▲', 'A &lt;', 'repeating-linear-gradient')]
        self.assertEqual(order, sorted(order))
        self.assertEqual(html.count('#58e070; box-shadow'), 2)      # the faint key's own LED and the lone one

    def test_main_writes_the_boards_and_their_index(self):
        (ROOT / 'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory() as traces, tempfile.TemporaryDirectory(dir=ROOT / 'build') as out:
            for suite, state, shot in (('options', 'settings', 1), ('options', 'looks', 2), ('home', 'plain', 1)):
                Path(traces, f'{suite}_{state}.json').write_text(json.dumps(
                    {'suite': suite, 'state': state, 'shot': shot, 'shell': '#c23a2e', 'items': []}))
            Path(out, 'old_state.dc.html').write_text('stale')
            with contextlib.redirect_stdout(io.StringIO()):
                tool.main(['--trace', traces, '--out', out, 'options'])
            index = json.loads(Path(out, 'index.json').read_text(encoding='utf-8'))
            self.assertEqual(index, [{'file': 'options_settings.dc.html', 'title': 'B · OPTIONS: settings'},
                                     {'file': 'options_looks.dc.html', 'title': 'B · OPTIONS: looks'}])
            self.assertEqual(sorted(p.name for p in Path(out).iterdir()),
                             ['index.json', 'options_looks.dc.html', 'options_settings.dc.html'])
            with self.assertRaises(SystemExit):
                tool.main(['--trace', traces, '--out', out, 'nosuch'])


if __name__ == '__main__':
    unittest.main()
