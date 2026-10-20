#ifdef TRAINING_BUILD

#include "boot_menu.h"

#include <melee/gm/gm_16F1.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/lb/lbaudio_ax.h>

/// Number of unlockable items (the unlock notices' index range).
#define UNLOCK_ITEM_COUNT 0x42

/// Marks the unlock notices and trophy notices queued by the unlock checks as
/// shown: the items stay unlocked, only the pending flags go. The notice scene
/// also records each trophy in the trophy tables (Toy_SetUnlockState), which
/// needs the trophy data loaded and is not wanted here: a trophy nobody sees
/// need not be recorded, and its notice flag is raised again and cleared again
/// on the next boot.
static void training_boot_consume_notices(void)
{
    int i;

    gm_80172174(); /* trophy notices */

    for (i = 0; i < UNLOCK_ITEM_COUNT; i++) {
        gmMainLib_8015D8B0(i); /* unlock notices */
    }
}

void training_boot_enter_main_menu(void)
{
    /* The title screen's Start press: the menu's music and audio group... */
    gmMainLib_8015ECBC();
    lbAudioAx_80026F2C(0x1C);
    lbAudioAx_8002702C(0xC, 0);
    lbAudioAx_80027168();
    lbAudioAx_80027648();

    /* ...and the unlock checks that queue the notices (random stage select,
     * sound test, all characters, all stages, trophies) on a new save. Every
     * boot, so nothing queued ever reaches the main menu. */
    gm_80173EEC();
    gm_80172898(0x100);
    training_boot_consume_notices();
}

#endif
