# 02: Settings menu with the tech option pool

**What to build:** Choosing Custom drill opens a settings menu (built with drill-select's menu component) before the stage select. It has a Start row and one toggle row per tech option. Trying to empty the option pool is refused with an on-screen message. Settings last for the session, so restarting or re-entering Custom drill shows the previous settings.

**Blocked by:** 01

**Status:** done

- [x] Each tech option can be toggled in the menu and the drill uses only enabled options
- [x] Disabling the last enabled option is refused (shot)
- [x] Settings persist through restart and re-entering Custom drill in the same session
- [x] Shot of the settings menu
- [x] Rep log `drill_start` reflects the chosen options
