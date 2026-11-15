"""Production lower-screen routing and Flash/cave geometry with synthetic state."""
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    match = re.search(r'(?:static )?[\w *]+\b' + name + r'\([^;]+?\)\s*\{', source)
    if not match:
        raise AssertionError(name)
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


def additions(name, path):
    doc = json.loads((ROOT / 'patches/pokefirered' / name).read_text())
    row = next(r for r in doc['files'] if r['path'] == path)
    return ''.join(op['port'] for op in row['lines'] if 'port' in op)


class FieldPresentation(unittest.TestCase):
    def run_c(self, source):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            c = folder / 'test.c'
            c.write_text(source)
            exe = folder / ('test.exe' if os.name == 'nt' else 'test')
            subprocess.run([os.environ.get('CC', 'gcc'), '-std=c11', '-O2',
                            '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / '3ds_port/include'),
                            str(c), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_move_selection_and_item_animation_ownership(self):
        code = additions('0105-move-learning-bottom.json', 'src/pokemon_summary_screen.c')
        code += additions('0105-move-learning-bottom.json', 'src/pokemon_special_anim.c')
        code += additions('0105-move-learning-bottom.json', 'src/party_menu.c')
        code += additions('0107-cave-preview-presentation.json', 'src/fldeff_flash.c')
        self.run_c('''
#include <assert.h>
#include <stddef.h>
typedef void (*Callback)(void);
static void CB2_SetUpPSS(void) {}
static void CB2_RunPokemonSummaryScreen(void) {}
static void CB2_PSA(void) {}
static void CB2_ChangeMapMain(void) {}
static void Field(void) {}
static struct { Callback callback2; int inBattle; } gMain;
static int gPlayerParty[6], otherParty[6];
enum { PSS_MODE_NORMAL, PSS_MODE_SELECT_MOVE, PSS_MODE_FORGET_MOVE };
static struct Screen { struct { int *mons; } monList; int mode; } screen, *sMonSummaryScreen;
static int internal, *sPartyMenuInternal;
enum { Task_HandleRestoreWhichMoveInput=1, Task_DisplayLevelUpStatsPg2, Task_TryLearnNewMoves };
static int activeTask;
static int FuncIsActiveTask(int task) { return activeTask==task; }
static int CtrBottomParty_IsPartyCallback(void) { return gMain.callback2==Field; }
#define PLATFORM_3DS 1
''' + code + '''
int main(void) {
    assert(!CtrBottomSummary_IsMoveSelection());
    sMonSummaryScreen=&screen; screen.monList.mons=gPlayerParty;
    screen.mode=PSS_MODE_SELECT_MOVE; gMain.callback2=CB2_SetUpPSS;
    assert(CtrBottomSummary_IsMoveSelection());
    gMain.callback2=CB2_RunPokemonSummaryScreen;
    assert(CtrBottomSummary_IsMoveSelection());
    screen.mode=PSS_MODE_FORGET_MOVE; assert(CtrBottomSummary_IsMoveSelection());
    screen.mode=PSS_MODE_NORMAL; assert(!CtrBottomSummary_IsMoveSelection());
    screen.mode=PSS_MODE_SELECT_MOVE; screen.monList.mons=otherParty;
    assert(!CtrBottomSummary_IsMoveSelection());
    gMain.callback2=CB2_PSA; assert(CtrBottomPartyChild_IsOpen());
    gMain.inBattle=1; assert(!CtrBottomPartyChild_IsOpen());
    gMain.inBattle=0; gMain.callback2=Field;
    assert(!CtrBottomPartyChild_IsOpen() && !CtrField_CaveTransitionActive());
    sPartyMenuInternal=&internal;
    for (activeTask=1;activeTask<=3;++activeTask) assert(CtrBottomPartyChild_IsMovePanel());
    activeTask=0; assert(!CtrBottomPartyChild_IsMovePanel());
    gMain.callback2=CB2_ChangeMapMain; assert(CtrField_CaveTransitionActive());
    activeTask=1; assert(!CtrBottomPartyChild_IsMovePanel());
}
''')

    def test_child_session_survives_long_animation_and_releases_top(self):
        source = (ROOT / '3ds_port/src/3ds_bottom.c').read_text()
        begin = function(source, 'BeginSession')
        update = function(source, 'UpdateSession')
        start = source.index('enum { MODE_OFF')
        modes = source[start:source.index(';', start) + 1]
        names = set(re.findall(r'\b(Ctr\w+)\(\)', begin + update))
        names -= {'CtrBottom_CurrentCallback', 'CtrBottomParty_Callback'}
        stubs = '\n'.join('static int ' + n + '_value;\nint ' + n
                          + '(void) { return ' + n + '_value; }' for n in sorted(names))
        start = source.index('    memset(s, 0, sizeof(*s));', source.index('static void SnapshotState('))
        end = source.index('    if (mode == MODE_STORAGE_PC', start)
        snapshot = source[start:end]
        self.run_c('''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "3ds_poketouch.h"
#include "3ds_lang.h"
#define BRIDGE_MAX 150
#define CTR_SUMMARY_BATTLE_PARTY 2
#define CtrLog_Write(...) ((void)0)
static struct { int active,owner,battle,entered,frames,away; void *partyCallback; } sSession;
static int sScreen,sStorageSession,sPendingScreen=PT_NONE;
static void *sPartyMenuCallback;
static void *CtrBottom_CurrentCallback(void) { return NULL; }
static void *CtrBottomParty_Callback(void) { return NULL; }
static int CtrPokeTouch_LeavingFor(int root) { (void)root; return 0; }
static int NativeRoot(int root) { (void)root; return 0; }
''' + modes + '\n' + stubs + '\n' + begin + '''
static void BeginSessionPending(int owner) { BeginSession(owner); }
#define IS_CASE_MODE(m) ((m)==MODE_TMCASE || (m)==MODE_POUCH)
''' + update + '''
typedef struct { unsigned char mode,pressed; int cursor,tapped; CtrPokeTouchView touch; } Snapshot;
static void LearnSnapshot(Snapshot *s,int mode) { (void)s; (void)mode; }   /* no move to forget open */
static void NameSnapshot(Snapshot *s) { (void)s; }   /* no naming screen open */
static void ShopSnapshot(Snapshot *s,int mode) { (void)s; (void)mode; }   /* no mart open */
static void snapshot(Snapshot *s,int mode,int pressed) {
''' + snapshot + '''
}
int main(void) {
    assert(UpdateSession(MODE_PARTY_CHILD,0));
    assert(sSession.active && sSession.entered && sSession.owner==PT_POKEMON);
    for (int i=0;i<3000;++i) assert(UpdateSession(MODE_PARTY_CHILD,0));
    assert(sSession.away==0);
    Snapshot state; snapshot(&state,MODE_PARTY_CHILD,0);
    assert(state.touch.viewport && !state.touch.backVisible);
    for (int i=0;i<PT_COUNT;++i) assert(!state.touch.enabled[i]);
    CtrBottom_IsFieldCallback_value=1;
    CtrBottom_IsFading_value=1;
    assert(UpdateSession(MODE_FIELD,0) && sSession.active);
    CtrBottom_IsFading_value=0;
    assert(!UpdateSession(MODE_FIELD,0) && !sSession.active && sScreen==PT_NONE);
    /* A Fame Checker opened from the session's Bag, however long, keeps the session and TOP through the Bag's return. */
    CtrBottom_IsFieldCallback_value=0;
    BeginSession(PT_BAG);
    for (int i=0;i<3000;++i) assert(UpdateSession(MODE_GBAVIEW,0));
    assert(sSession.active && sSession.away==0);
    assert(UpdateSession(MODE_OFF,0) && UpdateSession(MODE_BAG,0) && sSession.owner==PT_BAG);
}
''')

    def test_flash_scanline_partition_cache_and_cleanup(self):
        video = (ROOT / '3ds_port/src/3ds_video.c').read_text()
        names = ('WindowEdge', 'BattleCurtain', 'IntroWindowCurtain', 'WindowCurtain',
                 'WindowSpan', 'WindowMaskOf', 'WindowRunAdd', 'FlashWindowSpan',
                 'WindowRuns', 'CtrVideo_SetFlashWindow')
        code = '\n'.join(function(video, name) for name in names)
        reset = additions('0106-flash-mask-restore.json', 'src/field_screen_effect.c')
        reset = '\n'.join(line for line in reset.splitlines() if 'SetGpuReg(' in line)
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define CTR_GAME_WIDTH 400
#define CTR_GAME_HEIGHT 240
#define CTR_STAGE_X 80
#define CTR_STAGE_Y 40
#define VIEW_LEFT 0
#define VIEW_RIGHT (sNative ? 240 : 400)
#define VIEW_TOP 0
#define VIEW_BOTTOM (sNative ? 160 : 240)
static bool sIntro,sStage,sBattle,sNative,sFieldBarnDoorWipe,sBattleTransition;
static bool sFieldLayers=1,sWindowCacheValid,sFieldMoveShowMon;
static float sZoom=1;
static uint16_t regs[48],sFlashWindow[160];
static unsigned sFlashWindowCount;
static bool sObjWindowRow[240];
static uint8_t sObjWindow[240][400];
static unsigned Reg(unsigned offset) { return regs[offset/2]; }
static unsigned Min(unsigned a,unsigned b) { return a<b?a:b; }
static bool NativeIntroActive(void) { return false; }
static bool NativeIntroCinematic(void) { return false; }
static bool NativeIntroTitle(void) { return false; }
typedef struct { bool on[2],objOn; int x0[2],x1[2],y0[2],y1[2]; unsigned inside[2],obj,outside; } WindowSpans;
typedef struct { int x0,x1; unsigned mask; } WindowRun;
#define REG_OFFSET_WIN0H 0x40
#define REG_OFFSET_WIN0V 0x44
#define WIN_RANGE(a,b) (((a)<<8)|(b))
static void SetGpuReg(unsigned offset,unsigned value) { regs[offset/2]=value; }
''' + code + '''
int main(void) {
    WindowSpans spans={.on={true,false},.x0={40,0},.x1={400,0},.y1={240,0},.inside={63,0},.outside=1};
    uint16_t bounds[160]={0}; WindowRun runs[401];
    /* A dark cave's first row is closed, but its middle is open. */
    for (int y=60;y<100;++y) bounds[y]=WIN_RANGE(80,160);
    sWindowCacheValid=true; CtrVideo_SetFlashWindow(bounds,160);
    assert(!sWindowCacheValid);
    assert(WindowRuns(0,&spans,0,400,runs)==1 && runs[0].mask==1);
    assert(WindowRuns(120,&spans,0,400,runs)==3);
    assert(runs[1].mask==63 && runs[1].x0==133 && runs[1].x1==267);
    sWindowCacheValid=true; CtrVideo_SetFlashWindow(bounds,160);
    assert(sWindowCacheValid);
    bounds[80]=0x00ff; CtrVideo_SetFlashWindow(bounds,160);
    assert(!sWindowCacheValid);
    assert(WindowRuns(120,&spans,0,400,runs)==1 && runs[0].mask==63);
    /* A different scene must ignore a lingering Flash snapshot. */
    sFieldLayers=false;
    assert(WindowRuns(120,&spans,0,400,runs)==2 && runs[0].mask==1);
    sFieldLayers=true; sWindowCacheValid=true;
    sObjWindowRow[120]=true; sObjWindow[120][0]=1;
    /* Disabled OBJ windows cannot hide cave pixels. */
    assert(WindowRuns(120,&spans,0,400,runs)==1 && runs[0].mask==63);
    CtrVideo_SetFlashWindow(NULL,0); assert(!sWindowCacheValid && !sFlashWindowCount);
''' + reset + '''
    int first,last;
    WindowSpan(Reg(0x40),false,WindowCurtain(0),&first,&last);
    assert(first==0 && last==400);
    WindowSpan(Reg(0x44),true,false,&first,&last);
    assert(first==0 && last==240);
    sFieldMoveShowMon=true;
    WindowSpan(0x2878,true,false,&first,&last); assert(first==80 && last==160);
    WindowSpan(0x00f1,false,false,&first,&last); assert(first==0 && last==400);
    WindowSpan(0xf0f1,false,false,&first,&last); assert(first==last);
    WindowSpan(0xffff,false,false,&first,&last); assert(first==last);
    WindowSpan(0x00a1,true,false,&first,&last); assert(first==0 && last==240);
    sFieldMoveShowMon=false;
    /* Cave preview clips to the GBA stage; field geometry returns afterward. */
    sNative=true; sFieldLayers=false;
    WindowSpan(0x00ff,false,false,&first,&last); assert(last-first==240);
    sNative=false; sFieldLayers=true;
    WindowSpan(0x00ff,false,false,&first,&last); assert(last-first==400);
}
''')

    def test_summary_turns_on_press(self):
        """The owner found the Summary slow to turn pages by touch, and MOVES <-> TRAINER slow on the D-pad: both acted
        on release. The previous / next Pokemon and the page tabs act on the touch's press (BACK, a move, the IV / EV
        switch still on release, so sliding off cancels them); RIGHT on MOVES and LEFT on TRAINER act on the key's
        press, and the plan they start is not dropped for that key being held (a key pressed after it still drops
        it)."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        names = ['SumPlanClear', 'SumPlanStart', 'SumPlanPress', 'RunSumPlan', 'ProcessSummaryTouch']
        code = '\n'.join(function(bottom, n) for n in names)
        start = bottom.index('static struct {\n    int kind;\n    int target;')
        plan = bottom[start:bottom.index('} sSumPlan;', start) + len('} sSumPlan;')]
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
#include "3ds_summary.h"
#define K_A (1u << 0)
#define K_B (1u << 1)
#define K_RIGHT (1u << 4)
#define K_LEFT (1u << 5)
#define K_UP (1u << 6)
enum { HIT_NONE = 0xFF, HIT_PANEL = 0x50 };
enum { SUMPLAN_NONE = 0, SUMPLAN_PAGE, SUMPLAN_MOVE, SUMPLAN_PRESS };
#define SUMMARY_MOVES 2
#define SUMMARY_MOVES_INFO 3
static uint16_t sInjected;
static CtrInput sIn;
const CtrInput *CtrInput_Get(void) { return &sIn; }
static int page = SUI_PAGE_MOVES, asked = -1, acted = -1, hitCode = -1;
void CtrSummary_AskMove(int row) { (void)row; }
void CtrSummary_AskPage(int p) { asked = p; }
int CtrBottomSummary_Page(void) { return page; }
int CtrBottomSummary_Mon(void) { return 0; }
unsigned CtrBottomSummary_Serial(void) { return 1; }
int CtrBottomSummary_Ready(void) { return 1; }
int CtrBottomSummary_MoveCursor(void) { return 0; }
int CtrSummaryUi_Hit(const CtrSummaryUiView *v, int x, int y) { (void)v; (void)x; (void)y; return hitCode; }
static struct { CtrSummaryUiView sum; unsigned char summaryUi; } sShown;
static int sHaveShown = 1;
static struct { int down, inside, btn, trainer, ivView; } sSum;
static void SummaryAct(int hit) { acted = hit; }
''' + plan + '\n' + code + '''
static void Frame(void) { ProcessSummaryTouch(&sIn); RunSumPlan(); }
int main(void) {
    sShown.summaryUi = 1;
    /* RIGHT on MOVES: TRAINER on the press, the plan asks FireRed for INFO while RIGHT stays held */
    sIn.down = sIn.held = K_RIGHT; Frame();
    assert(sSum.trainer == 1 && sSumPlan.kind == SUMPLAN_PAGE && asked == SUI_PAGE_INFO);
    sIn.down = 0; Frame();                      /* still held: the plan goes on (its release frame) */
    assert(sSumPlan.kind == SUMPLAN_PAGE);
    sIn.held = K_RIGHT | K_A; sIn.down = K_A; Frame();
    assert(sSumPlan.kind == SUMPLAN_NONE);      /* a key pressed after it drops it */
    /* LEFT on TRAINER (FireRed on INFO): MOVES on the press */
    page = SUI_PAGE_INFO; sShown.sum.trainer = 1; sIn.held = sIn.down = K_LEFT; asked = -1; Frame();
    assert(sSum.trainer == 0 && asked == SUI_PAGE_MOVES);
    SumPlanClear(); sIn.held = sIn.down = 0; Frame();
    /* a page tab and the next Pokemon act on the touch's press */
    hitCode = SUI_HIT_TAB + 1; sIn.touchActive = sIn.touchDown = 1; acted = -1; Frame();
    assert(acted == SUI_HIT_TAB + 1);
    sIn.touchDown = 0; acted = -1; Frame(); sIn.touchActive = 0; sIn.touchUp = 1; Frame();
    assert(acted == -1);                        /* once: not again on release */
    sIn.touchUp = 0;
    hitCode = SUI_HIT_NEXT; sIn.touchActive = sIn.touchDown = 1; Frame();
    assert(acted == SUI_HIT_NEXT);
    sIn.touchDown = 0; sIn.touchActive = 0; Frame();
    /* BACK on release, and not when the touch slid off */
    hitCode = SUI_HIT_BACK; acted = -1; sIn.touchActive = sIn.touchDown = 1; Frame();
    assert(acted == -1);
    sIn.touchDown = 0; sIn.touchActive = 0; Frame();
    assert(acted == SUI_HIT_BACK);
    acted = -1; sIn.touchActive = sIn.touchDown = 1; Frame();
    hitCode = -1; sIn.touchDown = 0; Frame(); sIn.touchActive = 0; Frame();
    assert(acted == -1);
    return 0;
}
''')


if __name__ == '__main__':
    unittest.main()
