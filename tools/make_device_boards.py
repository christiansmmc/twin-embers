"""The red device's views as boards ("B — como ficou"), made from what the host harness draws.

    python tools/make_device_boards.py --trace DIR --out build/boards [--run] [SUITE ...]

Reads the harness's traces, DIR/<suite>_<state>.json (3ds_port/tests/device_render_test.c with DEVICE_TRACE=DIR: what
the kit is told each state draws, in draw order), and writes OUT/<suite>_<state>.dc.html, one board per state in the
structure of the owner's canvas (.progress/design-b/project/B*.dc.html: the 320x240 screen at twice its size in the
shell's colour, Pixelify Sans standing in for the game's font, a striped box where art goes), and OUT/index.json
([{"file", "title"}], in the suites' order and each suite's own). With --run it first builds the harness with its
synthetic fonts and no pack and runs the suites with DEVICE_TRACE=DIR. A trace holds the harness's own words and the
layout's rectangles, never a pixel: no art and nothing of a pack can reach a board. `python tools/dev.py boards` runs
this with --run in the devkitPro MSYS shell (default suites: the views plan 3 drew without a board).
"""
import argparse
import html
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / '3ds_port'
SUITES = ['options', 'save', 'card', 'mail', 'dex', 'choice', 'learn', 'relearn', 'naming', 'box', 'easychat', 'frame',
          'idle']
# A board's title: "B · <view>: <state>" (the canvas's words: pt-BR, the game's own in capitals).
VIEWS = {'kit': 'Kit', 'home': 'Início', 'battle': 'Batalha', 'party': 'POKéMON', 'summary': 'Resumo', 'bag': 'BAG',
         'options': 'OPTIONS', 'save': 'SAVE', 'card': 'Cartão de treinador', 'mail': 'Carta', 'dex': 'POKéDEX',
         'choice': 'Escolhas', 'learn': 'Esquecer golpe', 'relearn': 'Relembrar golpe', 'naming': 'Teclado de nomes',
         'box': 'PC: caixas', 'easychat': 'Easy Chat', 'frame': 'Tela do FireRed', 'idle': 'Em repouso'}
# What a stand-in box says it stands for (art the view draws from the pack, or the project's logo).
ART = {'icon': 'ÍCONE', 'picture': 'FIGURA', 'item': 'ITEM', 'badge': 'INSÍGNIA', 'wallpaper': 'PAPEL DE PAREDE',
       'mark': 'MARCA', 'sprite': 'SPRITE', 'frame': 'MOLDURA', 'logo': 'LOGO', 'gba': 'TELA DO FIRERED'}

# The kit's numbers (3ds_device_layout.h, 3ds_device_kit_impl.h) and the boards' CSS for what it draws.
SINK = 2                            # DEV_SINK: a chosen key's face, lower
LED = '#58e070'
PAPER, BEZEL = '#f8f8f4', '#2a2a30'
STRIPES = 'repeating-linear-gradient(45deg, #ececec 0 3px, #f6f6f6 3px 6px)'
KEY_UP = {'key': 'background: #f2ece0; box-shadow: 0 3px 0 rgba(0,0,0,0.38), inset 0 -2px 0 #d6ccb8, inset 0 1px 0 #ffffff',
          'dark': 'background: #2a2a30; box-shadow: 0 3px 0 rgba(0,0,0,0.38), inset 0 1px 0 #4a4a54',
          'faint': 'background: #d2cab8; box-shadow: 0 3px 0 rgba(0,0,0,0.38), inset 0 -2px 0 #bcb29c'}
KEY_SUNK = {'key': 'background: #e6dece; box-shadow: 0 1px 0 rgba(0,0,0,0.38), inset 0 2px 0 #cfc4ae',
            'dark': 'background: #2a2a30; box-shadow: 0 1px 0 rgba(0,0,0,0.38), inset 0 2px 0 rgba(0,0,0,0.38)',
            'faint': 'background: #d2cab8; box-shadow: 0 1px 0 rgba(0,0,0,0.38), inset 0 2px 0 rgba(0,0,0,0.38)'}
SCREWS = {1: ((2, 2), (313, 2)), 2: ((2, 232), (313, 232))}
ARROWS = {'up': '▲', 'down': '▼', 'left': '◀', 'right': '▶'}
# A text's box: the capitals start 3 px into a box of the normal font, 2 px into one of the small (the layout's rule).
# Pixelify Sans at these sizes is about as wide as the harness's 6 px and 5 px stub glyphs (STUB_WIDTH, its small 5).
FONTS = {'normal': (3, 'font-size: 11px; line-height: 14px'), 'small': (2, 'font-size: 9px; line-height: 10px')}
STUB_WIDTH = 6                      # the game font's usual width (the ES and DE label-fit builds' too)

