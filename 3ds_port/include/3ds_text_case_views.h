#ifndef CTR_TEXT_CASE_VIEWS_H
#define CTR_TEXT_CASE_VIEWS_H

#include "3ds_text_case.h"
#include "3ds_poketouch.h"

/*
 * The bar's game names in proper case (3ds_text_case.h), as v0.6.0's ember look showed them. Nicknames, the player's and
 * the rival's names and the box names are what the player typed and stay as they are. Every view is the red device's
 * now and draws FireRed's names in capitals (decision 5): bar[] is still cased here, and every view draws it through
 * DevUpper. The Bag's item names (and a case's moves) are cased where they are read (3ds_bottom.c,
 * CtrBottomBag_ItemName); the Bag's page upper-cases them again.
 */
#define CTR_CASE_ALL(a) do { for (size_t i_ = 0; i_ < sizeof(a) / sizeof((a)[0]); ++i_) CtrTextCase((a)[i_]); } while (0)

/* The field's views' bar. */
static inline void CtrTextCase_Touch(CtrPokeTouchView *v)
{
    CTR_CASE_ALL(v->bar);
}

#undef CTR_CASE_ALL

#endif
