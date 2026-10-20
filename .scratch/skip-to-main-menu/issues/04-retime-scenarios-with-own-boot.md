# 04: Retime scenarios that spell out their own boot

Spec: `../spec.md`

**What to build:** Scenarios that do not include the shared boot prefixes work on the new, shorter boot timeline: `css-pick-cpu-mewtwo`, `custom-drill-opponent-to-jigglypuff`, `drill-history-saved-capped`, `drill-history-saved-reload`, `drill-history-window`, `locked-camera-ledges`, `tech-option-miss-hit` and `tech-option-miss-player-right`. Presses that wait on a load sit about 10 frames or more after the earliest frame measured to work.

**Blocked by:** 01, 02, 03

**Status:** resolved

- [ ] Each listed scenario runs to its shots and expected rep-log lines.
- [ ] None contains a `start` aimed at the intro or title, or an `a` aimed at an unlock notice.
- [ ] The 10-frame load margin is kept (measured earliest frame noted in each scenario's comment).
- [ ] Any Dolphin launched is closed with `dev.py stop`.

## Progress

The three history scenarios that spelled out their own boot now include `boot-to-main-menu`. Not yet re-run live: `css-pick-cpu-mewtwo`, `custom-drill-opponent-to-jigglypuff`, `drill-history-saved-capped`, `drill-history-saved-reload`, `drill-history-window`, `locked-camera-ledges`, `tech-option-miss-hit`, `tech-option-miss-player-right`.

## Answer

All eight scenarios were played live on the new boot and reached their shots and expected rep-log lines. The `drill-history-saved-capped` run needs `--keep-card` after `drill-history-saved-reload` (it then logs `history_graph attempts=120 kept=100`); its comment counts were corrected.