HEAD = '''<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<title>{title}</title>
<script src="./support.js"></script>
</head>
<body>
<x-dc>
<helmet>
<link href="https://fonts.googleapis.com/css2?family=Pixelify+Sans:wght@400;600&amp;display=swap" rel="stylesheet">
<style>
body{{margin:0}}
a{{color:#3e4c6b}}a:hover{{color:#26324a}}
</style>
</helmet>
<div style="width: 640px; height: 480px; overflow: hidden; background: #1e1e1e; font-family: 'Pixelify Sans', monospace">
<div style="position: relative; width: 320px; height: 240px; transform: scale(2); transform-origin: 0 0; \
background: {{{{shell}}}}; box-shadow: inset 0 2px 0 rgba(255,255,255,0.25), inset 0 -3px 0 rgba(0,0,0,0.3); color: #4a4a4a; \
font-size: 11px; line-height: 14px; text-shadow: none">
'''
TAIL = '''</div>
</div>
</x-dc>
<script type="text/x-dc" data-dc-script data-props='{{"shell":{{"editor":"color","default":"{shell}","options":\
["#C23A2E","#3E4C6B","#2E7A4E","#5A5A64"]}},"$preview":{{"width":640,"height":480}}}}'>
class Component extends DCLogic {{
  renderVals() {{
    return {{ shell: this.props.shell ?? '{shell}' }};
  }}
}}
</script>
</body>
</html>
'''


def title(suite, state):
    return f'B · {VIEWS.get(suite, suite.upper())}: {state.replace("_", " ")}'


def words(text):
    """Text for a board: HTML-escaped, and no "{" the canvas would take for a template."""
    return html.escape(text).replace('{', '&#123;').replace('}', '&#125;')


def box(item, dy=0):
    return f'position: absolute; left: {item["x"]}px; top: {item["y"] + dy}px; width: {item["w"]}px; height: {item["h"]}px'


def shadow(colour):
    return f'text-shadow: 1px 1px 0 {colour}' if colour else 'text-shadow: none'


def led(x, y, size):
    return (f'<span style="position: absolute; left: {x}px; top: {y}px; width: {size}px; height: {size}px; '
            f'border-radius: 50%; background: {LED}; box-shadow: 0 0 3px {LED}"></span>')


def element(item):
    """A traced piece as the boards draw it (a key's LED with its key)."""
    op = item['op']
    if op == 'shell':
        return '\n'.join(f'<span style="position: absolute; left: {x}px; top: {y}px; width: 5px; height: 5px; '
                         'border-radius: 50%; background: rgba(0,0,0,0.35); box-shadow: inset 1px 1px 0 '
                         'rgba(255,255,255,0.35)"></span>'
                         for bit, places in SCREWS.items() if item['screws'] & bit for x, y in places)
    if op == 'screen':
        paper = {'x': item['x'] + 6, 'y': item['y'] + 6, 'w': item['w'] - 12, 'h': item['h'] - 12}
        return (f'<div style="{box(item)}; border-radius: 6px; background: {BEZEL}; box-shadow: inset 0 0 0 1px #4a4a54, '
                f'0 1px 0 rgba(255,255,255,0.25)"></div>\n<div style="{box(paper)}; background: {PAPER}"></div>')
    if op == 'key':
        sunk, h = item['sunk'], item['h']
        styles = KEY_SUNK if sunk else KEY_UP
        out = (f'<button type="button" style="{box(item, SINK if sunk else 0)}; padding: 0; border: 0; '
               f'border-radius: {5 if h <= 24 else 6}px; {styles[item["look"]]}"></button>')
        if sunk:
            size = 5 if h >= 30 else 4
            out += '\n' + led(item['x'] + item['w'] - 7 - size, item['y'] + SINK + (6 if h >= 40 else 4), size)
        return out
    if op == 'dim':
        return f'<div style="{box(item)}; background: {{{{shell}}}}; opacity: 0.5"></div>'
    if op == 'hole':
        return (f'<div style="{box(item)}; border-radius: {item["radius"]}px; background: rgba(0,0,0,0.28); '
                'box-shadow: inset 0 3px 0 rgba(0,0,0,0.35), 0 1px 0 rgba(255,255,255,0.2)"></div>')
    if op == 'row':
        return f'<div style="{box(item)}; background: {BEZEL if item["chosen"] else PAPER}"></div>'
    if op == 'rect':
        return f'<div style="{box(item)}; background: {item["fill"]}"></div>'
    if op == 'led':
        return led(item['x'], item['y'], item['w'])
    if op == 'text':
        into, size = FONTS[item['font']]
        return (f'<div style="position: absolute; left: {item["x"]}px; top: {item["y"] - into}px; white-space: nowrap; '
                f'{size}; color: {item["ink"]}; {shadow(item.get("shadow"))}">{words(item["text"])}</div>')
    if op == 'chip':
        ink_shadow = item['shadow'] if item['shadow'] != '#ffffff' else None
        return (f'<span style="{box(item)}; overflow: hidden; background: {item["fill"]}; color: #ffffff; '
                f'{shadow(ink_shadow)}; font-size: 8px; line-height: {item["h"]}px; text-align: center; '
                f'white-space: nowrap">{words(item["text"])}</span>')
    if op == 'bar':
        return (f'<span style="{box(item)}; box-sizing: border-box; border: 1px solid #404850; background: #d8d8d0">'
                f'<span style="display: block; width: {item["filled"]}px; height: 100%; background: {item["fill"]}">'
                '</span></span>')
    if op == 'tri':
        return (f'<span style="{box(item)}; display: flex; align-items: center; justify-content: center; '
                f'color: {item["ink"]}; font-size: 9px; line-height: 9px">{ARROWS[item["dir"]]}</span>')
    if op == 'art':
        label = ''
        if item['w'] >= 40 and item['h'] >= 24:
            label = (f'{ART.get(item["kind"], item["kind"].upper())}<br>{item["w"]} x {item["h"]}')
        return (f'<div style="{box(item)}; box-sizing: border-box; border: 1px dashed #9a9a9a; background: {STRIPES}; '
                'display: flex; align-items: center; justify-content: center; font-size: 8px; line-height: 10px; '
                f'color: #6a6a6a; text-align: center" title="{item["kind"]}">{label}</div>')
    raise ValueError(f'unknown piece {op!r}')


