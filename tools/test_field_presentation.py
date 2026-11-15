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
    /* a touch press on a tab or NEXT while a physical key is held does nothing: no act, no plan */
    sIn.held = K_UP; sIn.down = 0; sIn.touchActive = sIn.touchDown = 1;
    hitCode = SUI_HIT_TAB + 1; acted = -1; Frame();
    assert(acted == -1 && sSumPlan.kind == SUMPLAN_NONE);
    hitCode = SUI_HIT_NEXT; Frame();
    assert(acted == -1 && sSumPlan.kind == SUMPLAN_NONE);
    sIn.touchDown = sIn.touchActive = 0; sIn.held = 0;
    return 0;
}
''')

    def test_summary_page_shown_when_taken(self):
        """A page turned by a plan (an arrow, TRAINER) is shown as soon as FireRed has taken it (its page is the one
        asked, while its own flip still runs), as a page turned by FireRed's own keys is; the way there (out of the move
        details first) stays hidden. The owner found TRAINER, always a plan, much slower: the lower screen waited for the
        whole flip. FireRed's input readiness flips at the end of every page turn and is not part of the picture, so the
        page is not drawn again for it."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        nav = function(bottom, 'NavPending')
        summary = function(bottom, 'SummaryFill')
        self.run_c('''
#include <assert.h>
#include <string.h>
#include "3ds_poketouch.h"
#include "3ds_summary.h"
enum { MODE_BAG = 3, MODE_SUMMARY = 5 };
enum { BAG_BUSY = 0 };
enum { SUMPLAN_NONE = 0, SUMPLAN_PAGE, SUMPLAN_MOVE, SUMPLAN_PRESS };
static struct { int mode, battleUi, summaryUi; } sShown;
static int sHaveShown = 1;
static struct { int pocketTarget; } sBatBag;
static struct { int kind, target; } sSumPlan;
static struct { int down, inside, btn, trainer, ivView; unsigned serial; CtrSummaryView latch; } sSum;
static int page, ready;
int CtrBottomBag_InBattle(void) { return 0; }
int CtrBottomBag_Pocket(void) { return 0; }
int CtrBottomBag_State(void) { return 0; }
int CtrBottomSummary_Ready(void) { return ready; }
int CtrBottomSummary_Page(void) { return page; }
unsigned CtrBottomSummary_Serial(void) { return 1; }
void CtrBottomSummary_GetView(CtrSummaryView *v) { memset(v, 0, sizeof(*v)); v->serial = 1; v->ready = (uint8_t)ready; strcpy(v->species, "X"); }
unsigned CtrBottomSummary_IconSpecies(void) { return 1; }
''' + nav + '\n' + summary + '''
int main(void) {
    sShown.mode = MODE_SUMMARY; sShown.summaryUi = 1;
    sSumPlan.kind = SUMPLAN_PAGE; sSumPlan.target = SUI_PAGE_INFO;
    page = SUI_PAGE_MOVES; ready = 1;
    assert(NavPending(MODE_SUMMARY));               /* not taken yet */
    page = SUI_PAGE_INFO; ready = 0;
    assert(!NavPending(MODE_SUMMARY));              /* taken: shown while FireRed's flip runs */
    sSumPlan.target = SUI_PAGE_SKILLS; page = SUI_PAGE_MOVES; ready = 1;
    assert(NavPending(MODE_SUMMARY));               /* out of the move details first: the way stays hidden */
    sSumPlan.kind = SUMPLAN_NONE;
    assert(!NavPending(MODE_SUMMARY));
    {
        CtrSummaryUiView s, t;

        ready = 0; SummaryFill(&s);
        ready = 1; SummaryFill(&t);
        assert(!memcmp(&s, &t, sizeof(s)));         /* readiness is not a change that redraws the page */
    }
    return 0;
}
''')

    def test_summary_takes_input_while_its_page_slides_in(self):
        """A page a plan turned to is shown as soon as FireRed has taken it, while its flip still runs (see above): the
        next tap or key acts on what is shown. A tab, the previous / next Pokemon and the TRAINER key start their own
        plan over the one that arrived (it waits for the flip to end before it presses anything), and RIGHT on the
        moves page reaches TRAINER while FireRed's own flip to the moves page still runs. A plan still on its way (out
        of the move details first) keeps the screen's input, and a key pressed after a plan began still drops it."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        names = ['SumPlanClear', 'SumPlanStart', 'SumPlanPress', 'RunSumPlan', 'SummaryAct', 'ProcessSummaryTouch']
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
#define K_DOWN (1u << 7)
enum { HIT_NONE = 0xFF, HIT_PANEL = 0x50 };
enum { SUMPLAN_NONE = 0, SUMPLAN_PAGE, SUMPLAN_MOVE, SUMPLAN_PRESS };
#define SUMMARY_MOVES 2
#define SUMMARY_MOVES_INFO 3
static uint16_t sInjected;
static CtrInput sIn;
const CtrInput *CtrInput_Get(void) { return &sIn; }
static int page = SUI_PAGE_INFO, ready = 1, asked = -1, hitCode = -1;
void CtrSummary_AskMove(int row) { (void)row; }
void CtrSummary_AskPage(int p) { asked = p; }
int CtrBottomSummary_Page(void) { return page; }
int CtrBottomSummary_Mon(void) { return 0; }
unsigned CtrBottomSummary_Serial(void) { return 1; }
int CtrBottomSummary_Ready(void) { return ready; }
int CtrBottomSummary_MoveCursor(void) { return 0; }
int CtrSummaryUi_Hit(const CtrSummaryUiView *v, int x, int y) { (void)v; (void)x; (void)y; return hitCode; }
static struct { CtrSummaryUiView sum; unsigned char summaryUi; } sShown;
static int sHaveShown = 1;
static struct { int down, inside, btn, trainer, ivView; } sSum;
''' + plan + '\n' + code + '''
static void Frame(void) { ProcessSummaryTouch(&sIn); RunSumPlan(); }
static void Touch(int hit)
{ hitCode = hit; sIn.touchActive = sIn.touchDown = 1; Frame(); sIn.touchActive = sIn.touchDown = 0; }
int main(void) {
    sShown.summaryUi = 1;
    /* the MOVES tab on INFO: the plan asks FireRed for MOVES, which takes it (its flip runs, the page shows) */
    Touch(SUI_HIT_TAB + SUI_TAB_MOVES);
    assert(sSumPlan.kind == SUMPLAN_PAGE && sSumPlan.target == SUI_PAGE_MOVES && asked == SUI_PAGE_MOVES);
    page = SUI_PAGE_MOVES; ready = 0; Frame();
    /* STATS tapped while MOVES slides in: its own plan, pressed when the flip is over */
    Touch(SUI_HIT_TAB + SUI_TAB_STATS);
    assert(sSumPlan.kind == SUMPLAN_PAGE && sSumPlan.target == SUI_PAGE_SKILLS && asked == -1);
    ready = 1; Frame();
    assert(asked == SUI_PAGE_SKILLS);
    /* NEXT tapped while STATS slides in: DOWN once the flip is over */
    page = SUI_PAGE_SKILLS; ready = 0; Frame();
    Touch(SUI_HIT_NEXT);
    assert(sSumPlan.kind == SUMPLAN_PRESS && sInjected == 0);
    ready = 1; Frame();
    assert(sInjected == K_DOWN && sSumPlan.kind == SUMPLAN_NONE);
    sInjected = 0;
    /* RIGHT on MOVES: TRAINER; LEFT pressed as soon as TRAINER shows (FireRed's flip to INFO still runs): MOVES */
    page = SUI_PAGE_MOVES; asked = -1;
    sIn.held = sIn.down = K_RIGHT; Frame();
    assert(sSum.trainer == 1 && sSumPlan.target == SUI_PAGE_INFO && asked == SUI_PAGE_INFO);
    sIn.held = sIn.down = 0; page = SUI_PAGE_INFO; ready = 0; sShown.sum.trainer = 1; Frame();
    sIn.held = sIn.down = K_LEFT; Frame();
    assert(sSum.trainer == 0 && sSumPlan.kind == SUMPLAN_PAGE && sSumPlan.target == SUI_PAGE_MOVES && asked == -1);
    sIn.down = 0; ready = 1; Frame();                   /* LEFT still held: the plan goes on */
    assert(asked == SUI_PAGE_MOVES);
    /* RIGHT while FireRed's own RIGHT still turns STATS to MOVES (no plan): TRAINER once the flip is over */
    SumPlanClear(); sIn.held = 0; sShown.sum.trainer = 0; asked = -1;
    page = SUI_PAGE_MOVES; ready = 0;
    sIn.held = sIn.down = K_RIGHT; Frame();
    assert(sSum.trainer == 1 && sSumPlan.kind == SUMPLAN_PAGE && sSumPlan.target == SUI_PAGE_INFO && asked == -1);
    sIn.down = 0; ready = 1; Frame();
    assert(asked == SUI_PAGE_INFO);
    /* while TRAINER slides in, a key pressed after the plan began drops it, and a touch with it held does nothing */
    sIn.held = 0; page = SUI_PAGE_INFO; ready = 0; Frame();
    sIn.held = sIn.down = K_UP; Touch(SUI_HIT_TAB + SUI_TAB_STATS);
    assert(sSumPlan.kind == SUMPLAN_NONE);
    sIn.held = sIn.down = 0; ready = 1; Frame();
    /* STATS from the move details: B first; on the way (the moves page, not the one asked) a tap is not taken */
    page = SUI_PAGE_MOVES_INFO;
    Touch(SUI_HIT_TAB + SUI_TAB_STATS);
    assert(sSumPlan.kind == SUMPLAN_PAGE && sSumPlan.target == SUI_PAGE_SKILLS && sInjected == K_B);
    page = SUI_PAGE_MOVES; ready = 0; Frame();
    Touch(SUI_HIT_TAB + SUI_TAB_INFO);
    assert(sSumPlan.kind == SUMPLAN_PAGE && sSumPlan.target == SUI_PAGE_SKILLS);
    return 0;
}
''')

    def test_bridge_reports_the_destination(self):
        """FireRed moves to a new pocket only at the end of its switch's animation, and its Party's cursor pointer goes
        back to the first Pokemon while two of them slide to their places: the owner saw the Bag's list late and the
        Party's arrow go back and forth. The bridge reports where they are going: the pocket being revealed (its count,
        cursor and items are its own already), and the second Pokemon's slot while they slide."""
        name = '0262-lower-screen-shows-destination.json'
        bag = function(additions(name, 'src/item_menu.c'), 'CtrBagShownPocket')
        party = function(additions(name, 'src/party_menu.c'), 'CtrPartyShownSlot')
        self.run_c('''
#include <assert.h>
typedef unsigned char u8;
typedef signed char s8;
typedef short s16;
typedef void (*TaskFunc)(u8);
#define POCKET_ITEMS 1
#define POCKET_POKE_BALLS 3
#define tSwitchDir data[11]
enum { PARTY_ACTION_CHOOSE_MON, PARTY_ACTION_SWITCH, PARTY_ACTION_SWITCHING, PARTY_ACTION_SOFTBOILED };
static struct { TaskFunc func; s16 data[16]; } gTasks[4];
static struct { u8 pocket; } gBagMenuState;
static struct { u8 action; s8 slotId, slotId2; } gPartyMenu;
static void Task_AnimateSwitchPockets(u8 t) { (void)t; }
static void Task_Other(u8 t) { (void)t; }
static int bagOpen = 1;
static int BagTask(u8 *taskId) { *taskId = 2; return bagOpen; }
static s8 *GetCurrentPartySlotPtr(void)
{ return gPartyMenu.action == PARTY_ACTION_SWITCH ? &gPartyMenu.slotId2 : &gPartyMenu.slotId; }
''' + bag + '\n' + party + '''
int main(void) {
    gBagMenuState.pocket = 0; gTasks[2].func = Task_Other;
    assert(CtrBagShownPocket() == 0);
    gTasks[2].func = Task_AnimateSwitchPockets; gTasks[2].data[11] = 1;
    assert(CtrBagShownPocket() == 1);                   /* KEY ITEMS being revealed */
    gBagMenuState.pocket = 2; gTasks[2].data[11] = 1;
    assert(CtrBagShownPocket() == 2);                   /* never past the last pocket */
    bagOpen = 0;
    assert(CtrBagShownPocket() == 2);
    gPartyMenu.action = PARTY_ACTION_SWITCH; gPartyMenu.slotId = 1; gPartyMenu.slotId2 = 4;
    assert(CtrPartyShownSlot() == 4);                   /* choosing where to move it */
    gPartyMenu.action = PARTY_ACTION_SWITCHING;
    assert(CtrPartyShownSlot() == 4);                   /* sliding: where the cursor lands, not back on the first */
    gPartyMenu.action = PARTY_ACTION_CHOOSE_MON; gPartyMenu.slotId = 4;
    assert(CtrPartyShownSlot() == 4);
    return 0;
}
''')

    def test_party_switch_shows_the_new_places(self):
        """FireRed swaps two Pokemon's data only once both have slid out of sight (SwitchPartyMon at the end of
        Task_SlideSelectedSlotsOffscreen): the owner saw the lower screen's cards change about a second after the
        pick. While they slide out, the bridge shows each of the two slots with the Pokemon about to be there; before
        and after, every slot is its own."""
        name = '0263-party-switch-shows-new-places.json'
        shown = function(additions(name, 'src/party_menu.c'), 'CtrBottomParty_ShownMon')
        self.run_c('''
#include <assert.h>
typedef unsigned char u8;
typedef signed char s8;
typedef void (*TaskFunc)(u8);
enum { PARTY_ACTION_CHOOSE_MON, PARTY_ACTION_SWITCH, PARTY_ACTION_SWITCHING };
static struct { u8 action; s8 slotId, slotId2; } gPartyMenu;
static void Task_SlideSelectedSlotsOffscreen(u8 t) { (void)t; }
static TaskFunc active;
static int FuncIsActiveTask(TaskFunc f) { return f == active; }
''' + shown + '''
int main(void) {
    gPartyMenu.action = PARTY_ACTION_SWITCH; gPartyMenu.slotId = 1; gPartyMenu.slotId2 = 4;
    for (int i = 0; i < 6; ++i)
        assert(CtrBottomParty_ShownMon(i) == i);        /* choosing where: nothing moved yet */
    gPartyMenu.action = PARTY_ACTION_SWITCHING; active = Task_SlideSelectedSlotsOffscreen;
    assert(CtrBottomParty_ShownMon(1) == 4 && CtrBottomParty_ShownMon(4) == 1);
    assert(CtrBottomParty_ShownMon(0) == 0 && CtrBottomParty_ShownMon(5) == 5);
    active = 0;                                         /* swapped, sliding back in: every slot its own */
    assert(CtrBottomParty_ShownMon(1) == 1 && CtrBottomParty_ShownMon(4) == 4);
    return 0;
}
''')

    def test_touch_shows_where_it_goes(self):
        """The owner saw the arrow go to a touched tab or Pokemon, back to where it was, then to it again. A Party slot
        touched (the Party takes the touch on its next frame) is shown as the selection until the Party's cursor is
        there, or for a short while at most; a Bag tab's pocket is held (the way there hidden) while the Bag walks to
        it; and a pocket FireRed has just switched to is drawn at once, while it slides in (its list is already the
        pocket's own), instead of after the slide."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        fill = (ROOT / '3ds_port/src/3ds_bottom_field_fill_impl.h').read_text(encoding='utf-8')
        battle = (ROOT / '3ds_port/src/3ds_bottom_battle_fill_impl.h').read_text(encoding='utf-8')
        echo = function(bottom, 'PartyEchoCursor')
        keys = function(bottom, 'PartyEchoKeys')
        left = function(bottom, 'PartyEchoMode')
        button = function(battle, 'PartyButton')
        hold = function(bottom, 'FieldBagHold')
        self.assertIn('PartyEchoCursor(s->cursor)', function(battle, 'BattlePartyFill'))
        self.assertIn('FieldBagHold(', function(fill, 'FieldBagFill'))
        self.run_c('''
#include <assert.h>
#include <stdint.h>
#include "3ds_poketouch.h"
enum { BAG_BUSY = 0, BAG_LIST };
enum { MODE_FIELD = 1, MODE_PARTY = 2 };
#define PARTY_SIZE 6
static struct { int slot, wait; } sPartyEcho = {-1, 0};
static struct { int pocketTarget; } sFieldBag = {-1};
static int gState = BAG_LIST, gPocket = 0;
static int CtrBottomBag_State(void) { return gState; }
static int CtrBottomBag_Pocket(void) { return gPocket; }
''' + '\n'.join([echo, keys, left, button, hold]) + '''
int main(void) {
    CtrPokeTouchBag last = {0};
    /* the Party: the slot touched until the cursor gets there */
    assert(PartyEchoCursor(2) == 2);
    sPartyEcho.slot = 4; sPartyEcho.wait = 30;
    assert(PartyEchoCursor(2) == 4 && PartyEchoCursor(2) == 4);
    assert(PartyEchoCursor(4) == 4 && sPartyEcho.slot == -1);
    assert(PartyEchoCursor(1) == 1);
    sPartyEcho.slot = 5; sPartyEcho.wait = 2;
    assert(PartyEchoCursor(0) == 5 && PartyEchoCursor(0) == 0);     /* not forever */
    /* a physical button drops it; so does leaving the Party (closed, a Summary or the field took over) */
    sPartyEcho.slot = 3; sPartyEcho.wait = 30;
    PartyEchoKeys(0); assert(PartyEchoCursor(0) == 3);
    PartyEchoKeys(1u << 4); assert(sPartyEcho.slot == -1 && PartyEchoCursor(0) == 0);
    sPartyEcho.slot = 3; sPartyEcho.wait = 30;
    PartyEchoMode(MODE_PARTY); assert(PartyEchoCursor(0) == 3);
    PartyEchoMode(MODE_FIELD); assert(sPartyEcho.slot == -1 && PartyEchoCursor(0) == 0);
    /* picking several: the button follows the echoed cursor; otherwise there is none */
    sPartyEcho.slot = 6; sPartyEcho.wait = 30;
    assert(PartyButton(1, PartyEchoCursor(0)) == 0);                /* a touched CONFIRM shows at once */
    sPartyEcho.slot = 7; sPartyEcho.wait = 30;
    assert(PartyButton(1, PartyEchoCursor(0)) == 1 && PartyButton(0, 7) == -1 && PartyButton(1, 2) == -1);
    sPartyEcho.slot = -1;
    /* the Bag: hold the picture on the way to a tab's pocket; draw a switched pocket while it slides */
    last.kind = PT_BAG_KIND_BAG; last.pocket = 0;
    gState = BAG_LIST; gPocket = 1; sFieldBag.pocketTarget = 2;
    assert(FieldBagHold(&last) == 1);                               /* on the way */
    gState = BAG_BUSY; gPocket = 2; last.pocket = 1;
    assert(FieldBagHold(&last) == 0);                               /* arrived, sliding in: drawn */
    sFieldBag.pocketTarget = -1; gPocket = 1; last.pocket = 0;
    assert(FieldBagHold(&last) == 0);                               /* the D-pad switched it: drawn */
    last.pocket = 1;
    assert(FieldBagHold(&last) == 1);                               /* busy otherwise (a message printing): held */
    gState = BAG_LIST;
    assert(FieldBagHold(&last) == 0);
    return 0;
}
''')

    def test_battle_bag_holds_on_the_way_to_berries(self):
        """BERRIES opens the Berry Pouch from the Key Items pocket, which is only the way there: the battle Bag's
        picture holds while FireRed slides that pocket in. FireRed reports the pocket being revealed from the slide's
        first frame (0262), so the walk ends only when the slide is over. The walk back to Items (from the Berry Pouch)
        still ends as Items starts to slide in, so Items is drawn at once."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        start = bottom.index('static struct {\n    int pocketTarget, pocketWait;')
        state = bottom[start:bottom.index('} sBatBag = {-1, 0, 0, 0};', start) + len('} sBatBag = {-1, 0, 0, 0};')]
        names = ('NavPending', 'BattleBagVisiblePocketStep', 'ProcessBattleBagTouch')
        code = '\n'.join(function(bottom, n) for n in names)
        self.run_c('''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
#define K_A (1u << 0)
#define K_B (1u << 1)
#define K_RIGHT (1u << 4)
#define K_LEFT (1u << 5)
enum { MODE_SUMMARY = 3, MODE_BAG = 4 };
enum { BAG_BUSY = 0, BAG_LIST, BAG_ACTIONS, BAG_YESNO, BAG_QUANTITY, BAG_MESSAGE };
enum { BAGPLAN_NONE = 0, BAGPLAN_LIST, BAGPLAN_MENU, BAGPLAN_PRESS };
enum { SUMPLAN_NONE = 0, SUMPLAN_PAGE, SUMPLAN_MOVE, SUMPLAN_PRESS };
typedef struct { int (*cursor)(void); } ListScreen;
static int Cursor(void) { return 0; }
static const ListScreen sBagScreen = {Cursor};
static struct { int mode, battleUi, summaryUi; CtrBattleView battle; } sShown;
static int sHaveShown = 1;
static struct { int kind, target; } sSumPlan;
static struct { int kind, target, activate; uint16_t keys; } sBagPlan;
static struct { int down, inside, btn, state; } sBat;
static int sBatTouchLast;
static int gPocket, gState = BAG_LIST, hitCode = -1;
int CtrBottomBag_InBattle(void) { return 1; }
int CtrBottomBag_Pocket(void) { return gPocket; }
int CtrBottomBag_State(void) { return gState; }
int CtrBottomBag_Count(void) { return 2; }
unsigned CtrBottomBag_Item(int index) { return index == 1 ? 99u : 7u; }
unsigned CtrBattle_BerryPouch(void) { return 99u; }
int CtrBottomSummary_Page(void) { return 0; }
uint16_t CtrBottom_FilterGameKeys(uint16_t held) { return held; }
int CtrBattleUi_Hit(const CtrBattleView *v, int x, int y) { (void)v; (void)x; (void)y; return hitCode; }
int CtrBattleUi_BagRailTarget(const CtrBattleBag *b, int y) { (void)b; (void)y; return -1; }
static void BagPlanClear(void) { memset(&sBagPlan, 0, sizeof(sBagPlan)); }
static void BagPlanStart(const ListScreen *list, int kind, int target, int activate, uint16_t keys)
{ (void)list; sBagPlan.kind = kind; sBagPlan.target = target; sBagPlan.activate = activate; sBagPlan.keys = keys; }
''' + state + '\n' + code + '''
int main(void) {
    CtrInput in;

    memset(&in, 0, sizeof(in));
    sShown.mode = MODE_BAG; sShown.battleUi = 1; sShown.battle.state = BUI_BAG;
    sShown.battle.bag.bagState = BAG_LIST; sShown.battle.bag.berries = 99;
    /* BERRIES (press, release): the Key Items pocket is walked to */
    hitCode = BUI_BAG_BERRIES; in.touchActive = in.touchDown = 1; ProcessBattleBagTouch(&in);
    hitCode = -1; in.touchActive = in.touchDown = 0; ProcessBattleBagTouch(&in);
    assert(sBatBag.pocketTarget == 1 && sBatBag.berryWait > 0);
    ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_PRESS && sBagPlan.keys == K_RIGHT && NavPending(MODE_BAG));
    /* FireRed slides Key Items in (and reports it at once): the picture holds */
    BagPlanClear(); gPocket = 1; gState = BAG_BUSY;
    ProcessBattleBagTouch(&in);
    assert(sBatBag.pocketTarget == 1 && NavPending(MODE_BAG));
    ProcessBattleBagTouch(&in);
    assert(sBatBag.pocketTarget == 1 && NavPending(MODE_BAG));
    /* the slide is over: the walk ends and the Berry Pouch is chosen */
    gState = BAG_LIST; ProcessBattleBagTouch(&in);
    assert(sBatBag.pocketTarget == -1);
    ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == 1 && sBagPlan.activate);
    /* back from the Berry Pouch, in the Key Items pocket: walked to Items, which is drawn as it slides in */
    BagPlanClear(); sBatBag.autoWait = sBatBag.berryWait = 0;
    ProcessBattleBagTouch(&in);
    assert(sBatBag.pocketTarget == 0);
    ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_PRESS && sBagPlan.keys == K_LEFT);
    BagPlanClear(); gPocket = 0; gState = BAG_BUSY;
    ProcessBattleBagTouch(&in);
    assert(sBatBag.pocketTarget == -1 && !NavPending(MODE_BAG));
    return 0;
}
''')


if __name__ == '__main__':
    unittest.main()
