#ifndef CTR_TEXT_CASE_VIEWS_H
#define CTR_TEXT_CASE_VIEWS_H

#include "3ds_text_case.h"
#include "3ds_poketouch.h"

/*
 * The names FireRed's text gives the lower screen's views still in the ember look, in proper case (3ds_text_case.h).
 * Nicknames, the player's and the rival's names and the box names are what the player typed and stay as they are. The
 * red device's views (HOME, the battle, the Party, the Summary, the Bag, OPTIONS, SAVE, the Trainer Card, the letter, the
 * Pokedex) are not cased: they draw FireRed's names in capitals (decision 5; bar[] stays cased for the ember views, and
 * the red device's draw it through DevUpper). The Bag's item names (and a case's moves) are cased where they are read
 * (3ds_bottom.c, CtrBottomBag_ItemName) for the bars of the ember views; the Bag's page upper-cases them again.
 */
#define CTR_CASE(f) CtrTextCase(f)
#define CTR_CASE_ALL(a) do { for (size_t i_ = 0; i_ < sizeof(a) / sizeof((a)[0]); ++i_) CtrTextCase((a)[i_]); } while (0)

/* The field's views in the ember look. */
static inline void CtrTextCase_Touch(CtrPokeTouchView *v)
{
    CTR_CASE_ALL(v->bar);
    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        CTR_CASE(v->learn.row[i].name);
        CTR_CASE(v->learn.row[i].typeName);
    }
    /* The move relearner: its moves (and CANCEL), their types, the question and the description; the Pokemon is a nickname. */
    CTR_CASE(v->relearn.question);
    CTR_CASE(v->relearn.desc);
    for (int i = 0; i < PT_RL_ROWS; ++i) {
        CTR_CASE(v->relearn.row[i].name);
        CTR_CASE(v->relearn.row[i].typeName);
    }
    for (int i = 0; i < PT_CHOICE_MAX; ++i)
        CTR_CASE(v->choice.label[i]);
}

#undef CTR_CASE
#undef CTR_CASE_ALL

#endif
