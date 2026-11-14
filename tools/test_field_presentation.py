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
static void ViewFrame(CtrPokeTouchView *v) { (void)v; }   /* the player's FRAME: test_field_pages_take_the_frame */
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
static void HomePickDrop(void) { }
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

    def test_summary_turns_on_press(self):
        """The owner found the Summary slow to turn pages by touch, and MOVES <-> TRAINER slow on the D-pad: both acted
        on release. The page arrows and the previous / next Pokemon act on the touch's press (BACK, a move, the IV / EV
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
    /* a page arrow and the next Pokemon act on the touch's press */
    hitCode = SUI_HIT_PAGE_NEXT; sIn.touchActive = sIn.touchDown = 1; acted = -1; Frame();
    assert(acted == SUI_HIT_PAGE_NEXT);
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

    def test_summary_page_shown_when_taken(self):
        """A page turned by a plan (an arrow, TRAINER) is shown as soon as FireRed has taken it (its page is the one
        asked, while its own flip still runs), as a page turned by FireRed's own keys is; the way there (out of the move
        details first) stays hidden. The owner found TRAINER, always a plan, much slower: the lower screen waited for the
        whole flip."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        nav = function(bottom, 'NavPending')
        self.run_c('''
#include <assert.h>
#include "3ds_poketouch.h"
#include "3ds_summary.h"
enum { MODE_BAG = 3, MODE_SUMMARY = 5 };
enum { BAG_BUSY = 0 };
enum { SUMPLAN_NONE = 0, SUMPLAN_PAGE, SUMPLAN_MOVE, SUMPLAN_PRESS };
static struct { int mode, battleUi, summaryUi; } sShown;
static int sHaveShown = 1;
static struct { int pocketTarget; } sBatBag;
static struct { int kind, target; } sSumPlan;
static int page, ready;
int CtrBottomBag_InBattle(void) { return 0; }
int CtrBottomBag_Pocket(void) { return 0; }
int CtrBottomBag_State(void) { return 0; }
int CtrBottomSummary_Ready(void) { return ready; }
int CtrBottomSummary_Page(void) { return page; }
''' + nav + '''
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
    return 0;
}
''')

    def test_bridge_reports_the_destination(self):
        """FireRed moves to a new pocket only at the end of its switch's animation, and its Party's cursor pointer goes
        back to the first Pokemon while two of them slide to their places: the owner saw the Bag's list late and the
        Party's arrow go back and forth. The bridge reports where they are going: the pocket being revealed (its count,
        cursor and items are its own already), and the second Pokemon's slot while they slide."""
        name = '0239-lower-screen-shows-destination.json'
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
        name = '0240-party-switch-shows-new-places.json'
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
        there, or for a short while at most; a Bag tab's pocket is shown chosen while the Bag walks to it, the way there
        hidden; and a pocket FireRed has just switched to is drawn at once, while it slides in (its list is already the
        pocket's own), instead of after the slide."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        echo = function(bottom, 'PartyEchoCursor')
        hold = function(bottom, 'FieldBagHold')
        self.run_c('''
#include <assert.h>
enum { BAG_BUSY = 0, BAG_LIST };
static struct { int slot, wait; } sPartyEcho = {-1, 0};
''' + echo + '\n' + hold + '''
int main(void) {
    /* the Party: the slot touched until the cursor gets there */
    assert(PartyEchoCursor(2) == 2);
    sPartyEcho.slot = 4; sPartyEcho.wait = 30;
    assert(PartyEchoCursor(2) == 4 && PartyEchoCursor(2) == 4);
    assert(PartyEchoCursor(4) == 4 && sPartyEcho.slot == -1);
    assert(PartyEchoCursor(1) == 1);
    sPartyEcho.slot = 5; sPartyEcho.wait = 2;
    assert(PartyEchoCursor(0) == 5 && PartyEchoCursor(0) == 0);     /* not forever */
    /* the Bag: hold the picture on the way to a tab's pocket; draw a switched pocket while it slides */
    assert(FieldBagHold(1, BAG_LIST, 1, 0, 2) == 1);                /* on the way: KEY ITEMS toward POKe BALLS */
    assert(FieldBagHold(1, BAG_BUSY, 2, 1, 2) == 0);                /* arrived, sliding in: drawn */
    assert(FieldBagHold(1, BAG_BUSY, 1, 0, -1) == 0);               /* the D-pad switched it: drawn */
    assert(FieldBagHold(1, BAG_BUSY, 1, 1, -1) == 1);               /* busy otherwise (a message printing): held */
    assert(FieldBagHold(1, BAG_LIST, 1, 1, -1) == 0);
    assert(FieldBagHold(0, BAG_BUSY, 0, 0, -1) == 1 && FieldBagHold(0, BAG_LIST, 0, 0, -1) == 0);   /* a case */
    return 0;
}
''')

    def test_bag_page_cancel_and_drag(self):
        """The BAG's page: its CANCEL (and a tap off the item menu) is the Bag's B, sent through the Bag's plan as the
        old bar's BACK was; a drag over the rows moves the selection a row per BAG_ROW_H (the page's 30 px rows)."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        touch = function(bottom, 'ProcessFieldBagTouch')
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
#include "3ds_lower_layout.h"
#define K_A (1u << 0)
#define K_B (1u << 1)
#define K_RIGHT (1u << 4)
#define K_LEFT (1u << 5)
#define K_UP (1u << 6)
#define K_DOWN (1u << 7)
#define LIST_DRAG_SLOP 8
#define LIST_DRAG_ROW 26
enum { BAG_BUSY = 0, BAG_LIST, BAG_ACTIONS };
enum { BAGPLAN_NONE = 0, BAGPLAN_LIST, BAGPLAN_MENU, BAGPLAN_PRESS };
enum { MODE_BAG = 3 };
typedef struct { int (*state)(void); int (*count)(void); int (*maxShowed)(void); int (*cursor)(void); } ListScreen;
typedef struct { int mode, kind; const ListScreen *list; } FieldList;
static int cursorNow = 0;
static int State(void) { return BAG_LIST; }
static int Count(void) { return 9; }
static int Cursor(void) { return cursorNow; }
static const ListScreen sList = {State, Count, Count, Cursor};
static const FieldList sF = {MODE_BAG, PT_BAG_KIND_BAG, &sList};
static struct { int down, inside, code; int pocketTarget, pocketWait; int y0, cursor0, scrolling; } sFieldBag = {0, 0, -1, -1, 0, 0, 0, 0};
static struct { int kind; } sBagPlan;
static int sHaveShown = 1;
static struct { int mode; CtrPokeTouchView touch; } sShown;
static int planKind = -1, planTarget = -1, planKeys = 0, hitCode = -1;
static void BagPlanStart(const ListScreen *l, int kind, int target, int activate, unsigned short keys)
{ (void)l; (void)activate; planKind = kind; planTarget = target; planKeys = keys; }
int CtrBottomBag_Pocket(void) { return 0; }
bool CtrBottom_IsFading(void) { return 0; }
int CtrPokeTouch_BagHit(const CtrPokeTouchView *v, int x, int y) { (void)v; (void)x; (void)y; return hitCode; }
''' + touch + '''
static CtrInput in;
int main(void) {
    sShown.mode = MODE_BAG; sShown.touch.bag.on = 1; sShown.touch.bag.state = BAG_LIST; sShown.touch.bag.count = 9;
    /* CANCEL: on release, the Bag's B */
    hitCode = PT_BAG_HIT_BACK; in.touchActive = in.touchDown = 1; ProcessFieldBagTouch(&sF, &in);
    assert(planKind == -1);
    in.touchDown = 0; in.touchActive = 0; ProcessFieldBagTouch(&sF, &in);
    assert(planKind == BAGPLAN_PRESS && planKeys == K_B);
    /* a drag from a row: 3 rows down the list per 3 * BAG_ROW_H up */
    planKind = -1; hitCode = PT_BAG_HIT_ROW + 1; cursorNow = 4;
    in.touchActive = in.touchDown = 1; in.touchY = 150; ProcessFieldBagTouch(&sF, &in);
    in.touchDown = 0; in.touchY = 150 - 3 * BAG_ROW_H; ProcessFieldBagTouch(&sF, &in);
    assert(planKind == BAGPLAN_LIST && planTarget == 7);
    return 0;
}
''')

    def test_pages_take_the_frame(self):
        """OPTIONS -> FRAME reaches every page drawn in the kit: the battle's views (commands, moves, the Party, the
        Bag) through BattleCommon, and the Summary through SummaryFill. The Party and the Summary used to stay on the
        first frame."""
        fills = (ROOT / '3ds_port/src/3ds_bottom_battle_fill_impl.h').read_text(encoding='utf-8')
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        common = function(fills, 'BattleCommon')
        summary = function(bottom, 'SummaryFill')
        self.run_c('''
#include <assert.h>
#include <string.h>
#include "3ds_poketouch.h"
#include "3ds_summary.h"
static int option;
int CtrPokeTouch_OptionGet(int id) { return id == PT_OPT_FRAME ? option : 0; }
static struct { int down, inside, btn; } sBat;
static unsigned sBatAnim;
static void PartyFill(uint8_t a[6], uint8_t b[6], uint8_t c[6], uint16_t d[6]) { (void)a; (void)b; (void)c; (void)d; }
static int BattleActiveSlot(void) { return 0; }
static unsigned CtrBottomParty_Level(int s) { (void)s; return 5; }
static void CtrBottomParty_Nickname(int s, char *o) { (void)s; o[0] = 0; }
static unsigned CtrBottomParty_IconSpecies(int s) { (void)s; return 1; }
static unsigned CtrBottomParty_HP(int s) { (void)s; return 1; }
static unsigned CtrBottomParty_MaxHP(int s) { (void)s; return 1; }
enum { SUMPLAN_NONE = 0 };
static struct { int kind; } sSumPlan;
static struct { int down, inside, btn, trainer, ivView; unsigned serial; CtrSummaryView latch; } sSum;
unsigned CtrBottomSummary_Serial(void) { return 1; }
static int summaryReady;
void CtrBottomSummary_GetView(CtrSummaryView *v) { memset(v, 0, sizeof(*v)); v->serial = 1; v->ready = summaryReady; strcpy(v->species, "X"); }
unsigned CtrBottomSummary_IconSpecies(void) { return 1; }
''' + common + '\n' + summary + '''
int main(void) {
    CtrBattleView v;
    CtrSummaryUiView s;
    for (option = 0; option < 10; ++option) {
        BattleCommon(&v);
        assert(v.frameType == option);
        SummaryFill(&s);
        assert(s.frameType == option);
    }
    {
        /* FireRed's input readiness (it flips at the end of every page turn) is not part of the picture: the view stays
         * the same, so the page is not drawn again for it */
        CtrSummaryUiView t;

        summaryReady = 0; SummaryFill(&s);
        summaryReady = 1; SummaryFill(&t);
        assert(!memcmp(&s, &t, sizeof(s)));
    }
    return 0;
}
''')

    def test_field_pages_take_the_frame(self):
        """OPTIONS -> FRAME reaches every field page: SnapshotState gives every view the player's frame before any mode's
        own fill (the choice, the PC box, the forget-a-move page, the naming keyboard and FireRed's picture's frame return
        before the field's common block, and stayed on the first frame)."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='latin-1')
        frame = function(bottom, 'ViewFrame')
        snap = function(bottom, 'SnapshotState')
        first_branch = snap.index('if (mode ==')
        self.assertIn('ViewFrame(&s->touch);', snap[:first_branch])
        self.run_c('''
#include <assert.h>
#include <string.h>
#include "3ds_poketouch.h"
static int option;
int CtrPokeTouch_OptionGet(int id) { return id == PT_OPT_FRAME ? option : 0; }
''' + frame + '''
int main(void) {
    CtrPokeTouchView v;
    for (option = 0; option < 10; ++option) {
        memset(&v, 0, sizeof(v));
        ViewFrame(&v);
        assert(v.frameType == option);
    }
    return 0;
}
''')

    def test_home_pick_dropped(self):
        """The map's hint has the party's window while a place is picked (or the Party's FLY waits); the final review
        found nothing a player would try gave the party back. A tap on the hint does, and so does any button in the
        field (walking away); a battle, a native screen or START's menu still give up FLY alone."""
        source = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
        enabled = function(source, 'HomeTouchEnabled')
        activate = function(source, 'HomeTouchActivate')
        drop = function(source, 'HomePickDrop')
        expire = function(source, 'HomePicksExpire')
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include "3ds_poketouch.h"
enum { MODE_FIELD = 1, MODE_BATTLE = 2 };
static int sFlyPick, sTurboTap, activated = -1;
static struct { int valid; } sHome;
static bool CtrBottom_TurboCanCycle(void) { return 1; }
static int FieldEnabled(int id) { (void)id; return 1; }
static void PokeTouchActivate(int id) { activated = id; }
''' + drop + '\n' + enabled + '\n' + activate + '\n' + expire + '''
int main(void) {
    assert(HomeTouchEnabled(PT_HOME_HINT) == 1);
    sHome.valid = 1; assert(HomeTouchActivate(PT_HOME_HINT) && !sHome.valid && activated == -1 && !sTurboTap);
    sFlyPick = 1; assert(HomeTouchActivate(PT_HOME_HINT) && !sFlyPick);
    sHome.valid = sFlyPick = 1; HomePicksExpire(MODE_FIELD, 0, 0); assert(sHome.valid && sFlyPick);
    HomePicksExpire(MODE_FIELD, 0x40u, 0); assert(!sHome.valid && !sFlyPick);
    sHome.valid = sFlyPick = 1; HomePicksExpire(MODE_FIELD, 0x400u, 0); assert(sHome.valid && sFlyPick);
    HomePicksExpire(MODE_BATTLE, 0x1u, 1); assert(sHome.valid && !sFlyPick);
    return 0;
}
''')

    def test_field_pages_leave_the_sidebar(self):
        """While a field page shows (the Pokedex, ...) the old sidebar is gone: a touch where its buttons were routes
        nowhere, and BACK is the page's own cell (CtrPokeTouch_BackHit), released on it once, as the bar's was."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='latin-1')
        names = ['FieldPageUp', 'ProcessBack', 'ProcessRouteTouch', 'PokeHit']
        code = '\n'.join(function(bottom, n) for n in names)
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
enum { MODE_FIELD = 1, MODE_DEX = 7 };
static struct { int down, inside; } sBack;
static struct { uint8_t id, inside; } sPoke;
static int sHaveShown = 1, sPendingScreen = PT_NONE, pageShown = 1, backAt = 0, pressed = -1, requested = -1;
static struct { int mode; CtrPokeTouchView touch; } sShown;
static int BackVisible(int mode) { (void)mode; return 1; }
static int BackEnabled(int mode) { (void)mode; return 1; }
static void BackPress(int mode) { pressed = mode; }
static int CanRouteDestination(int id, int mode) { (void)mode; return id == PT_BAG; }
static void RequestDestination(int id, int mode) { (void)mode; requested = id; }
bool CtrPokeTouch_FieldPageShown(const CtrPokeTouchView *v) { (void)v; return pageShown; }
bool CtrPokeTouch_BackHit(const CtrPokeTouchView *v, int x, int y) { (void)v; return x == backAt && y == backAt; }
bool CtrPokeTouch_HitBack(int x, int y) { return x == 200 && y == 220; }
int CtrPokeTouch_HitTest(int x, int y) { return x > 248 && y < 80 ? PT_BAG : PT_NONE; }
int CtrPokeTouch_HomeHit(const CtrPokeTouchView *v, int x, int y) { (void)v; (void)x; (void)y; return -1; }
''' + code + '''
static CtrInput in;
static void Touch(int x, int y, int down) { in.touchActive = 1; in.touchDown = down; in.touchX = x; in.touchY = y; }
static void Lift(void) { in.touchActive = in.touchDown = 0; }
int main(void) {
    sShown.mode = MODE_DEX;
    /* the page's BACK: pressed on it, released on it: the Pokedex's B once */
    backAt = 260;
    Touch(260, 260, 1); ProcessBack(MODE_DEX, &in); assert(sBack.down && pressed == -1);
    Touch(260, 260, 0); ProcessBack(MODE_DEX, &in); Lift(); ProcessBack(MODE_DEX, &in);
    assert(pressed == MODE_DEX);
    /* the old bar's BACK is not there */
    pressed = -1; Touch(200, 220, 1); ProcessBack(MODE_DEX, &in); Lift(); ProcessBack(MODE_DEX, &in);
    assert(pressed == -1);
    /* the sidebar's BAG routes nowhere while a page shows; it does where the old shell still has it */
    Touch(300, 60, 1); ProcessRouteTouch(MODE_DEX, &in); Lift(); ProcessRouteTouch(MODE_DEX, &in);
    assert(requested == -1);
    assert(PokeHit(0, 300, 60) == PT_NONE);
    pageShown = 0;
    Touch(300, 60, 1); ProcessRouteTouch(MODE_DEX, &in); Lift(); ProcessRouteTouch(MODE_DEX, &in);
    assert(requested == PT_BAG && PokeHit(0, 300, 60) == PT_BAG);
    pressed = -1; Touch(200, 220, 1); ProcessBack(MODE_DEX, &in); Lift(); ProcessBack(MODE_DEX, &in);
    assert(pressed == MODE_DEX);
    return 0;
}
''')

    def test_dex_page_touch(self):
        """The Pokedex's page: its arrows (a page of the list or of the grid, a habitat page) act on the touch's press, as
        the owner asked of every page arrow; a row on release. A drag over the list moves a row per DEX_LIST_ROW_H. The
        row or place a touch's plan takes the cursor to is shown at once (DexShownCursor), not the old one first."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='latin-1')
        names = ['DexShownCursor', 'FieldDexAct', 'ProcessFieldDexTouch']
        code = '\n'.join(function(bottom, n) for n in names)
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
#include "3ds_lower_layout.h"
#define K_A (1u << 0)
#define K_START (1u << 3)
#define LIST_DRAG_SLOP 8
enum { MODE_DEX = 7 };
enum { DEX_BUSY = 0, DEX_MENU, DEX_LIST, DEX_CATEGORY, DEX_PAGE };
enum { DEXPLAN_LIST = 0, DEXPLAN_SLOT, DEXPLAN_FLIP };
static struct { int active, target, state, steps, release, kind, page, press; } sDexPlan;
static struct { int down, inside, code; int y0, cursor0, scrolling, acted; } sFieldDex = {0, 0, -1, 0, 0, 0, 0};
static int sHaveShown = 1, state = DEX_LIST, cursor = 10, hitCode = -1, planTo = -1, planPick = -1, planKind = -1;
static unsigned short sRouteKey;
static struct { int mode; CtrPokeTouchView touch; } sShown;
int CtrBottomDex_State(void) { return state; }
int CtrBottomDex_Cursor(void) { return cursor; }
int CtrBottomDex_PageCanFlip(int dir) { (void)dir; return 1; }
bool CtrBottom_IsFading(void) { return 0; }
int CtrPokeTouch_DexHit(const CtrPokeTouchView *v, int x, int y) { (void)v; (void)x; (void)y; return hitCode; }
int CtrPokeTouch_DexTabRow(const CtrPokeTouchView *v, int tab) { (void)v; return tab * 5; }
int CtrPokeTouch_DexGridPageRow(const CtrPokeTouchView *v, int dir) { (void)v; return dir > 0 ? 9 : -1; }
static void DexPlanClear(void) { memset(&sDexPlan, 0, sizeof(sDexPlan)); }
static void DexPlanTo(int kind, int target, int st)
{ DexPlanClear(); sDexPlan.active = 1; sDexPlan.kind = planKind = kind; sDexPlan.target = planTo = target; sDexPlan.state = st; }
static void DexPlanPick(int kind, int target, int st) { DexPlanTo(kind, target, st); planPick = target; sDexPlan.press = 1; }
''' + code + '''
static CtrInput in;
static void Touch(int y, int down) { in.touchActive = 1; in.touchDown = down; in.touchY = y; ProcessFieldDexTouch(&in); }
static void Lift(void) { in.touchActive = in.touchDown = 0; ProcessFieldDexTouch(&in); }
int main(void) {
    CtrPokeTouchDex *d = &sShown.touch.dex;
    sShown.mode = MODE_DEX; d->on = 1; d->state = DEX_LIST; d->listCount = 151; d->listCursor = 10; d->listFirst = 8;
    /* the list's DOWN: on the press, four rows on */
    hitCode = PT_DEX_HIT_DOWN; Touch(120, 1);
    assert(planTo == 14 && planKind == DEXPLAN_LIST);
    Touch(120, 0); planTo = -1; Lift();
    assert(planTo == -1);                               /* not again on release */
    /* a row: nothing on the press, its pick on release */
    DexPlanClear(); hitCode = PT_DEX_HIT_ROW + 3; Touch(100, 1);
    assert(planTo == -1);
    Lift(); assert(planPick == 11);
    /* a drag from a row: a row per DEX_LIST_ROW_H */
    DexPlanClear(); planTo = -1; Touch(150, 1); Touch(150 - 3 * DEX_LIST_ROW_H, 0);
    assert(planTo == 13);
    Lift();
    /* the shown cursor: the plan's target while it runs in the state it was made for (not a habitat page's turn) */
    DexPlanClear();
    assert(DexShownCursor(DEX_LIST, 10) == 10);
    DexPlanTo(DEXPLAN_LIST, 40, DEX_LIST);
    assert(DexShownCursor(DEX_LIST, 10) == 40 && DexShownCursor(DEX_MENU, 3) == 3);
    DexPlanTo(DEXPLAN_SLOT, 2, DEX_CATEGORY);
    assert(DexShownCursor(DEX_CATEGORY, 0) == 2);
    DexPlanTo(DEXPLAN_FLIP, 1, DEX_CATEGORY);
    assert(DexShownCursor(DEX_CATEGORY, 0) == 0);
    /* a habitat page's NEXT and the grid's DOWN: on the press */
    DexPlanClear(); state = DEX_CATEGORY; d->state = DEX_CATEGORY; hitCode = PT_DEX_HIT_NEXT; planKind = -1; Touch(200, 1);
    assert(planKind == DEXPLAN_FLIP && sDexPlan.target == 1);
    Lift();
    DexPlanClear(); state = DEX_MENU; d->state = DEX_MENU; hitCode = PT_DEX_HIT_DOWN; planTo = -1; Touch(100, 1);
    assert(planTo == 9);
    Lift();
    /* AREA: on release, its A */
    DexPlanClear(); state = DEX_PAGE; d->state = DEX_PAGE; hitCode = PT_DEX_HIT_AREA; Touch(200, 1);
    assert(!sRouteKey);
    Lift(); assert(sRouteKey == K_A);
    return 0;
}
''')

    def test_options_page_turns_on_press(self):
        """OPTIONS' PAGE cell turns to the next page on the touch's press (the owner's rule for page arrows), once per
        touch, held while the finger stays; a row's value still steps on release over the same place."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='latin-1')
        touch = function(bottom, 'ProcessOptionsTouch')
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
static struct { int sel; int down, row, zone, inside; int pageDown, pageInside; } sOpt;
static int onPage, onRow = -1, stepped = 0;
int CtrOptions_RowCount(void) { return 17; }
bool CtrPokeTouch_HitOptPage(int x, int y) { (void)x; (void)y; return onPage; }
bool CtrPokeTouch_OptionHit(int x, int y, int rows, int *row, int *zone)
{ (void)x; (void)y; (void)rows; if (onRow < 0) return false; *row = onRow; *zone = PT_OPT_ZONE_NEXT; return true; }
int CtrOptions_IsLevels(int row) { (void)row; return 0; }
int CtrPokeTouch_OptionSegment(int x, int count) { (void)x; (void)count; return 0; }
int CtrOptions_Count(int row) { (void)row; return 3; }
void CtrOptions_Set(int row, int value) { (void)row; (void)value; }
void CtrOptions_Step(int row, int dir) { (void)row; stepped += dir; }
''' + touch + '''
static CtrInput in;
static void Touch(int down) { in.touchActive = 1; in.touchDown = down; ProcessOptionsTouch(&in); }
static void Lift(void) { in.touchActive = in.touchDown = 0; ProcessOptionsTouch(&in); }
int main(void) {
    /* PAGE: the next page on the press, held while on it, nothing more on release */
    onPage = 1; Touch(1);
    assert(sOpt.sel == PT_OPT_MAX && sOpt.pageDown && sOpt.pageInside);
    Touch(0); assert(sOpt.sel == PT_OPT_MAX && sOpt.pageInside);
    Lift(); assert(sOpt.sel == PT_OPT_MAX && !sOpt.pageDown);
    Touch(1); Lift(); assert(sOpt.sel == 2 * PT_OPT_MAX);
    Touch(1); Lift(); assert(sOpt.sel == 0);       /* the last page wraps to the first */
    /* a row's value: on release */
    onPage = 0; onRow = 2; Touch(1);
    assert(stepped == 0 && sOpt.sel == 2);
    Lift(); assert(stepped == 1);
    return 0;
}
''')

    def test_choice_page_tap_off_is_b(self):
        """A choice of the field as a page: a row is asked of FireRed's menu and taken with its A on release; a tap off the
        menu's window is the menu's B on release (as off the Bag's menu), and nothing when the touch slid onto a row."""
        bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='latin-1')
        touch = function(bottom, 'ProcessChoiceTouch')
        self.run_c('''
#include <assert.h>
#include <stdbool.h>
#include "3ds_input.h"
#include "3ds_poketouch.h"
#define K_A (1u << 0)
#define K_B (1u << 1)
enum { MODE_CHOICE = 9 };
static struct { int down, inside, code; } sChoiceTap = {0, 0, -1};
static int sHaveShown = 1, hitCode = -1, asked = -2;
static unsigned short sRouteKey;
static struct { int mode; CtrPokeTouchView touch; } sShown;
int CtrFieldChoice_Ready(void) { return 1; }
void CtrMenu_Ask(int row) { asked = row; }
int CtrPokeTouch_ChoiceHit(const CtrPokeTouchView *v, int x, int y) { (void)v; (void)x; (void)y; return hitCode; }
''' + touch + '''
static CtrInput in;
static void Touch(int down) { in.touchActive = 1; in.touchDown = down; ProcessChoiceTouch(&in); }
static void Lift(void) { in.touchActive = in.touchDown = 0; ProcessChoiceTouch(&in); }
int main(void) {
    sShown.mode = MODE_CHOICE; sShown.touch.choice.on = 1;
    hitCode = PT_CHOICE_HIT_BACK; Touch(1); assert(!sRouteKey);
    Lift(); assert(sRouteKey == K_B && asked == -2);
    sRouteKey = 0; hitCode = PT_CHOICE_HIT_ROW + 2; Touch(1); Lift();
    assert(sRouteKey == K_A && asked == 2);
    sRouteKey = 0; asked = -2; hitCode = PT_CHOICE_HIT_BACK; Touch(1); hitCode = PT_CHOICE_HIT_ROW; Touch(0); Lift();
    assert(!sRouteKey && asked == -2);
    return 0;
}
''')


if __name__ == '__main__':
    unittest.main()
