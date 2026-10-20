#ifndef TRAINING_BOOT_MENU_H
#define TRAINING_BOOT_MENU_H

#ifdef TRAINING_BUILD

/// What the title screen's Start press does before the main menu loads,
/// without the notices: picks the menu's music, loads its audio, and runs the
/// unlock checks, then marks every notice they queued as already shown instead
/// of routing to the notice scene. Called from the boot mode's leave callback
/// (gmboot.c), once the create-save step is over.
void training_boot_enter_main_menu(void);

#endif

#endif