def board_html(trace: dict) -> str:
    """One state's board: each piece of its trace an element, in draw order, on the shell of the trace's colour."""
    body = '\n'.join(element(item) for item in trace['items'])
    return (HEAD.format(title=words(title(trace['suite'], trace['state']))) + body + '\n'
            + TAIL.format(shell=trace['shell'].upper()))


def out_dir(path):
    """`path` resolved, when it lies under the repository's build/ (ignored by Git), where boards are made."""
    out, build = Path(path).resolve(), (ROOT / 'build').resolve()
    if out != build and build not in out.parents:
        raise SystemExit(f'--out {path}: boards are made only under {build} (ignored by Git)')
    return out


def read_trace(path):
    try:
        return json.loads(Path(path).read_text(encoding='utf-8'))
    except ValueError as error:
        raise SystemExit(f'{path}: not a trace ({error})') from error


def run_harness(trace, suites):
    """Build the harness (its synthetic fonts, STUB_WIDTH wide, no pack) in `trace` and trace each suite there, its old
    traces gone."""
    trace.mkdir(parents=True, exist_ok=True)
    harness = trace / ('device_render_test.exe' if os.name == 'nt' else 'device_render_test')
    subprocess.run([os.environ.get('CC') or shutil.which('gcc') or 'gcc', '-std=c11', '-O1', '-Wall', '-Wextra',
                    '-Werror', '-I', str(PORT / 'include'), '-I', str(PORT / 'src'), '-I', str(PORT / 'tests'),
                    str(PORT / 'tests/device_render_test.c'), str(PORT / 'src/3ds_pak.c'), '-o', str(harness), '-lm'],
                   check=True)
    env = {k: v for k, v in os.environ.items() if k != 'FIRERED_TEST_PAK' and not k.startswith('DEVICE_')}
    env.update(DEVICE_TRACE=str(trace), DEVICE_FONT_W=str(STUB_WIDTH))
    for suite in suites:
        for old in trace.glob(f'{suite}_*.json'):
            old.unlink()
        done = subprocess.run([str(harness), suite], capture_output=True, text=True, env=env)
        if done.returncode:
            raise SystemExit(f'{suite}: the harness failed ({done.returncode}):\n{done.stdout}{done.stderr}')


def main(argv: list[str] | None = None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--trace', type=Path, required=True, help='the harness traces (DEVICE_TRACE=DIR)')
    ap.add_argument('--out', type=Path, required=True, help='where the boards and index.json go (under build/ only)')
    ap.add_argument('--run', action='store_true', help='build the harness and trace the suites first')
    ap.add_argument('suites', nargs='*', help=f'suites to make boards of (default: {", ".join(SUITES)})')
    args = ap.parse_args(argv)
    out = out_dir(args.out)
    suites = args.suites or SUITES
    if args.run:
        run_harness(args.trace.resolve(), suites)
    traces = [read_trace(p) for p in sorted(args.trace.glob('*.json'))]
    boards = []
    for suite in suites:
        mine = sorted((t for t in traces if t['suite'] == suite), key=lambda t: t.get('shot', 0))
        if not mine:
            raise SystemExit(f'No trace of {suite} in {args.trace} (DEVICE_TRACE, or --run)')
        boards += mine
    out.mkdir(parents=True, exist_ok=True)
    for old in [*out.glob('*.dc.html'), out / 'index.json']:
        old.unlink(missing_ok=True)
    index = []
    for trace in boards:
        name = f'{trace["suite"]}_{trace["state"]}.dc.html'
        (out / name).write_text(board_html(trace), encoding='utf-8')
        index.append({'file': name, 'title': title(trace['suite'], trace['state'])})
        print((out / name).as_posix())
    (out / 'index.json').write_text(json.dumps(index, ensure_ascii=False, indent=1) + '\n', encoding='utf-8')
    print(f'{len(index)} boards, {(out / "index.json").as_posix()}')


if __name__ == '__main__':
    main(sys.argv[1:])
