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
            {'op': 'key', 'x': 8, 'y': 8, 'w': 73, 'h': 24, 'look': 'key', 'sunk': 1, 'radius': 5},
            {'op': 'text', 'x': 20, 'y': 12, 'w': 40, 'font': 'normal', 'ink': '#3a3a3a', 'text': 'SETTINGS'},
            {'op': 'art', 'x': 20, 'y': 124, 'w': 32, 'h': 32, 'kind': 'sprite'}]})
        self.assertIn('left: 8px; top: 10px; width: 73px; height: 24px', html)          # sunk: 2 px lower, as the kit draws it
        for want in ('SETTINGS', 'repeating-linear-gradient', '#58e070'):
            self.assertIn(want, html)
        for banned in ('data:', 'base64', '<img', '<canvas'):
            self.assertNotIn(banned, html)

    def test_sunk_key_led(self):
        """A sunk key's LED is 4x4 at (x + w - 11, faceY + 4) whatever the key's height (the Definitiva's TECLAS)."""
        html = tool.element({'op': 'key', 'x': 8, 'y': 8, 'w': 96, 'h': 40, 'look': 'key', 'sunk': 1, 'radius': 6})
        self.assertIn('left: 93px; top: 14px; width: 4px; height: 4px; border-radius: 50%; background: #58e070', html)

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

    def test_real_traces_pin_the_pieces(self):
        """The pieces a hand-made trace stands for, on the harness's own traces: the PC box's message dims CLOSE (dim)
        beside its screens (screen) (the box itself is never dimmed: its icons fade), the box's Party has its rows (row);
        the naming keyboard's page key FireRed's cursor turns to lights a lone LED (led); the relearner's list has its
        type chips (chip) and ▲ ▼ (tri); the Trainer Card's front has its badges not earned as holes in the paper
        (disc). Each is its board's element. A key is traced with its corners' radius as the kit clamps it (the naming
        keyboard's top row: 24 tall at radius 6), and its board's corners are that radius."""
        with tempfile.TemporaryDirectory() as tmp:
            harness = Path(tmp) / ('device_render_test.exe' if os.name == 'nt' else 'device_render_test')
            build(harness)
            for suite in ('box', 'naming', 'relearn', 'card'):
                run(harness, suite, DEVICE_TRACE=tmp)
            traces = {p.stem: tool.read_trace(p) for p in Path(tmp).glob('*.json')}
        wanted = {'box_message': {'dim': 'opacity: 0.5', 'screen': 'box-shadow: inset 0 0 0 1px #4a4a54'},
                  'box_party': {'row': 'background: #f8f8f4'},
                  'naming_cursor_page': {'led': '#58e070; box-shadow'},
                  'relearn_list': {'chip': 'color: #ffffff', 'tri': '▼'},
                  'card_front': {'disc': 'border-radius: 50%'}}
        for state, pieces in wanted.items():
            ops = [item['op'] for item in traces[state]['items']]
            html = tool.board_html(traces[state])
            for op, css in pieces.items():
                self.assertIn(op, ops, state)
                self.assertIn(css, html, f'{state} {op}')
        top = [i for i in traces['naming_upper']['items'] if i['op'] == 'key' and i['y'] == 56 and i['h'] == 24]
        self.assertEqual(len(top), 5)
        self.assertEqual({i['radius'] for i in top}, {6})
        self.assertIn('border-radius: 6px', tool.element(top[0]))

    def test_the_five_trace_their_pieces(self):
        """HOME, the battle, the Party, the Summary and the Bag (plan 04b Task 18) on the harness's own traces: HOME's map
        is art of its own kind over the window (a MAPA stand-in), its party's hole a ring, RUN's dim LED a disc; the
        battle's lead icon (at 2x) and FIGHT's swords are art, its balls discs and the place of no Pokemon a ring,
        FIGHT's word told at twice the size, a move's PP pips rects; the Party's icons and the Bag's items are art; the
        Summary's picture is art and its STATS switch rounded rects. Each is its board's element."""
        with tempfile.TemporaryDirectory() as tmp:
            harness = Path(tmp) / ('device_render_test.exe' if os.name == 'nt' else 'device_render_test')
            build(harness)
            for suite in ('home', 'battle', 'party', 'summary', 'bag'):
                run(harness, suite, DEVICE_TRACE=tmp)
            traces = {p.stem: tool.read_trace(p) for p in Path(tmp).glob('*.json')}

        def pieces(state, **want):
            return [item for item in traces[state]['items'] if all(item.get(k) == v for k, v in want.items())]

        wanted = {'home_plain': [({'op': 'art', 'kind': 'map', 'x': 14, 'y': 14, 'w': 192, 'h': 128}, 'MAPA<br>192 x 128'),
                                 ({'op': 'ring', 'x': 277, 'y': 96, 'w': 16, 'h': 16, 'ink': '#c4c4c4'},
                                  'border: 1px solid #c4c4c4'),
                                 ({'op': 'disc', 'w': 6, 'fill': '#d6ccb8'}, 'border-radius: 50%; background: #d6ccb8')],
                  'battle_action': [({'op': 'art', 'kind': 'icon', 'x': 20, 'y': 18, 'w': 48, 'h': 48}, 'ÍCONE'),
                                    ({'op': 'art', 'kind': 'icon', 'w': 22, 'h': 22}, 'title="icon"'),
                                    ({'op': 'disc', 'x': 230, 'y': 20, 'w': 9, 'fill': '#626262'}, 'background: #626262'),
                                    ({'op': 'ring', 'x': 290, 'y': 20, 'w': 9, 'h': 9, 'ink': '#b0b0a8'},
                                     'border: 1px solid #b0b0a8'),
                                    ({'op': 'text', 'font': 'large', 'text': 'FIGHT'}, 'font-size: 22px')],
                  'battle_moves': [({'op': 'rect', 'w': 5, 'h': 5}, 'width: 5px; height: 5px; background')],
                  'party_bays': [({'op': 'art', 'kind': 'icon', 'w': 32, 'h': 32}, 'title="icon"')],
                  'summary_info': [({'op': 'art', 'kind': 'picture', 'x': 30, 'y': 80, 'w': 64, 'h': 64}, 'FIGURA')],
                  'summary_stats': [({'op': 'rect', 'x': 186, 'y': 49, 'w': 70, 'h': 13, 'fill': '#b0b0a8', 'radius': 6},
                                     'border-radius: 6px; background: #b0b0a8'),
                                    ({'op': 'rect', 'x': 189, 'y': 50, 'w': 31, 'h': 11, 'fill': '#3e4c6b', 'radius': 5},
                                     'border-radius: 5px; background: #3e4c6b')],
                  'bag_items': [({'op': 'art', 'kind': 'item', 'w': 12, 'h': 12}, 'title="item"'),
                                ({'op': 'art', 'kind': 'item', 'x': 18, 'y': 199, 'w': 24, 'h': 24}, 'title="item"')]}
        for state, wants in wanted.items():
            html = tool.board_html(traces[state])
            for want, css in wants:
                found = pieces(state, **want)
                self.assertTrue(found, f'{state}: {want}')
                self.assertIn(css, tool.element(found[0]), f'{state}: {want}')
                self.assertIn(css, html, state)

    def test_rounded_rect_ring_and_large_text(self):
        """A rounded fill is a rect with its corners' radius (none: square, as before), a ring a circle's 1 px edge with
        nothing inside it, a word at twice the size the normal font doubled: capitals 6 px into its box, its shadow 2 px
        off."""
        rect = tool.element({'op': 'rect', 'x': 186, 'y': 49, 'w': 70, 'h': 13, 'fill': '#b0b0a8', 'radius': 6})
        self.assertIn('left: 186px; top: 49px; width: 70px; height: 13px; border-radius: 6px; background: #b0b0a8', rect)
        square = tool.element({'op': 'rect', 'x': 20, 'y': 70, 'w': 92, 'h': 1, 'fill': '#d5d5cd'})
        self.assertNotIn('border-radius', square)
        ring = tool.element({'op': 'ring', 'x': 290, 'y': 20, 'w': 9, 'h': 9, 'ink': '#b0b0a8'})
        for want in ('left: 290px; top: 20px; width: 9px; height: 9px', 'box-sizing: border-box', 'border-radius: 50%',
                     'border: 1px solid #b0b0a8'):
            self.assertIn(want, ring)
        self.assertNotIn('background', ring)
        big = tool.element({'op': 'text', 'x': 140, 'y': 124, 'w': 60, 'font': 'large', 'ink': '#a02a20',
                            'shadow': '#ffffff', 'text': 'FIGHT'})
        for want in ('left: 140px; top: 118px', 'font-size: 22px; line-height: 28px', 'color: #a02a20',
                     'text-shadow: 2px 2px 0 #ffffff', '>FIGHT<'):
            self.assertIn(want, big)

    def test_key_corners_are_the_traced_radius(self):
        for h, radius in ((24, 6), (24, 5), (40, 6), (20, 4)):
            html = tool.element({'op': 'key', 'x': 8, 'y': 8, 'w': 60, 'h': h, 'look': 'key', 'sunk': 0, 'radius': radius})
            self.assertIn(f'border-radius: {radius}px', html)

    def test_text_keeps_its_spaces(self):
        """A text's repeated spaces survive (the counts' "SEEN 142  OWNED 118"): white-space pre, never wrapped."""
        html = tool.element({'op': 'text', 'x': 8, 'y': 8, 'w': 90, 'font': 'normal', 'ink': '#626262',
                             'text': 'SEEN 1  OWNED 2'})
        self.assertIn('white-space: pre;', html)
        self.assertIn('SEEN 1  OWNED 2', html)

    def test_untraced_suites_are_refused(self):
        """Every view's suite makes boards (HOME, the battle, the Party, the Summary and the Bag too: plan 04b Task 18);
        the kit's sheet draws pieces the trace does not tell and a suite that is not one is no view: no board is made of
        either, before anything is built or read."""
        self.assertEqual(tool.SUITES, ['home', 'battle', 'party', 'summary', 'bag', 'options', 'save', 'card', 'mail',
                                       'dex', 'choice', 'learn', 'relearn', 'naming', 'box', 'easychat', 'frame', 'idle'])
        self.assertEqual({suite: tool.VIEWS[suite] for suite in tool.SUITES[:5]},
                         {'home': 'Início', 'battle': 'Batalha', 'party': 'POKéMON', 'summary': 'Resumo', 'bag': 'BAG'})
        (ROOT / 'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory() as traces, tempfile.TemporaryDirectory(dir=ROOT / 'build') as out:
            for suite in ('kit', 'nosuch'):
                for run_first in ([], ['--run']):
                    with self.assertRaises(SystemExit) as stop:
                        tool.main(['--trace', traces, '--out', out, *run_first, 'options', suite])
                    self.assertIn(suite, str(stop.exception))
                    self.assertIn('options', str(stop.exception))
            self.assertEqual(list(Path(traces).iterdir()), [])

    def test_the_shell_is_the_canvas_hole(self):
        """The shell's colour is a {{shell}} hole the design canvas fills (its Shell editor); a browser shows the hole
        unfilled, so the docstring and `dev.py boards`' help say so."""
        self.assertIn('{{shell}}', tool.__doc__)
        dev = (ROOT / 'tools/dev.py').read_text(encoding='utf-8')
        self.assertIn('{{shell}}', dev[dev.index("sub.add_parser('boards'"):dev.index("set_defaults(func=boards)")])

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
            {'op': 'key', 'x': 244, 'y': 190, 'w': 68, 'h': 40, 'look': 'dark', 'sunk': 0, 'radius': 6},
            {'op': 'key', 'x': 8, 'y': 190, 'w': 68, 'h': 40, 'look': 'faint', 'sunk': 1, 'radius': 6},
            {'op': 'dim', 'x': 244, 'y': 190, 'w': 68, 'h': 43},
            {'op': 'hole', 'x': 164, 'y': 120, 'w': 148, 'h': 50, 'radius': 6},
            {'op': 'led', 'x': 300, 'y': 12, 'w': 4, 'h': 4},
            {'op': 'chip', 'x': 20, 'y': 100, 'w': 40, 'h': 12, 'fill': '#e86e28', 'shadow': '#8a3a10', 'text': 'FIRE'},
            {'op': 'bar', 'x': 20, 'y': 120, 'w': 56, 'h': 5, 'fill': '#48c060', 'filled': 30},
            {'op': 'tri', 'x': 290, 'y': 70, 'w': 9, 'h': 5, 'ink': '#4a4a4a', 'dir': 'up'},
            {'op': 'text', 'x': 30, 'y': 50, 'w': 30, 'font': 'small', 'ink': '#747474', 'text': 'A < B & "C"'},
            {'op': 'art', 'x': 50, 'y': 60, 'w': 64, 'h': 64, 'kind': 'picture'},
            {'op': 'disc', 'x': 125, 'y': 134, 'w': 32, 'h': 32, 'fill': '#e2e2da', 'rim': '#c8c8c0'},
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
        hole = tool.element(items[-1])                              # a paper hole: a disc with its 2 px top rim
        for want in ('left: 125px; top: 134px; width: 32px; height: 32px', 'border-radius: 50%', 'background: #e2e2da',
                     'box-shadow: inset 0 2px 0 #c8c8c0'):
            self.assertIn(want, hole)
        spot = tool.element({'op': 'disc', 'x': 226, 'y': 164, 'w': 5, 'h': 5, 'fill': '#3e4c6b'})
        self.assertIn('border-radius: 50%; background: #3e4c6b', spot)
        self.assertNotIn('box-shadow', spot)                         # a disc without a rim

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
