#ifdef TRAINING_BUILD

#include "new_save.h"

#include <melee/gm/gm_1601.h>

/// Unlocks every character and every stage in the in-memory save data, just
/// before the "create Game Data" step writes it to the card (ADR-0006: a new
/// game ID starts from a blank save). Trophies, collectibles and the feature
/// bitmask (which holds the modes) are left as the new save has them. Called
/// from gm_Scene_MemCard_OnFrame.
void training_new_save_unlock_all(void)
{
    gm_80164F18(); /* all characters */
    gm_8016468C(); /* all stages */
}

/// The save's comment title: two 32-byte lines like the vanilla title, so the
/// training build's save is told apart from vanilla Melee's in a card manager
/// (ADR-0006). Called from lb_8001C658 in place of the vanilla title.
const char* training_new_save_title(void)
{
    return "RTC Training                    Game Data";
}

#endif
