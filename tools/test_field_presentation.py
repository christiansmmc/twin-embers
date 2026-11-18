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
static int sScreen,sStorageSession;
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
        # The eight Bag getters (count, rows shown, cursor, serial, item, pocket name, pocket, quantity) answer for the
        # pocket shown, and the Party's active slot is the slot shown.
        self.assertEqual(additions(name, 'src/item_menu.c').count('CtrBagShownPocket()'), 8)
        self.assertEqual(additions(name, 'src/party_menu.c').count('return CtrPartyShownSlot();'), 1)
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
        # The lower screen's Party reads each slot through the Pokemon shown there (its ten call sites).
        self.assertEqual(additions(name, 'src/ctr_bottom_party.c').count('CtrBottomParty_ShownMon(slot)'), 10)
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

    def test_party_extras_switch(self):
        """The owner's call: HEAL and REMEMBER (recipes 0235, 0238) leave the Party's action menu behind one compile-time
        switch, CTR_PARTY_EXTRAS 0; their code stays (CursorCB_Heal, CursorCB_Relearn, the texts, the dispatch by id), so
        1 brings them back. The two adds move into a helper that, like its one call, is compiled only with the switch."""
        text = additions('0264-party-extras-switch.json', 'src/party_menu.c')
        self.assertIn('#define CTR_PARTY_EXTRAS 0', text)
        helper = function(text, 'CtrAppendPartyExtras')
        self.assertIn('CURSOR_OPTION_RELEARN);', helper)
        self.assertIn('CURSOR_OPTION_HEAL);', helper)
        blocks = re.findall(r'#if CTR_PARTY_EXTRAS\n(.*?)#endif', text, re.S)
        self.assertTrue(any('static void CtrAppendPartyExtras(void)' in b for b in blocks))   # the helper: only with the switch
        self.assertTrue(any('CtrAppendPartyExtras();' in b for b in blocks))                   # its one call, likewise
        doc = json.loads((ROOT / 'patches/pokefirered/0264-party-extras-switch.json').read_text())
        self.assertEqual([f['path'] for f in doc['files']], ['src/party_menu.c'])

    def test_summary_reports_the_nature(self):
        """The owner's call (decision 2): the B Summary's STATS marks the stat the nature raises and the one it lowers, so
        the bridge reports the nature's number beside its name, read from the Pokemon shown (recipe 0265, the summary
        screen's file only); the lower screen's view holds it right after the name."""
        name = '0265-summary-reports-nature.json'
        self.assertIn('out->natureId = GetNature(&screen->currentMon);', additions(name, 'src/pokemon_summary_screen.c'))
        doc = json.loads((ROOT / 'patches/pokefirered' / name).read_text())
        self.assertEqual([f['path'] for f in doc['files']], ['src/pokemon_summary_screen.c'])
        header = (ROOT / '3ds_port/include/3ds_summary.h').read_text(encoding='utf-8')
        self.assertRegex(header, r'char nature\[12\];\s*unsigned char natureId;')

    def test_summary_reports_contact(self):
        """The Summary's move details say whether the move makes contact (def_sum_moves): the bridge reports a bit per move,
        read from the move's own flags (recipe 0266, the summary screen's file only), after the ribbons."""
        name = '0266-summary-reports-contact.json'
        self.assertIn('if (gBattleMoves[out->moves[i]].flags & FLAG_MAKES_CONTACT) out->contact |= (unsigned char)(1u << i);',
                      additions(name, 'src/pokemon_summary_screen.c'))
        doc = json.loads((ROOT / 'patches/pokefirered' / name).read_text())
        self.assertEqual([f['path'] for f in doc['files']], ['src/pokemon_summary_screen.c'])
        self.assertRegex((ROOT / '3ds_port/include/3ds_summary.h').read_text(encoding='utf-8'), r'unsigned char ribbons;\s*unsigned char contact;')

    def test_choice_prices_split(self):
        """A priced multichoice row (FireRed's "FRESH WATER{CLEAR_TO 0x57}{FONT_SMALL}¥200", the Game Corner's
        "{FONT_SMALL} 50 COINS{CLEAR_TO 0x45}¥1,000") reaches the lower screen as its label and its price (recipe 0267)."""
        name = '0267-field-choice-prices.json'
        added = additions(name, 'src/menu.c')
        self.assertIn('void CtrFieldChoice_Price(int index, char *out, int size)', added)
        self.assertEqual([f['path'] for f in json.loads((ROOT / 'patches/pokefirered' / name).read_text())['files']], ['src/menu.c'])
        self.run_c('''#include <assert.h>
#include <string.h>
typedef unsigned char u8;
#define EOS 0xFF
#define EXT_CTRL_CODE_BEGIN 0xFC
#define EXT_CTRL_CODE_CLEAR_TO 0x13
static u8 GetExtCtrlCodeLength(u8 code) { return code == 0x06 || code == 0x13 ? 2 : 1; }
''' + function(added, 'CtrFieldChoice_Split') + '''
int main(void) {
    static const u8 a[] = {0xC0, 0xC1, 0xFC, 0x13, 0x57, 0xFC, 0x06, 0x00, 0xA1, 0xA2, 0xFF};
    static const u8 b[] = {0xFC, 0x06, 0x00, 0x00, 0xC0, 0xFC, 0x13, 0x45, 0xA1, 0xFF}, c[] = {0xC0, 0xFF};
    u8 l[8], p[8];
    CtrFieldChoice_Split(a, l, p, 8); assert(!memcmp(l, "\\xC0\\xC1\\xFF", 3) && !memcmp(p, "\\xA1\\xA2\\xFF", 3));
    CtrFieldChoice_Split(b, l, p, 8); assert(!memcmp(l, "\\xC0\\xFF", 2) && !memcmp(p, "\\xA1\\xFF", 2));
    CtrFieldChoice_Split(c, l, p, 8); assert(!memcmp(l, "\\xC0\\xFF", 2) && p[0] == 0xFF);
    CtrFieldChoice_Split(a, l, p, 2); assert(l[0] == 0xC0 && l[1] == 0xFF);
    return 0; }''')

    def test_device_drawers_write_non_ascii_as_escapes(self):
        """A string literal in a device drawer holds ASCII and backslash escapes only (UTF-8 spelled as bytes, as the codebase
        does): a raw non-ASCII character there is saved as other bytes and draws the wrong letters (ES YES was "S" plus two
        garbage characters). Comments may hold any text."""
        found = []
        for path in sorted((ROOT / '3ds_port/src').glob('3ds_device_*_impl.h')):
            text = path.read_text(encoding='utf-8')
            text = re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', ' ', m.group(0)), text, flags=re.S)
            for n, line in enumerate(text.split('\n'), 1):
                for lit in re.findall(r'"(?:[^"\\n]|\.)*"', line):
                    if any(ord(c) > 127 for c in lit):
                        found.append('%s:%d' % (path.name, n))
        self.assertEqual(found, [])

    def test_party_menu_marks(self):
        """The B Party's action menu: a field move's name in blue (FireRed's own colour for them, ids from
        CURSOR_OPTION_FIELD_MOVES on), CANCEL a dark key (CURSOR_OPTION_CANCEL1 or _CANCEL2); bit i for row i. The ids
        are party_menu.h's, named the same in the fill."""
        battle = (ROOT / '3ds_port/src/3ds_bottom_battle_fill_impl.h').read_text(encoding='utf-8')
        ids = '\n'.join(re.findall(r'#define CURSOR_OPTION_\w+ +\d+', battle))
        self.assertIn('PartyMenuMarks(', function(battle, 'BattlePartyFill'))
        self.run_c('''
#include <assert.h>
#include <stdint.h>
''' + ids + '\n' + function(battle, 'PartyMenuMarks') + '''
int main(void)
{
    static const unsigned char a[] = {0, 1, 3, 19, 20, 2}, c[] = {10, 0, 2};
    uint8_t b = 0xff, d = 0xff;

    assert(CURSOR_OPTION_CANCEL1 == 2 && CURSOR_OPTION_CANCEL2 == 9 && CURSOR_OPTION_FIELD_MOVES == 18);
    PartyMenuMarks(a, 6, &b, &d);
    assert(b == 0x18 && d == 0x20);
    PartyMenuMarks(c, 3, &b, &d);
    assert(b == 0 && d == 0x04);
    return 0;
}
''')

    def test_party_names(self):
        """The B Party shows a Pokemon's name in capitals when it is its species' own (the battle's rule), as typed when
        the player gave it; the answer is kept per slot while its species and name stay (the fill runs every frame)."""
        battle = (ROOT / '3ds_port/src/3ds_bottom_battle_fill_impl.h').read_text(encoding='utf-8')
        species = re.search(r'#define BAT_SPECIES \d+', battle).group(0)
        code = '\n'.join(function(battle, n) for n in ('BatUpperByte', 'BatSameName', 'BatNameIsSpecies', 'BatCopy',
                                                     'PartyNameIsSpecies'))
        self.assertIn('PartyNameIsSpecies(', function(battle, 'BattlePartyFill'))
        self.run_c('''
#include <assert.h>
#include <string.h>
#include <stdint.h>
static int calls;
static void CtrBottomDex_Name(unsigned species, char *out, int size)
{
    (void)size;
    ++calls;
    strcpy(out, species == 5 ? "Charmeleon" : species == 17 ? "Pidgeotto" : "");
}
''' + species + '\n' + code + '''
int main(void)
{
    assert(PartyNameIsSpecies(0, "CHARMELEON", 5) && calls == 1);
    assert(PartyNameIsSpecies(0, "CHARMELEON", 5) && calls == 1);       /* kept */
    assert(!PartyNameIsSpecies(0, "Sparky", 5) && calls == 2);          /* renamed: asked again */
    assert(!PartyNameIsSpecies(0, "Sparky", 5) && calls == 2);
    assert(!PartyNameIsSpecies(0, "CHARMELEON", 17) && calls == 3);     /* another Pokemon in the slot */
    assert(PartyNameIsSpecies(1, "Pidgeotto", 17) && calls == 4);       /* each slot its own */
    assert(!PartyNameIsSpecies(0, "CHARMELEON", 17) && calls == 4);
    return 0;
}
''')

    def test_party_fill_copies_gender(self):
        """The B Party's bays show each Pokemon's gender sign (def_party): the fill copies the Party snapshot's (0 male,
        1 female, 2 none, CtrBottomParty_Gender) beside the name."""
        fill = (ROOT / '3ds_port/src/3ds_bottom_battle_fill_impl.h').read_text(encoding='utf-8')
        self.assertIn('p->gender[i] = s->gender[i];', function(fill, 'BattlePartyFill'))

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
        self.assertIn('PartyButton(', function(battle, 'BattlePartyFill'))
        self.assertIn('PartyEchoKeys(', function(bottom, 'ProcessBattlePartyTouch'))
        self.assertIn('FieldBagHold(', function(fill, 'FieldBagFill'))
        touch = function(bottom, 'ProcessTouch')
        # The Party forgets the touched slot when it is left, between two screens too (the sBridging return).
        self.assertLess(touch.index('PartyEchoMode(mode);'), touch.index('if (sBridging)'))
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
    /* the button follows the echoed cursor: CONFIRM picking several, CANCEL always (the B Party sinks the key the
     * cursor is on); a slot is none */
    sPartyEcho.slot = 6; sPartyEcho.wait = 30;
    assert(PartyButton(1, PartyEchoCursor(0)) == 0);                /* a touched CONFIRM shows at once */
    sPartyEcho.slot = 7; sPartyEcho.wait = 30;
    assert(PartyButton(1, PartyEchoCursor(0)) == 1 && PartyButton(0, 7) == 1 && PartyButton(1, 2) == -1);
    assert(PartyButton(0, 6) == -1 && PartyButton(0, 2) == -1);     /* one at a time: no CONFIRM */
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

    def run_battle_bag(self, main):
        """ProcessBattleBagTouch, the state it keeps and the drag's numbers, as 3ds_bottom.c has them, over a synthetic
        battle Bag (the Bag's state, pocket and cursor, the hit under the finger, the plans it starts); `main` drives it."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        start = bottom.index('static struct {\n    int pocketTarget, pocketWait;')
        state = bottom[start:bottom.index(';', bottom.index('} sBatBag = {', start)) + 1]
        drag = '\n'.join(re.search(r'#define ' + n + r' \d+', bottom).group(0) for n in ('LIST_DRAG_SLOP', 'BAG_DRAG_ROW'))
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
static int gCursor;
static int Cursor(void) { return gCursor; }
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
static void BagPlanClear(void) { memset(&sBagPlan, 0, sizeof(sBagPlan)); }
static void BagPlanStart(const ListScreen *list, int kind, int target, int activate, uint16_t keys)
{ (void)list; sBagPlan.kind = kind; sBagPlan.target = target; sBagPlan.activate = activate; sBagPlan.keys = keys; }
''' + drag + '\n' + state + '\n' + code + main)

    def test_dex_list_drags_a_row_per_tall_row(self):
        """The Pokedex's list on the red device has tall rows (DevTallRow, 3ds_device_layout.h): a finger dragged on it
        moves the cursor one Pokemon per LIST_DRAG_ROW pixels, the rows' own pitch, so the list follows the finger."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        layout = (ROOT / '3ds_port/include/3ds_device_layout.h').read_text(encoding='utf-8')
        pitch = int(re.search(r'DevTallRow\(int i, int narrow\)\s*\{\s*return DevListRow\(i, narrow, (\d+)\);', layout).group(1))
        self.assertEqual(int(re.search(r'#define LIST_DRAG_ROW (\d+)', bottom).group(1)), pitch)
        self.assertIn('/ LIST_DRAG_ROW', function(bottom, 'ProcessFieldDexTouch'))

    def test_bag_list_drags_a_row_per_grid_row(self):
        """The Bag-page lists' rows are the grid's, 26 px apart (3ds_device_layout.h): a drag moves one item per row."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        layout = (ROOT / '3ds_port/include/3ds_device_layout.h').read_text(encoding='utf-8')
        pitch = int(re.search(r'#define DEV_GRID_ROW_PITCH (\d+)', layout).group(1))
        self.assertEqual(int(re.search(r'#define BAG_DRAG_ROW (\d+)', bottom).group(1)), pitch)

    def run_relearn(self, main):
        """RelearnFill and ProcessRelearnTouch, their state and the game's answers as 3ds_bottom_relearn_impl.h declares
        them, over a synthetic relearner on its list: gCount moves (the list has one more row, FireRed's CANCEL, index
        gCount), the cursor at gCursor, the hit under the finger gHit; `main` drives it."""
        relearn = (ROOT / '3ds_port/src/3ds_bottom_relearn_impl.h').read_text(encoding='utf-8')
        externs = '\n'.join(re.findall(r'^extern [^;]+;', relearn, re.M))
        start = relearn.index('static struct {\n    int down, inside, code;')
        state = relearn[start:relearn.index('\n', relearn.index('static int sRelearnFirst;'))]
        code = function(relearn, 'RelearnFill') + '\n' + function(relearn, 'ProcessRelearnTouch')
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
#define K_A (1u << 0)
#define K_B (1u << 1)
#define K_UP (1u << 6)
#define K_DOWN (1u << 7)
enum { MODE_GBAVIEW = 18 };
static uint16_t sRouteKey;
static int sHaveShown = 1;
static struct { int mode; CtrPokeTouchView touch; } sShown;
static int gCount = 8, gCursor = 1, gHit = -1;
extern int CtrBottom_IsFading(void);
int CtrBottom_IsFading(void) { return 0; }
''' + externs + '''
int CtrBottomRelearn_IsOpen(void) { return 1; }
int CtrBottomRelearn_Panel(void) { return PT_RL_PANEL_LIST; }
int CtrBottomRelearn_Input(void) { return 1; }
int CtrBottomRelearn_State(void) { return 0; }
int CtrBottomRelearn_MenuCursor(void) { return 0; }
void CtrBottomRelearn_Message(char *out, int size) { (void)size; out[0] = 0; }
int CtrBottomRelearn_Count(void) { return gCount; }
int CtrBottomRelearn_Cursor(void) { return gCursor; }
unsigned CtrBottomRelearn_Species(void) { return 1; }
void CtrBottomRelearn_Name(char *nickname, int nsize, char *level, int lsize)
{ (void)nsize; (void)lsize; nickname[0] = level[0] = 0; }
void CtrBottomRelearn_Question(char *out, int size) { (void)size; out[0] = 0; }
unsigned CtrBottomRelearn_Row(int i, int *type, char *name, int nsize, char *pp, int psize, char *power, int wsize,
                              char *accuracy, int asize)
{
    (void)nsize; (void)psize; (void)wsize; (void)asize;
    name[0] = pp[0] = power[0] = accuracy[0] = 0;
    *type = i < gCount ? 0 : -1;
    return i < gCount ? 100u + (unsigned)i : 0u;     /* FireRed's CANCEL row: no move */
}
void CtrBottomRelearn_Description(unsigned move, char *out, int size) { (void)move; (void)size; out[0] = 0; }
void CtrBottomRelearn_TypeName(int type, char *out, int size) { (void)type; (void)size; out[0] = 0; }
int CtrPokeTouch_RelearnHit(const CtrPokeTouchView *view, int x, int y) { (void)view; (void)x; (void)y; return gHit; }
''' + state + '\n' + code + '''
/* What the screen shows now (RelearnFill), as the frame before the touch showed it. */
static void Shown(void)
{
    memset(&sShown, 0, sizeof(sShown));
    sShown.mode = MODE_GBAVIEW;
    RelearnFill(&sShown.touch.relearn, -1);
}
/* A tap on `hit`: down, held, released. */
static __attribute__((unused)) void Tap(int hit)
{
    CtrInput in;

    gHit = hit;
    memset(&in, 0, sizeof(in));
    in.touchActive = in.touchDown = 1; in.touchX = 10; in.touchY = 10;
    ProcessRelearnTouch(&in);
    in.touchDown = 0; ProcessRelearnTouch(&in);
    in.touchActive = 0; ProcessRelearnTouch(&in);
}
''' + main)

    def test_relearn_cancel_sends_b(self):
        """The relearner's CANCEL is a key of its own on the red device (owner decision 4: PT_RL_HIT_CANCEL, 235, in the
        Bag's CLOSE place), FireRed's CANCEL row no longer drawn as a row. One tap on it is FireRed's B on the list
        (owner decision, 2026-11-18), which asks the same give-up question as A on that row and is enabled while the list
        takes keys (RelearnSnapshot): wherever the cursor is, B once, and no walk. A row is still walked to, and the
        cursor's row taken with A. (The relearn suite of device_render_test.c pins where CANCEL is hit.)"""
        self.run_relearn('''
int main(void)
{
    gCursor = 1; Shown();
    Tap(PT_RL_HIT_CANCEL); assert(sRouteKey == K_B && !sRelearnPlan.active);           /* the cursor on a move: B, once */
    sRouteKey = 0; Tap(-1); assert(!sRouteKey && !sRelearnPlan.active);
    gCursor = gCount; Shown(); Tap(PT_RL_HIT_CANCEL); assert(sRouteKey == K_B && !sRelearnPlan.active);   /* on CANCEL */
    sRouteKey = 0; gCursor = 1; Shown(); assert(sShown.touch.relearn.first == 1);      /* the window back up to it */
    Tap(PT_RL_HIT_ROW + 0); assert(sRouteKey == K_A && !sRelearnPlan.active);           /* the cursor's row: A */
    sRouteKey = 0; Tap(PT_RL_HIT_ROW + 3); assert(!sRouteKey && sRelearnPlan.active && sRelearnPlan.target == 4);
    return 0;
}
''')

    def test_relearn_window_ends_at_the_last_move(self):
        """The list's window of PT_RL_ROWS rows follows the cursor and ends at the last move: FireRed's CANCEL row is the
        CANCEL key, not a row, so a window past the last move would leave its last row blank. With the cursor on the
        last move or on CANCEL the window is the last PT_RL_ROWS moves; a list shorter than that starts at its top."""
        self.run_relearn('''
int main(void)
{
    gCount = 8; gCursor = 7; Shown(); assert(sShown.touch.relearn.first == 3);
    for (int i = 0; i < PT_RL_ROWS; ++i) assert(sShown.touch.relearn.row[i].move == 103u + (unsigned)i);
    gCursor = 8; Shown(); assert(sShown.touch.relearn.first == 3);                      /* the cursor on CANCEL */
    gCursor = 0; Shown(); assert(sShown.touch.relearn.first == 0);
    gCursor = 5; Shown(); assert(sShown.touch.relearn.first == 1);
    gCount = 5; gCursor = 5; Shown(); assert(sShown.touch.relearn.first == 0);
    gCount = 3; gCursor = 3; Shown(); assert(sShown.touch.relearn.first == 0 && sShown.touch.relearn.row[2].move == 102u);
    return 0;
}
''')

    def test_battle_bag_drags_its_rows(self):
        """The battle Bag's rows drag as the field Bag's do (v0.6.0's battle Bag scrolled by touch, its rail): a finger
        on a row that moves LIST_DRAG_SLOP pixels up or down scrolls instead of tapping, the cursor following it one item
        per BAG_DRAG_ROW pixels from where it was at the touch-down, walked by one Bag plan at a time, kept within the
        items (no CLOSE row in the battle's list); a drag never also selects on its release, and a tap still acts once."""
        self.run_battle_bag('''
int main(void) {
    CtrInput in;
    CtrBattleBag *b = &sShown.battle.bag;

    memset(&in, 0, sizeof(in));
    sShown.mode = MODE_BAG; sShown.battleUi = 1; sShown.battle.state = BUI_BAG;
    b->bagState = BAG_LIST; b->count = 7; b->cursor = 0; b->first = 0;          /* a long Items pocket */
    /* a finger on row 2; within the slop it is still a tap */
    hitCode = BUI_BAG_ROW + 2; in.touchActive = in.touchDown = 1; in.touchY = 150; ProcessBattleBagTouch(&in);
    in.touchDown = 0; in.touchY = 150 - (LIST_DRAG_SLOP - 1); ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_NONE && sBat.down && sBat.inside);
    /* past it: the rows follow the finger (up: further down the list), one item per BAG_DRAG_ROW pixels */
    in.touchY = 150 - 2 * BAG_DRAG_ROW; hitCode = BUI_BAG_ROW; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == 2 && !sBagPlan.activate && !sBat.inside);
    /* one plan at a time: none over a plan still walking */
    in.touchY = 150 - 4 * BAG_DRAG_ROW; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == 2);
    BagPlanClear(); gCursor = 2; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == 4);
    /* far past the end: the last item (count - 1), not past it */
    BagPlanClear(); gCursor = 4; in.touchY = 0; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == b->count - 1);
    /* back near where it began: still a drag, to the touch-down's item; past the top, the first */
    BagPlanClear(); gCursor = 6; in.touchY = 150 + 3; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == 0 && !sBat.inside);
    BagPlanClear(); gCursor = 0; in.touchY = 239; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_NONE);
    /* released: a drag selects nothing */
    hitCode = BUI_BAG_ROW + 2; in.touchActive = 0; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_NONE && !sBat.down && sBatBag.autoWait == 0);
    /* a tap on another row: walked to once, not taken */
    hitCode = BUI_BAG_ROW + 3; in.touchActive = in.touchDown = 1; in.touchY = 120; ProcessBattleBagTouch(&in);
    in.touchDown = 0; in.touchY = 122; ProcessBattleBagTouch(&in);
    in.touchActive = 0; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == 3 && !sBagPlan.activate);
    BagPlanClear(); ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_NONE);
    /* a tap on the cursor's row takes it (and USE once the menu opens) */
    hitCode = BUI_BAG_ROW; in.touchActive = in.touchDown = 1; ProcessBattleBagTouch(&in);
    in.touchDown = 0; in.touchActive = 0; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == 0 && sBagPlan.activate && sBatBag.autoWait > 0);
    BagPlanClear(); sBatBag.autoWait = 0;
    /* a finger that moves on DOWN does not scroll: DOWN pages on release */
    hitCode = BUI_BAG_DOWN; in.touchActive = in.touchDown = 1; in.touchY = 150; ProcessBattleBagTouch(&in);
    in.touchDown = 0; in.touchY = 150 - 3 * BAG_DRAG_ROW; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_NONE && sBat.inside);
    in.touchActive = 0; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_LIST && sBagPlan.target == BUI_BAG_ROWS - 1 && !sBagPlan.activate);
    /* outside the list (the item's menu open) a row is not dragged */
    BagPlanClear(); gState = b->bagState = BAG_ACTIONS;
    hitCode = BUI_BAG_ROW + 1; in.touchActive = in.touchDown = 1; in.touchY = 150; ProcessBattleBagTouch(&in);
    in.touchDown = 0; in.touchY = 150 - 3 * BAG_DRAG_ROW; ProcessBattleBagTouch(&in);
    assert(sBagPlan.kind == BAGPLAN_NONE);
    return 0;
}
''')

    def test_battle_bag_holds_on_the_way_to_berries(self):
        """BERRIES opens the Berry Pouch from the Key Items pocket, which is only the way there: the battle Bag's
        picture holds while FireRed slides that pocket in. FireRed reports the pocket being revealed from the slide's
        first frame (0262), so the walk ends only when the slide is over. The walk back to Items (from the Berry Pouch)
        still ends as Items starts to slide in, so Items is drawn at once."""
        self.run_battle_bag('''
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

    def test_home_speed_and_party_taps(self):
        """HOME's SPEED line and its party (the owner's "F"): SPEED is usable where START would change the speed and a
        tap is one START press for the turbo, taken once; a Pokemon is usable when POKeMON is and opens it."""
        source = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        enabled = function(source, 'HomeTouchEnabled')
        activate = function(source, 'HomeTouchActivate')
        take = function(source, 'CtrBottom_TakeTurboTap')
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include "3ds_poketouch.h"
static int canCycle, partyOpens, activated = -1;
static bool CtrBottom_TurboCanCycle(void) { return canCycle; }
static int FieldEnabled(int id) { return id == PT_POKEMON && partyOpens; }
static void PokeTouchActivate(int id) { activated = id; }
static int sTurboTap;
''' + enabled + '\n' + activate + '\n' + take + '''
int main(void) {
    assert(!HomeTouchEnabled(PT_HOME_SPEED)); canCycle = 1; assert(HomeTouchEnabled(PT_HOME_SPEED) == 1);
    assert(!CtrBottom_TakeTurboTap());
    assert(HomeTouchActivate(PT_HOME_SPEED) && activated == -1);
    assert(CtrBottom_TakeTurboTap() && !CtrBottom_TakeTurboTap());
    for (int i = 0; i < 6; ++i) {
        partyOpens = 0; assert(!HomeTouchEnabled(PT_HOME_PARTY + i));
        partyOpens = 1; assert(HomeTouchEnabled(PT_HOME_PARTY + i) == 1);
    }
    assert(HomeTouchActivate(PT_HOME_PARTY + 5) && activated == PT_POKEMON);
    activated = -1;
    assert(HomeTouchEnabled(PT_BAG) == -1 && HomeTouchEnabled(PT_HOME_PARTY + 6) == -1);
    assert(!HomeTouchActivate(PT_BAG) && !HomeTouchActivate(PT_RUN) && activated == -1 && !CtrBottom_TakeTurboTap());
    return 0;
}
''')

    def test_home_pick_snaps_to_a_town(self):
        """HOME's map at 1:1: a town is one 8x8 cell, so a tap whose own cell is no place Fly goes to takes the nearest
        one that is, when its cell's centre is within 8 px of the tap (v0.6.0's orb snap); a tap on a route far from
        towns still takes the route. The cells are the home impl's own (CtrPokeTouch_HomeCellPoint/HomeMapCell)."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        home = (ROOT / '3ds_port/src/3ds_device_home_impl.h').read_text(encoding='utf-8')
        cells = '\n'.join(function(home, n) for n in ('CtrPokeTouch_HomeWindowPoint', 'CtrPokeTouch_HomeCellPoint',
                                                       'CtrPokeTouch_HomeMapCell'))
        pick = function(bottom, 'HomeFlyCell') + '\n' + function(bottom, 'HomePick')
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include "3ds_poketouch.h"
#define MAP_CELLS_X 22
#define MAP_CELLS_Y 15
#define MAPSEC_NONE_VALUE 0
/* A map of one town (5,5) and two routes beside it (6,5), (5,6), and one far away (12,10); 3 is the town's section. */
static int CtrPokeTouch_MapSection(int group, int x, int y, int *mapsec, int *dungeon)
{
    (void)group;
    *dungeon = 0;
    *mapsec = x == 5 && y == 5 ? 3 : (x == 6 && y == 5) || (x == 5 && y == 6) || (x == 12 && y == 10) ? 40 : 0;
    return *mapsec != 0;
}
static int CtrPokeTouch_FlyStatus(unsigned mapsec) { return mapsec == 3 ? PT_FLY_UNVISITED : PT_FLY_NONE; }
''' + cells + '\n' + pick + '''
int main(void) {
    CtrPokeTouchView v = {0};
    int cx, cy, x, y;

    /* the town's centre is (66, 62): its own cell takes it */
    assert(CtrPokeTouch_HomeCellPoint(&v, 5, 5, &x, &y) && x == 66 && y == 62);
    assert(HomePick(&v, 0, 66, 62, &cx, &cy) && cx == 5 && cy == 5);
    /* a near miss on the route beside it (5 px off the town's centre): the town */
    assert(HomePick(&v, 0, 71, 62, &cx, &cy) && cx == 5 && cy == 5);
    assert(HomePick(&v, 0, 66, 67, &cx, &cy) && cx == 5 && cy == 5);
    assert(HomePick(&v, 0, 71, 67, &cx, &cy) && cx == 5 && cy == 5);              /* 7 px, diagonally */
    /* further than 8 px: the route itself */
    assert(HomePick(&v, 0, 77, 62, &cx, &cy) && cx == 6 && cy == 5);
    assert(HomePick(&v, 0, 66, 71, &cx, &cy) && cx == 5 && cy == 6);
    /* a route far from towns, and the sea: the cell under the tap */
    assert(CtrPokeTouch_HomeCellPoint(&v, 12, 10, &x, &y) && HomePick(&v, 0, x + 3, y, &cx, &cy) && cx == 12 && cy == 10);
    assert(HomePick(&v, 0, 150, 120, &cx, &cy) && CtrPokeTouch_HomeMapCell(&v, 150, 120, &x, &y) && cx == x && cy == y);
    /* off the cells (the window's edge) or the window: nothing */
    assert(!HomePick(&v, 0, 15, 15, &cx, &cy) && !HomePick(&v, 0, 250, 60, &cx, &cy));
    return 0;
}
''')

    def test_home_place_shows_its_dungeon(self):
        """A place picked on HOME's map: its name, and under it why Fly cannot go there, or, when Fly has nothing to
        say about it and a dungeon lies over it, the dungeon's name (v0.6.0's line: ROUTE 4 shows MT. MOON); else
        nothing, and the panel shows the region."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        start = bottom.index('static struct {\n    int valid;')
        state = bottom[start:bottom.index('} sHome;', start) + len('} sHome;')]
        self.run_c('''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "3ds_poketouch.h"
#include "3ds_ui_strings.h"
#define MAPSEC_NONE_VALUE 0
enum { ROUTE_4 = 1, MT_MOON, CERULEAN, ROUTE_5, PEWTER };
static int CtrPokeTouch_FlyStatus(unsigned mapsec)
{
    return mapsec == CERULEAN ? PT_FLY_UNVISITED : mapsec == PEWTER ? PT_FLY_OK : PT_FLY_NONE;
}
static void CtrPokeTouch_MapSectionName(unsigned mapsec, char *out, int size)
{
    static const char *const kName[] = {"", "ROUTE 4", "MT. MOON", "CERULEAN CITY", "ROUTE 5", "PEWTER CITY"};
    snprintf(out, (size_t)size, "%s", mapsec <= PEWTER ? kName[mapsec] : "?");
}
''' + state + '\n' + function(bottom, 'HomeSelectionBar') + '''
static void Pick(CtrPokeTouchView *v, int mapsec, int dungeon)
{
    memset(v, 0, sizeof(*v));
    sHome.mapsec = mapsec;
    sHome.dungeon = dungeon;
    HomeSelectionBar(v);
}
int main(void) {
    CtrPokeTouchView v;

    Pick(&v, ROUTE_4, MT_MOON);                         /* the dungeon over the route, under its name */
    assert(!strcmp(v.bar[0], "ROUTE 4") && !strcmp(v.bar[2], "MT. MOON") && !v.flyOk);
    Pick(&v, ROUTE_5, MAPSEC_NONE_VALUE);               /* nothing over it: nothing (the region) */
    assert(!strcmp(v.bar[0], "ROUTE 5") && !v.bar[2][0]);
    Pick(&v, MAPSEC_NONE_VALUE, MT_MOON);               /* the dungeon alone: its name is the place's */
    assert(!strcmp(v.bar[0], "MT. MOON") && !v.bar[2][0]);
    Pick(&v, CERULEAN, MT_MOON);                        /* Fly says why not: that, not the dungeon */
    assert(!strcmp(v.bar[0], "CERULEAN CITY") && !strcmp(v.bar[2], CtrUiText(UIS_FLY_UNVISITED)));
    Pick(&v, PEWTER, MT_MOON);                          /* Fly can go: FLY, and the line stays the region's */
    assert(!strcmp(v.bar[0], "PEWTER CITY") && !v.bar[2][0] && v.flyOk);
    return 0;
}
''')

    def test_home_touch_only_on_home_shown(self):
        """On a field frame HOME's keys, its party, FLY, the region key and the map take a touch only while HOME is the
        picture the lower screen shows: the frame after another screen closed still shows that screen's picture (the
        Bag's, a viewport's), and a touch there must not open what HOME has at that place. A native screen (OPTIONS,
        SAVE) takes its touches itself: nothing of HOME's is hit under it, shown yet or not."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        start = bottom.index('static struct {\n    int valid;')
        state = bottom[start:bottom.index('} sHome;', start) + len('} sHome;')]
        begin = bottom.index('/* Touch-down presses an enabled element')
        process = function(bottom, 'ProcessPokeTouch')
        code = bottom[begin:bottom.index(process) + len(process)]
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
enum { MODE_FIELD = 1, MODE_BAG = 4 };
#define MAPSEC_NONE_VALUE 0
#define MAP_TAP_SLOP 6
static struct { int mode; CtrPokeTouchView touch; } sShown;
static int sHaveShown;
static struct { uint8_t id; uint8_t inside; } sPoke = {PT_NONE, 0};
static int sFlyPick;
static unsigned sHomeOpen;
static int native, activated = -1, nextMap, flew, tapped;
static int NativeActive(void) { return native; }
static int OptionsActive(void) { return 0; }
static int SaveActive(void) { return 0; }
static void ProcessOptionsTouch(const CtrInput *in) { (void)in; }
static void ProcessSaveInput(const CtrInput *in) { (void)in; }
/* HOME: BAG's key at the bottom right, FLY and the region key in the window's top corners, the map below them */
int CtrPokeTouch_HomeHit(const CtrPokeTouchView *v, int x, int y) { (void)v; return x >= 200 && y >= 150 ? PT_BAG : PT_NONE; }
bool CtrPokeTouch_FlyHit(int x, int y) { return x >= 160 && x < 200 && y < 30; }
bool CtrPokeTouch_HomeRibbonHit(int x, int y) { return x < 30 && y < 30; }
bool CtrPokeTouch_HomeWindowPoint(int x, int y, int *vx, int *vy) { *vx = x; *vy = y; return x < 200 && y < 150; }
int CtrPokeTouch_FlyStatus(unsigned mapsec) { (void)mapsec; return PT_FLY_OK; }
void CtrPokeTouch_RequestFly(unsigned mapsec) { (void)mapsec; ++flew; }
''' + state + '''
static void HomeNextMap(unsigned open) { (void)open; ++nextMap; }
static int FieldEnabled(int id) { (void)id; return 1; }
static int HomePick(const CtrPokeTouchView *v, int group, int x, int y, int *cx, int *cy)
{ (void)v; (void)group; *cx = x / 8; *cy = y / 8; return 1; }
static void HomeTap(int cx, int cy) { (void)cx; (void)cy; ++tapped; }
static int HomeTouchActivate(int id) { (void)id; return 0; }
static void PokeTouchActivate(int id) { activated = id; }
''' + code + '''
static void Tap(int x, int y)
{
    CtrInput in;

    memset(&in, 0, sizeof(in));
    in.touchActive = in.touchDown = 1; in.touchX = (uint16_t)x; in.touchY = (uint16_t)y;
    ProcessPokeTouch(&in);
    in.touchDown = 0; ProcessPokeTouch(&in);
    in.touchActive = 0; ProcessPokeTouch(&in);
}
int main(void) {
    /* HOME shown: BAG's key, the map, the region key, FLY */
    sHaveShown = 1; sShown.mode = MODE_FIELD; sShown.touch.home = 1; sHome.view = 1;
    Tap(250, 200); assert(activated == PT_BAG);
    Tap(100, 80); assert(tapped == 1);
    Tap(10, 10); assert(nextMap == 1);
    sHome.valid = 1; sHome.mapsec = 5; Tap(180, 10); assert(flew == 1);
    /* the Bag's picture still shown on the field's first frame: nothing of HOME's is touched */
    activated = -1; tapped = nextMap = flew = 0; sHome.valid = 1;
    sShown.mode = MODE_BAG; sShown.touch.home = 0; sShown.touch.bag.on = 1;
    Tap(250, 200); Tap(100, 80); Tap(10, 10); Tap(180, 10);
    assert(activated == -1 && !tapped && !nextMap && !flew && !sHome.down && sPoke.id == PT_NONE);
    /* a field frame that is not HOME (a viewport's picture), or nothing shown yet: the same */
    sShown.mode = MODE_FIELD; sShown.touch.bag.on = 0; sShown.touch.viewport = 1;
    Tap(250, 200); Tap(100, 80); assert(activated == -1 && !tapped);
    sShown.touch.viewport = 0; sShown.touch.home = 1; sHaveShown = 0;
    Tap(250, 200); Tap(100, 80); assert(activated == -1 && !tapped);
    /* a native screen with nothing shown yet, or HOME still shown on its first frame: nothing of HOME's is hit */
    native = 1; sShown.touch.home = 1; Tap(250, 200); Tap(100, 80); Tap(10, 10);
    assert(activated == -1 && !tapped && !nextMap && sPoke.id == PT_NONE && !sHome.down);
    sHaveShown = 1; Tap(250, 200); Tap(100, 80); Tap(10, 10);
    assert(activated == -1 && !tapped && !nextMap && sPoke.id == PT_NONE && !sHome.down);
    /* SAVE, OPTIONS, the Pokedex, a choice, the move to forget, the relearner, the naming keyboard, the PC box, Easy
     * Chat or FireRed's picture on the red device: nothing hit */
    sShown.touch.home = 0; sShown.touch.save = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.save = 0; sShown.touch.options = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.options = 0; sShown.touch.dex.on = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.dex.on = 0; sShown.touch.choice.on = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.choice.on = 0; sShown.touch.learn.on = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.learn.on = 0; sShown.touch.relearn.on = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.relearn.on = 0; sShown.touch.name.on = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.name.on = 0; sShown.touch.box.on = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.box.on = 0; sShown.touch.ec.on = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.ec.on = 0; sShown.touch.viewport = 1; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);
    sShown.touch.viewport = 0; Tap(20, 20); assert(activated == -1 && sPoke.id == PT_NONE);     /* the frame's, empty */
    sShown.touch.options = 0; sShown.touch.home = 1; Tap(20, 20); assert(activated == -1);
    return 0;
}
''')

    def test_options_keys_and_rows(self):
        """OPTIONS on the red device: a key acts once on release over it (▲ ▼ the page before and after, round; a
        section's key its first page), the selection going to that page's first row; moved off, nothing. A row is the
        shown page's: selected on touch-down, its zone stepped (or its level set) once on release over the same zone.
        A held physical button cancels either."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        start = bottom.index('static struct {\n    int sel;')
        state = bottom[start:bottom.index('} sOpt;', start) + len('} sOpt;')]
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
static struct { int mode; CtrPokeTouchView touch; } sShown;
static int sHaveShown = 1;
/* six pages of five rows; the sections start at pages 0, 1, 4 and 5 */
static int CtrOptions_PageOf(int row) { return row / 5; }
static int CtrOptions_PageCount(void) { return 6; }
static int CtrOptions_PageBase(int page) { return page * 5; }
static int CtrOptions_SectionFirstPage(int s) { static const int k[4] = {0, 1, 4, 5}; return k[s]; }
static int stepRow = -1, stepDir, setRow = -1, setValue = -1;
static void CtrOptions_Step(int row, int dir) { stepRow = row; stepDir = dir; }
static void CtrOptions_Set(int row, int value) { setRow = row; setValue = value; }
/* the screen: a key's code at x (y < 50), row y - 100 below (y >= 100) in zone x / 100 */
int CtrPokeTouch_OptionKey(const CtrPokeTouchView *v, int x, int y) { (void)v; return y < 50 ? x : -1; }
bool CtrPokeTouch_OptionHit(const CtrPokeTouchView *v, int x, int y, int *row, int *zone)
{ (void)v; if (y < 100) return false; *row = y - 100; *zone = x / 100; return true; }
''' + state + '\n' + function(bottom, 'ProcessOptionsTouch') + '''
/* down at (x, y), held over (mx, my), released there; `held` a physical button the while */
static void Touch(int x, int y, int mx, int my, unsigned held)
{
    CtrInput in;

    memset(&in, 0, sizeof(in));
    in.touchActive = in.touchDown = 1; in.touchX = (uint16_t)x; in.touchY = (uint16_t)y;
    ProcessOptionsTouch(&in);
    in.touchDown = 0; in.touchX = (uint16_t)mx; in.touchY = (uint16_t)my; in.held = held;
    ProcessOptionsTouch(&in);
    in.touchActive = 0; ProcessOptionsTouch(&in);
}
int main(void) {
    sOpt.sel = 7;                                                   /* page 1 */
    Touch(PT_OPT_KEY_UP, 10, PT_OPT_KEY_UP, 10, 0); assert(sOpt.sel == 0 && !sOpt.key);
    Touch(PT_OPT_KEY_UP, 10, PT_OPT_KEY_UP, 10, 0); assert(sOpt.sel == 25);      /* round, to the last page */
    Touch(PT_OPT_KEY_DOWN, 10, PT_OPT_KEY_DOWN, 10, 0); assert(sOpt.sel == 0);   /* and back to the first */
    Touch(PT_OPT_KEY_SECTION + 2, 10, PT_OPT_KEY_SECTION + 2, 10, 0); assert(sOpt.sel == 20);
    Touch(PT_OPT_KEY_SECTION + 1, 10, PT_OPT_KEY_SECTION + 1, 10, 0); assert(sOpt.sel == 5);
    Touch(PT_OPT_KEY_DOWN, 10, 300, 200, 0); assert(sOpt.sel == 5 && !sOpt.key); /* moved off: nothing */
    Touch(PT_OPT_KEY_DOWN, 10, PT_OPT_KEY_DOWN, 10, 1); assert(sOpt.sel == 5); /* a button held: nothing */
    /* the shown page's rows: page 4's third row, its NEXT zone, then a level */
    sShown.touch.optPage = 4;
    Touch(300, 102, 300, 102, 0); assert(sOpt.sel == 22 && stepRow == 22 && stepDir == 1);
    Touch(250, 103, 250, 103, 0); assert(sOpt.sel == 23 && stepRow == 23 && stepDir == -1);
    stepRow = -1; Touch(100, 103, 100, 103, 0); assert(sOpt.sel == 23 && stepRow == -1);     /* the label selects only */
    Touch(PT_OPT_ZONE_SEG * 100 + 50, 104, PT_OPT_ZONE_SEG * 100 + 50, 104, 0); assert(setRow == 24 && setValue == 0);
    stepRow = -1; Touch(300, 102, 150, 102, 0); assert(sOpt.sel == 22 && stepRow == -1); /* moved to another zone */
    sHaveShown = 0; Touch(300, 102, 300, 102, 0); assert(stepRow == -1);                 /* nothing shown yet */
    return 0;
}
''')

    def test_route_and_column_gone(self):
        """The START column, its X / Y tiles and the route that unwound one menu to open another are gone (decision 3):
        no view draws the column, so nothing hits it or routes from it. The X and Y buttons still use the registered
        item and the Bicycle, from the field as ever. BACK is the one the view draws."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        poketouch = ((ROOT / '3ds_port/src/3ds_poketouch.c').read_text(encoding='utf-8')
                     + (ROOT / '3ds_port/include/3ds_poketouch.h').read_text(encoding='utf-8'))
        for name in ('ProcessRouteTouch', 'RequestDestination', 'CanRouteDestination', 'CtrPokeTouch_HitTest',
                     'gCtrPokeTouchRects', 'gCtrPokeTouchBack', 'CtrPokeTouch_DeviceShown', 'sPendingScreen',
                     'CtrPokeTouch_HitOptPage'):
            self.assertNotIn(name, bottom + poketouch)
        frame = function(bottom, 'CtrBottom_Frame')            # X and Y keep the registered item and the Bicycle
        self.assertIn('CtrPokeTouch_RequestFieldCommand(PT_BIKE);', frame)
        self.assertIn('CtrPokeTouch_RequestFieldCommand(PT_REGISTERED);', frame)
        self.assertIn('CtrPokeTouch_BackHit(&sShown.touch,', function(bottom, 'ProcessBack'))

    def test_idle_screen_in_the_shell_colour(self):
        """The idle screen (nothing for FireRed's screen above: MODE_OFF) is the red device's in the shell colour OPTIONS
        chose: Render gives it the snapshot's shell, which every frame fills whatever the mode (MODE_OFF too), so a
        change of the shell changes the snapshot and the idle screen is drawn again."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        self.assertIn('CtrPokeTouch_DrawIdle(sCanvas, s->idleLogo, s->touch.shell);', function(bottom, 'Render'))
        frame = function(bottom, 'CtrBottom_Frame')
        fill = frame.index('    if (!nav)\n        state.touch.shell = state.battle.shell = state.sum.shell = '
                           '(uint8_t)CtrOptions_Shell();\n')
        self.assertLess(frame.index('SnapshotState(&state,'), fill)
        self.assertLess(fill, frame.index('memcmp(&state, &sShown, sizeof(state))'))

    def test_box_chooser_cancel_sends_b(self):
        """The PC box's chooser (DEPOSIT's, JUMP's) has no CANCEL of FireRed's: its CANCEL key is the view's BACK while
        the chooser is up (3ds_device_box_impl.h DevBoxBackKey; the box suite of device_render_test.c pins where it is
        drawn and hit). BACK is shown in the box and enabled while the chooser is up, and a tap held and released on it
        sends FireRed's B once, as v0.6.0's BACK did there; moved off it, with a button held, or not enabled, nothing."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        self.assertIn('mode == MODE_STORAGE_BOX', function(bottom, 'BackVisible'))
        self.assertRegex(function(bottom, 'BackEnabled'),
                         r'if \(mode == MODE_STORAGE_BOX\)\s+return [^;]*CtrBottomStorage_BoxChoose\(\)')
        code = function(bottom, 'BackPress') + '\n' + function(bottom, 'ProcessBack')
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
enum { MODE_FIELD = 1, MODE_STORAGE_PC, MODE_STORAGE_BOX, MODE_CARD, MODE_DEX, MODE_CHOICE, MODE_GBAVIEW,
       MODE_PARTY_CHILD, MODE_SUMMARY };
enum { BAGPLAN_PRESS = 1, SUMPLAN_PRESS = 1 };
#define K_B (1u << 1)
typedef struct ListScreen ListScreen;
static uint16_t sRouteKey;
static int sHaveShown = 1, choosing = 1;
static struct { int mode; CtrPokeTouchView touch; } sShown;
static struct { int down; int inside; } sBack;
static int BackVisible(int mode) { return mode == MODE_STORAGE_BOX; }
static int BackEnabled(int mode) { return mode == MODE_STORAGE_BOX && choosing; }
/* the chooser's CANCEL, the view's BACK, at the bottom right */
bool CtrPokeTouch_BackHit(const CtrPokeTouchView *v, int x, int y) { (void)v; return x >= 188 && y >= 172; }
static void CloseNative(void) { assert(0); }
static uint16_t CardLeaveKey(void) { assert(0); return 0; }
static const ListScreen *ListFor(int mode) { (void)mode; return NULL; }
static void BagPlanStart(const ListScreen *l, int kind, int target, int activate, uint16_t keys)
{ (void)l; (void)kind; (void)target; (void)activate; (void)keys; assert(0); }
static void SumPlanStart(int kind, int target, int activate, uint16_t keys)
{ (void)kind; (void)target; (void)activate; (void)keys; assert(0); }
''' + code + '''
static void Touch(int x, int y, int down, int active, uint16_t held)
{
    CtrInput in;

    memset(&in, 0, sizeof(in));
    in.touchActive = active; in.touchDown = down; in.touchX = (uint16_t)x; in.touchY = (uint16_t)y; in.held = held;
    ProcessBack(MODE_STORAGE_BOX, &in);
}
int main(void)
{
    sShown.mode = MODE_STORAGE_BOX;
    Touch(250, 200, 1, 1, 0); Touch(250, 200, 0, 1, 0); assert(!sRouteKey);
    Touch(250, 200, 0, 0, 0); assert(sRouteKey == K_B);                        /* released on it: B, once */
    sRouteKey = 0; Touch(250, 200, 0, 0, 0); assert(!sRouteKey);
    Touch(250, 200, 1, 1, 0); Touch(100, 100, 0, 1, 0); Touch(100, 100, 0, 0, 0); assert(!sRouteKey);   /* moved off */
    Touch(250, 200, 1, 1, 0); Touch(250, 200, 0, 1, 2); Touch(250, 200, 0, 0, 0); assert(!sRouteKey);   /* B held */
    choosing = 0; Touch(250, 200, 1, 1, 0); Touch(250, 200, 0, 0, 0); assert(!sRouteKey);             /* not taken */
    return 0;
}
''')

    def test_field_lists_route_once(self):
        """The red device's Bag page takes its touch through one branch of ProcessTouch, FieldListFor's: the field's
        Bag, its cases, a mart's BUY list, the item PC and the mailbox. FieldListFor gives the last three in any session
        (before its PC-session check), and no branch of ProcessTouch before FieldListFor's names them, so they reach it
        as they did through their own copy of it."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        fill = (ROOT / '3ds_port/src/3ds_bottom_field_fill_impl.h').read_text(encoding='utf-8')
        touch = function(bottom, 'ProcessTouch')
        lists = function(fill, 'FieldListFor')
        self.assertEqual(touch.count('ProcessFieldBagTouch('), 1)
        before = touch[:touch.index('if (FieldListFor(mode) != NULL) {')]
        self.assertIn('ProcessFieldBagTouch(FieldListFor(mode), in);', touch[len(before):])
        case = re.search(r'#define IS_CASE_MODE\(mode\) (.*)', bottom).group(1)
        for mode in ('MODE_SHOP', 'MODE_ITEMPC', 'MODE_MAILBOX'):
            self.assertNotIn(mode, before)
            self.assertNotIn(mode, case)
            self.assertLess(lists.index('if (mode == ' + mode + ')'), lists.index('if (sStorageSession)'))

    def test_turbo_takes_the_speed_tap(self):
        """A tap on HOME's SPD is one START press for the turbo: the game bridge takes it every frame."""
        bridge = (ROOT / '3ds_port/src/3ds_game_bridge.c').read_text(encoding='utf-8')
        self.assertIn('bool turboTap = CtrBottom_TakeTurboTap();', bridge)
        self.assertIn('(CtrInput_Get()->physicalDown & CTR_KEY_START) != 0 || turboTap,', bridge)

    def test_battle_names_and_idle(self):
        """The battle fill: a Pokemon's name is its species' own (shown in capitals) when it matches the species name the
        game reports whatever the case (recipe 0251 title-cases them), not a nickname; a target's species is not
        reported, so any species' name counts. IDLE fills the quick ball as ACTION does, and in the Safari Zone (its
        chooser seen in this battle) shows Safari's keys instead; the end of the battle forgets it."""
        fill = (ROOT / '3ds_port/src/3ds_bottom_battle_fill_impl.h').read_text(encoding='utf-8')
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        species = re.search(r'#define BAT_SPECIES \d+', fill).group(0)
        code = '\n'.join(function(fill, n) for n in ('BatUpperByte', 'BatSameName', 'BatNameIsSpecies', 'BattleBallFill',
                                                     'BattleNoteChooser', 'BattleIdleFill'))
        self.assertIn('sBatSafari = 0;', function(bottom, 'CtrBottom_Frame'))
        self.run_c('''
#include <assert.h>
#include <string.h>
#include "3ds_poketouch.h"
#include "3ds_battle_nav.h"
enum { BATTLE_BUSY = 0, BATTLE_ACTION, BATTLE_MOVE, BATTLE_TARGET, BATTLE_SAFARI };
static uint8_t sBatSafari;
static int ballCalls;
static void CtrBottomDex_Name(unsigned species, char *out, int size)
{
    const char *name = species == 5 ? "Charmeleon" : species == 25 ? "Pikachu" : species == 29 ? "Nidoran\\xE2\\x99\\x80"
                     : species == 12 ? "Flab\\xC3\\xA9" "b\\xC3\\xA9" : species == 32 ? "Nidoran " : "";
    (void)size;
    strcpy(out, name);
}
static int CtrBattle_QuickBall(unsigned *item, unsigned *quantity, int *place, int *count)
{
    ++ballCalls;
    *item = 4; *quantity = 1200; *place = 1; *count = 3;
    return CTR_QUICK_BALL_READY;
}
static void CtrBottomBag_ItemName(unsigned item, char *out, int size) { (void)item; (void)size; strcpy(out, "GREAT BALL"); }
''' + species + '\n' + code + '''
int main(void)
{
    CtrBattleView v;

    /* the species' own name, in any case; a nickname; another species' name is not this one's */
    assert(BatNameIsSpecies("Charmeleon", 5) && BatNameIsSpecies("CHARMELEON", 5) && BatNameIsSpecies("charmeleon", 5));
    assert(!BatNameIsSpecies("Sparky", 5) && !BatNameIsSpecies("Charmeleo", 5) && !BatNameIsSpecies("Charmeleonx", 5));
    assert(!BatNameIsSpecies("Pikachu", 5) && !BatNameIsSpecies("", 5) && !BatNameIsSpecies("Charmeleon", 300));
    assert(BatNameIsSpecies("FLAB\\xC3\\x89" "B\\xC3\\x89", 12) && BatNameIsSpecies("NIDORAN\\xE2\\x99\\x80", 29));
    /* trailing spaces on either side do not count: the species name keeps one where its gender sign was (Nidoran),
     * the nickname has none */
    assert(BatNameIsSpecies("NIDORAN", 32) && BatNameIsSpecies("Nidoran  ", 32) && BatNameIsSpecies("NIDORAN", 0));
    assert(!BatNameIsSpecies("NIDORANX", 32) && !BatNameIsSpecies("NIDORA", 32) && !BatNameIsSpecies(" NIDORAN", 32));
    /* a target: any species' name */
    assert(BatNameIsSpecies("PIKACHU", 0) && BatNameIsSpecies("Charmeleon", 0) && !BatNameIsSpecies("Sparky", 0));
    /* IDLE: the quick ball as ACTION fills it; in the Safari Zone its own keys, no ball */
    memset(&v, 0, sizeof(v));
    BattleNoteChooser(BATTLE_ACTION);
    BattleIdleFill(&v);
    assert(!v.safari && v.ball == CTR_QUICK_BALL_READY && v.ballItem == 4 && v.ballQty == 999 && v.ballCount == 3
           && v.ballPlace == 1 && !strcmp(v.ballName, "GREAT BALL"));
    memset(&v, 0, sizeof(v));
    ballCalls = 0;
    BattleNoteChooser(BATTLE_SAFARI);
    BattleNoteChooser(BATTLE_BUSY);
    BattleIdleFill(&v);
    assert(v.safari && v.ball == CTR_QUICK_BALL_NONE && ballCalls == 0);
    BattleNoteChooser(BATTLE_MOVE);
    BattleIdleFill(&v);
    assert(!v.safari && v.ball == CTR_QUICK_BALL_READY);
    return 0;
}
''')


if __name__ == '__main__':
    unittest.main()
