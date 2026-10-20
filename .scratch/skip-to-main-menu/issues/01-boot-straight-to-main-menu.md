# 01: Boot straight to the main menu (existing save)

Spec: `../spec.md`

**What to build:** Power-on with an existing save lands on the main menu with the cursor on RTC 101: no intro FMV, no title screen, no attract-loop demos, and no input needed. The main menu behaves as if reached from the title. The change is training-build only and the matching build stays byte-identical. `boot-to-main-menu` is retimed with no `start` presses (main menu ready a measured number of frames after power-on, keeping the 10-frame load margin), and `e2e-title` / `e2e-smoke` are retargeted to the main menu since the title is no longer reachable.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [ ] `run --scenario boot-to-main-menu` reaches the main menu with no input before it; a shot shows the cursor on RTC 101.
- [ ] `main-menu-rtc-101` passes (A on the first frames after the menu is ready opens RTC 101; `menu menu=main choice=rtc_101` is logged).
- [ ] `e2e-title` and `e2e-smoke` test the main menu instead of the title screen, and the opt-in end-to-end test passes.
- [ ] No demo or title screen appears if the game is left idle on the main menu.
- [ ] `dev.py check` passes (matching build byte-identical).
- [ ] Any Dolphin launched is closed with `dev.py stop`.

## Answer

Boots straight to the main menu: the boot mode now goes to the menu, runs what the title's Start did (menu music, audio group, unlock checks). Existing save: menu takes input from frame 140; `boot-to-main-menu` waits 150. `main-menu-rtc-101` passes live (`choice=rtc_101`). `e2e-title` became `e2e-main-menu`. B on the main menu no longer returns to the title (it did `GM_TITLE`); it now does nothing in the training build. `dev.py check` passes.
