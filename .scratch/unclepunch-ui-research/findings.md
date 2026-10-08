# Findings: how UnclePunch's training pack does menus and feedback

Spec: [spec.md](spec.md). UnclePunch source is read from `ref/Training-Mode` (gitignored; clone of `https://github.com/UnclePunch/Training-Mode`, commit `fa0b9ef4ecb587e20dcce4d2d8aac5b688a42e72`, 2021-02-16, which is also the upstream `master` tip as of 2026-10-08).

## Summary

<!-- ticket 07 -->

### Verdict on the Event Match hunch

**Partly right.** UnclePunch did take over Event Match, but only for getting into its events and picking one. The menus and feedback players see during a drill are UnclePunch's own code drawn over a frozen match.

- **Confirmed: mode entry and the event list.** The 1P menu's Event Match entry, the Event Select Screen (ESS), the character select screen (CSS), the match setup and the "back to the list after a run" routing are all retail Event Match, patched. The Minigames are page 0 of the patched Event Match list, not a separate menu. Evidence: [Menus](#menus) ("Event selection: the retail Event Match screen, patched") and [Mode entry](#mode-entry-pause-restart-and-return-to-menu) ("Route through Event Match", with a step-by-step table from boot to event start).
- **Confirmed: event progress.** Event high scores and "cleared" bits reuse Event Match's own save fields. Evidence: [Saving](#saving-settings-and-progress), item 2.
- **Refuted: the in-event menu.** The Start menu inside an event (Lab options, Exit and so on) is UnclePunch's own C framework (`EventMenu`) on the game's text library, not an Event Match screen. Evidence: [Menus](#menus) ("In-event menu") and [Pause](#pause).
- **Refuted: generic feedback.** The on-screen messages (frame advantage, L-cancel and so on) are set up on every match start and work in any mode. Only the timing-bar HUDs belong to the events. Evidence: [Per-rep feedback](#per-rep-feedback) ("Messages", second bullet).
- **No bearing on tech chasing.** The nearest events (Lab, Combo Training) are entries in the Event Match list, but UnclePunch has no tech-chase drill and never hides a fighter. Evidence: [Tech-chase events and vanish](#tech-chase-events-and-vanish) ("Short answer").

For us this means: taking over Event Match is a proven route for the later **Game mode of its own** item, but it is not needed for drill-select or rep-feedback, which we build inside Training Mode (ADR-0003).

### The toolchain gap in one rule

UnclePunch is an injection on the retail disc. It patches instructions at fixed addresses, jumps past retail code, reaches its own C through a function table at an `rtoc` offset, loads its events as relocatable `.dat` files (MexTK/m-ex), and reads game structs through hand-written overlays at raw offsets. We compile into the game itself. Each of those becomes something plainer:

| UnclePunch | Ours |
| --- | --- |
| ASM branch injected at an address | One `#ifdef TRAINING_BUILD` line in the named decomp function, calling into `src/training/` (`docs/developer-workflow.md`, hooks section) |
| Function table at an `rtoc` offset, `MEX_LoadRelArchive` event files | Ordinary linked C; drills are our C table |
| Raw offsets and MexTK structs (`0x680`, `cpu.ai`, `Memcard`) | Named fields in `src/melee/ft/types.h` and `src/melee/gm/types.h` |
| `HSD_Update->onFrame` / GObj proc ordering | Our own GObj proc, or a hook at the exact call site |
| UnclePunch's `.dat` models (menu backdrop, HUD panels) | Not copyable (no licence) and we have no model pipeline: text-only or GX primitives |

Because our hooks should stay one guarded line, any recommendation below that "guards" an upstream function means a one-line call into `src/training/`, not a rewrite of that function. That rules out one idea: patching the game's ASCII-to-SIS converter `HSD_SisLib_803A67EC` the way UnclePunch does would be several edits inside a matching function. **Decision (asked for by [Per-rep feedback](#per-rep-feedback), recommendation 5):** keep converting strings ourselves, in one shared helper in `src/training/`.

### Carries over directly

These are calls into the game or facts about it. We call the same functions by their decomp names and reimplement the ideas in our own code (see [Licence](#licence): no copying).

In the "Downstream spec" columns, rep-feedback, drill-select, tech-options and tech-vanish are specs under `.scratch/`. Game mode of its own, Memory card saves and Missed-tech getup options are items under "Later" in `.scratch/priorities.md`, named as there, which have no spec yet.

| Idea | Section | Downstream spec |
| --- | --- | --- |
| SisLib for all text (`HSD_SisLib_803A611C`, `803A6754`, `803A6B98`, `803A70A0`, `803A74F0`); no font of our own | [Menus](#menus), [Per-rep feedback](#per-rep-feedback) | drill-select, rep-feedback |
| Data-driven menu model: item table, 9-row scrolling window, skip disabled rows, grey disabled text, A/B/Start, sounds 2/1/0 via `lbAudioAx_80024030` | [Menus](#menus) | drill-select |
| Pause without the retail pause UI: the game's own pause flag, sound and HUD calls, with `rules.disable_pausing` so we own Start (the calls are listed in [Pause](#pause)) | [Pause](#pause) | drill-select |
| Pause conventions: any human port's Start opens, only Start closes, B backs out without unpausing, the pausing port drives, the menu ends in an exit item | [Mode entry](#mode-entry-pause-restart-and-return-to-menu) | drill-select |
| Per-player text anchored at `ifAll_GetPlayerHUDPosition`; two-line "label / value" format; soft green/red/yellow/white; 1-based frames; "Nf Early / Perfect / Nf Late"; ~120-frame lifetime with same-kind replacement | [Per-rep feedback](#per-rep-feedback) | rep-feedback |
| Stretched "-" glyph as a text-only backdrop | [Per-rep feedback](#per-rep-feedback) | rep-feedback |
| Entering through `GM_EVENT` and its scene states, CSS type 0x17 (player + CPU), an added `GS_SSS` state, our own `rules.on_match_start` | [Mode entry](#mode-entry-pause-restart-and-return-to-menu) | Game mode of its own |
| Restart as `OUTCOME_RETRY` → `gm_SetNextGameModeStateId(1)`; quit as normal end → `GM_MENU` | [Mode entry](#mode-entry-pause-restart-and-return-to-menu) | Game mode of its own |
| Small settings in `GmSaveData.padding_trophy_flags`, riding on the game's own `lbCardGame_SaveChanges`; defaults at "no save"; bigger data in its own card file via `lb_8001BB48` / `lb_8001BF04` | [Saving](#saving-settings-and-progress) | Memory card saves |
| Getup options fed as CPU input on `DownWait`; roll stick re-read every frame relative to the player | [Tech-chase events and vanish](#tech-chase-events-and-vanish) | Missed-tech getup options, tech-options |
| Tech-input timers `fp->x680`/`fp->x684` and the `ftCo_800986B0` tech check, as a fallback for forced techs | [Tech-chase events and vanish](#tech-chase-events-and-vanish) | tech-options (ADR-0004) |

### Needs translating

| UnclePunch | Our form | Section | Downstream spec |
| --- | --- | --- | --- |
| ESS text and page hooks in `mnEvent_8024D15C`, `mnEvent_8024D7E0`, `mnEvent_8024D864` | Guarded hooks in `src/melee/mn/mnevent.c`, only if we take over Event Match | [Menus](#menus) | Game mode of its own |
| `evMenu.dat` backdrop, row boxes, highlight bar | Text colour for the highlight first; `HSD_Text` background or GX primitives later | [Menus](#menus) | drill-select |
| Dedicated menu/message camera on its own GX link | Only if our canvas turns out hidden under the game; ours already draws | [Menus](#menus), [Per-rep feedback](#per-rep-feedback) | drill-select, rep-feedback |
| OSD (on-screen display) messages fired by ASM hooks in fighter state functions | Calls from our rep logic in `src/training/tech_chase.c`; no game hooks needed for our feedback | [Per-rep feedback](#per-rep-feedback) | rep-feedback |
| HUD-panel models, life bar and colour flash from event `.dat` files | Text-only stand-ins | [Per-rep feedback](#per-rep-feedback) | rep-feedback |
| ASCII converter patch in `HSD_SisLib_803A67EC` | Our own conversion helper (decision above) | [Per-rep feedback](#per-rep-feedback) | rep-feedback |
| `onEnterVs`+0x3B8 jump to `EventInit`; A+B branches in `onExitVs`; patched `modes[GM_EVENT]` pointer; spoofed current-mode byte | One-line guarded hooks in `src/melee/gm/gmevent.c` / `gmmenumode.c` calling into `src/training/`; an edited `GameModeState` array | [Mode entry](#mode-entry-pause-restart-and-return-to-menu) | Game mode of its own |
| Hidden A+B "runback" chord, scene reload | An explicit Restart drill item that restarts in place (`start_drill`) | [Mode entry](#mode-entry-pause-restart-and-return-to-menu) | drill-select |
| Overlay struct at raw card offsets; branch patches in `gm_Scene_MemCard_OnFrame`, `lb_8001C658`, `getCurrentIcon` | Named `TRAINING_BUILD` fields carved from the padding (size unchanged); guarded hooks in those functions | [Saving](#saving-settings-and-progress) | Memory card saves |
| A separate game code (GTME) and so a separate save file | **Does not carry over.** We run as GALE01 and share the player's real save; check live that vanilla still loads it | [Saving](#saving-settings-and-progress) | Memory card saves |
| CPU logic in a GObj proc with CPU kind 15 | Keep our hook in `Fighter_procCpu` after the CPU tick (`training_cpu_input`) | [Tech-chase events and vanish](#tech-chase-events-and-vanish) | tech-options |
| `Hide Nametag When Invisible.asm` at `0x802FCCD8` | A guarded invisible check in `fn_802FCC44` (`src/melee/if/ifnametag.c`), only if a drill ever shows the opponent's tag | [Tech-chase events and vanish](#tech-chase-events-and-vanish) | tech-vanish |

### Open questions carried to downstream specs

- **drill-select:** whether `gm_SetDbPauseFlag(1)` fully freezes Training Mode's procs and CPU, whether `rules.disable_pausing` is the only trigger for Training Mode's own menu, and which route Training Mode takes back to its CSS ([Menus](#menus), [Mode entry](#mode-entry-pause-restart-and-return-to-menu)).
- **rep-feedback:** the developer should confirm keeping the result on screen past the reset (about 120 frames), which changes the spec's "cleared at reset" decision; and retest whether `HSD_SisLib_803A70A0` can grow a line, which would remove the placeholder rule in `src/training/score.c` ([Per-rep feedback](#per-rep-feedback)).
- **Memory card saves:** check live that a vanilla save with our padding bytes set still loads in an unmodified game ([Saving](#saving-settings-and-progress)).

## Licence

<!-- ticket 01 -->

**UnclePunch's Training-Mode repo has no licence.** There is nothing to quote:

- No `LICENSE`, `LICENCE` or `COPYING` file exists in the checkout, and none was ever added or deleted in its git history (`git log --all --diff-filter=AD -- '*LICEN*' 'COPYING*'` is empty).
- No source or text file (`.asm`, `.c`, `.h`, `.txt`, `.md`, scripts) carries a licence or copyright header. The only mention of copyright is `README.md` line 18, which is about Nintendo's code: the retail `Start.dol` contains copyrighted code, so it is not distributed.
- GitHub's API reports `"license": null` for `UnclePunch/Training-Mode` (`gh api repos/UnclePunch/Training-Mode`).

**What that permits.** With no licence, default copyright applies and the author keeps all rights: nobody may copy, distribute or create derivative works from the code without permission ([choosealicense.com: No License](https://choosealicense.com/no-permission/); [GitHub Docs: Licensing a repository](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository#choosing-the-right-license)). Publishing on GitHub only grants other users the right to view and fork the repo on GitHub under GitHub's Terms of Service, not to reuse the code elsewhere (same GitHub Docs page).

So for this project:

- **Carry-over versus translation.** Nothing carries over as code; only ideas do. In these findings "carries over directly" means an idea or game fact we reuse by calling the same game functions from our own code, and "needs translating" means the idea also has to be reshaped for our toolchain.
- **Do not copy** UnclePunch's code (ASM, C, data tables), assets (`Logos/`, `.mth` videos, `.dat` files) or text verbatim into the repo, and do not paste substantial excerpts into committed docs. Short references by path and line, as in these findings, are fine.
- **Do learn from it and reimplement.** Copyright covers expression, not ideas, methods or facts. What UnclePunch does (which game functions it hooks, which memory it reads, how its menus are laid out, what feedback it shows and when) can be studied and rewritten in our own code against our decomp's named functions. Game addresses and function behaviour are facts about Nintendo's game, not UnclePunch's work.
- **If direct reuse is ever wanted**, ask UnclePunch for permission or a licence first, and record the answer here.
- `ref/Training-Mode/MexTK/` and `Build TM Codeset/gecko.exe` are third-party tools bundled in the repo (MexTK, and JLaferri's gecko assembler per `README.md`) with their own terms; this research does not use them.

Recommendation for all downstream specs (rep-feedback, drill-select, tech-options, tech-vanish, and the Game mode of its own and Memory card saves items): cite UnclePunch as prior art by path, and write our own implementation.

## Menus

<!-- ticket 02 -->

**Short answer.** UnclePunch has two kinds of menu, built differently:

1. **Picking an event (including the Minigames page) uses the retail Event Match select screen**, patched ([verdict](#verdict-on-the-event-match-hunch)). The list, cursor, scrolling and layout are Melee's own `mnEvent` screen. ASM injections swap in UnclePunch's names, descriptions and page names, and add page switching.
2. **The in-event menu (Start during an event) is UnclePunch's own framework**: a data-driven menu written in C (`EventMenu`/`EventOption`). It draws all of its text with **the game's text library** (HSD SisLib) and uses its own 3D models for the backdrop, row boxes, highlight and scroll bar.

Neither part has its own text renderer or font. Every string goes through Melee's SisLib.

Paths below are relative to `ref/Training-Mode/`. Our equivalents were found by mapping the addresses in `MexTK/melee.link` (MexTK's name-to-address table) through `config/GALE01/symbols.txt`.

### Event selection: the retail Event Match screen, patched

- **Events and pages are C data.** `patch/tmdata/source/events.c:1112-1163` defines three pages (`Minigames_Page`, `General_Page`, `Spacie_Page`), each an array of `EventDesc` (`patch/tmdata/source/events.h:75-96`: name, description, tutorial, event file, CSS file, match settings). Getter functions (`events.c:4481-4563`: `GetEventName`, `GetEventDescription`, `GetPageName`, `GetPageEventNum`, ...) are exported through a function-pointer table at a fixed offset from `r13`/`rtoc` (`ASM/training-mode/Globals.s:1298-1305`, `MexTK/tmFunction.txt`). This is how the ASM hooks reach them.
- **The ASM hooks patch our `mnEvent` functions** (`src/melee/mn/mnevent.c`, matching), under `ASM/training-mode/Custom Events/Event Select Screen/`:
  - `Event Text/Display Custom Event Names on ESS.asm` (at `0x8024D470`, in `mnEvent_8024D15C`, the per-row builder at `mnevent.c:185-269`). It creates an `HSD_Text` with `HSD_SisLib_803A6754`, adds the ASCII event name with `HSD_SisLib_803A6B98` and colours it with `HSD_SisLib_803A74F0`. Retail uses a preset SIS string index instead (`HSD_SisLib_803A6368`, `mnevent.c:266-267`).
  - `Event Text/Display Custom Event Descriptions.asm` (at `0x8024D80C`, in `mnEvent_8024D7E0`, `mnevent.c:334-352`). It swaps the preset SIS description for UnclePunch's ASCII string, converted with `HSD_SisLib_803A67EC` (MexTK `Text_ConvertToMenuText`) into a buffer from `HSD_SisLib_Alloc`.
  - `Event Text/SkipDisplayingEventNames.asm` (`0x8024D3E8`, `mnEvent_8024D15C`) and `Event Text/OnlyDisplayEventsOnPage.asm` (`0x8024CFFC`, `mnEvent_8024CE74`) limit the rows to the current page's events. `Never Scroll If Less than 9 Events on Page.asm` and `CannotScrollPastEventsOnPage.asm` do the same for scrolling. The retail screen shows 9 rows (`MnEventData`, `src/melee/mn/mnevent.h:8-17`).
  - `Custom ESS Button Actions.asm` (`0x8024D92C`, in `mnEvent_8024D864`, the screen's input proc at `mnevent.c:405`) adds left/right to change page and L for an extra action. `Event Text/Get Custom Event Page Name.asm` supplies the page titles. It is a standalone helper placed at `0x8000552C`, over `extab` data rather than inside a game function, so there is no decomp function to map. The current page lives in UnclePunch's memory card data (`CurrentEventPage`).
- **The chosen event is read back from Event Match's own state.** `EventLoad` (`events.c:1369-1435`) takes the event ID from `memcard->EventBackup.event` and the page from `memcard->TM_EventPage`. It then loads that event's code file `TM/<eventFile>.dat`.

So the Minigames are not a separate menu. They are page 0 of the patched Event Match list.

### In-event menu: UnclePunch's own framework on SisLib

- **Data model** (`events.h:97-120`, kinds and states at `events.h:705-715`). An `EventMenu` has a title, an array of `EventOption`, a cursor, a scroll offset, a `prev` pointer for going back, and an optional per-frame `menu_think`. Each `EventOption` has a kind (`OPTKIND_MENU` submenu, `STRING` value list, `INT`, `FLOAT`, `FUNC` action), a name, a description, a value list, a `disable` flag, `onOptionChange` and `onOptionSelect` callbacks. Each event declares its menu as static tables, for example `patch/events/wavedash/source/wavedash.c:5-73` (Target/HUD/Tips/Help/Exit), or nested menus in `patch/events/lab/source/lab.c:928-1569`. `Exit` calls `Event_Exit` (`wavedash.c:113`), which ends the match back to the select screen.
- **Lifecycle.** `EventLoad` calls `EventMenu_Init(event_desc, *evFunction->menu_start)` (`events.c:1407`). Each frame, `EventUpdate` calls `EventMenu_Update` (`events.c:1437-1460`), hooked through `HSD_Update->onFrame`. `EventMenu_Init` (`events.c:3117-3154`):
  - creates a dedicated camera from the HUD's scene camera in `IfAll.dat` (the archive behind `ifAll_GetArchive`, `src/melee/if/ifall.c:128-132`) with `HSD_CObjLoadDesc`, `GObj_Create` and `GObj_SetupGXLinkMax` (MexTK `GOBJ_InitCamera`);
  - gives that camera its own GX links and priorities (`events.h:718-729`), so the menu draws above the match;
  - creates two SisLib canvases, menu and popup, with `HSD_SisLib_803A611C`.
- **Pause and unpause** (`EventMenu_Update`, `events.c:3156-3281`). Start from any human port toggles the menu, or X+D-pad-up in develop mode. That port then owns the menu. On open it builds the models and text, then freezes the match with the game's own pause flag, sound and HUD calls, and reverses them on close. The calls are listed once, in [Pause](#pause). This is "the game's pause without its pause UI", which the drill-select spec asks for.
- **Navigation** (`EventMenu_MenuThink`, `events.c:3298-3627`). Input comes from the pausing port's rapid-fire held buttons (`Pad_GetRapidHeld` = `fn_8018F674`, `src/melee/gm/gmtoulib.h:40`). Holding R switches to raw held buttons, which makes the cursor move faster.
  - Up/down (stick or D-pad) moves the cursor and skips disabled options. At most 9 rows show (`MENU_MAXOPTION`, `events.h:8`), and the list scrolls past that.
  - Left/right steps a `STRING`/`INT`/`FLOAT` value, clamped with no wrap, and fires `onOptionChange`.
  - A enters a submenu or runs a `FUNC` option. B goes back to `prev` and resets the child menu's cursor. B at the top level does nothing; the code to unpause there is commented out at `events.c:3600-3616`. Start closes the menu.
  - Sounds come from `SFX_PlayCommon` = `lbAudioAx_80024030`: 2 on move or change, 1 on confirm, 0 on back.
  - A popup value picker exists (`EventMenu_PopupThink`, `events.c:3629`; `EventMenu_CreatePopup*`, `events.c:4291-4459`), but the A press that opened it is commented out (`events.c:3515-3551`).
- **Text layout** (`EventMenu_CreateText`/`EventMenu_UpdateText`, `events.c:3911-4245`). There are four `HSD_Text` objects on the menu canvas: title, option names (left-aligned), option values (right-aligned) and description.
  - Each row is one subtext, added once with `HSD_SisLib_803A6B98`. Rows are updated in place with `HSD_SisLib_803A70A0` (MexTK `Text_SetText`), and coloured with `HSD_SisLib_803A74F0`: white when enabled, grey when disabled.
  - The description is rebuilt every update from the highlighted option's `desc`. It allows up to 4 lines split on `\n`, at most 100 characters each (`events.c:4056-4123`).
  - Positions, scales and spacing are constants at `events.h:732-825`. The title is at the top left. Rows step `MENU_TEXTYOFFSET`, and the description box sits below the rows.
  - SisLib calls use font/context index 2 throughout, where our code uses 0.
  - The MexTK `Text` struct (`MexTK/include/text.h:17-48`) is our `HSD_Text` (`src/sysdolphin/baselib/sislib.h:29-80`) under other field names.
- **Graphics** (`EventMenu_CreateModel`, `events.c:3776-3909`). The backdrop panel, row value boxes, highlight bar and scroll bar are `HSD_JObj`s loaded with `HSD_JObjLoadJoint` from UnclePunch's own asset file. `evMenu` (`events.h:31-43`) lives in `TmDt.dat`, built from `patch/tmdata/assets/evMenu.dat` by `patch/tmdata/build.bat`. Each row box is resized by moving its four corner joints. The highlight is a recoloured box at alpha 0.6, moved to `cursor * ROWBOX_YOFFSET`.
- **Escape hatch for custom screens** (`MenuData.custom_gobj*`, `events.h:143-145`; `events.c:3166-3170`). An option's `onOptionSelect` can install its own think and destroy callbacks, which take over input until they return. Lab uses this for its input display and export/keyboard screens (`lab.c:4011`, `4094-4095`, `5505-5507`), drawing extra `HSD_Text` on the same canvases.

### What carries over and what needs translating

**Carries over directly.** These are calls into the game's own code, which we can make from `src/training/` under their decomp names:

| UnclePunch (MexTK name) | Ours |
| --- | --- |
| `Text_CreateCanvas` | `HSD_SisLib_803A611C` |
| `Text_CreateText` | `HSD_SisLib_803A6754` |
| `Text_CreateText2` | `HSD_SisLib_803A5ACC` |
| `Text_AddSubtext` | `HSD_SisLib_803A6B98` |
| `Text_SetText` | `HSD_SisLib_803A70A0` |
| `Text_SetColor` | `HSD_SisLib_803A74F0` |
| `Text_SetScale` | `HSD_SisLib_803A7548` |
| `Text_SetPosition` | `HSD_SisLib_803A746C` |
| `Text_Destroy` | `HSD_SisLib_803A5CC4` |
| `Text_SetSisText` | `HSD_SisLib_803A6368` |
| `Text_ConvertToMenuText` | `HSD_SisLib_803A67EC` |
| `Text_GX` | `HSD_SisLib_803A84BC` |
| `Match_FreezeGame` / `Match_UnfreezeGame` | `gm_SetDbPauseFlag(1)` / `gm_ClearDbPauseFlag(1)` |
| `Match_HideHUD` / `Match_ShowHUD` | `ifAll_802F3394` / `ifAll_802F33CC` |
| `Match_AdjustSoundOnPause` | `lbAudioAx_80024E84` |
| `SFX_PlayCommon` | `lbAudioAx_80024030` |
| `Pad_GetRapidHeld` | `fn_8018F674` |
| `JOBJ_LoadJoint` | `HSD_JObjLoadJoint` |
| `COBJ_LoadDesc` | `HSD_CObjLoadDesc` |
| `GObj_AddGXLink` | `GObj_SetupGXLink` |
| `File_GetSymbol` | `HSD_ArchiveGetPublicAddress` |

`src/training/score.c` and `src/training/tech_chase.c` already use the SisLib calls this way. The menu's data model, navigation rules, sounds, colours and layout are ideas, so we can reimplement them freely.

**Needs translating** from UnclePunch's injection toolchain:

- **Hooks.** ASM injections at fixed addresses in `mnEvent_*` become small `TRAINING_BUILD`-guarded edits in `src/melee/mn/mnevent.c`. That matters only if we ever take over Event Match, which is the **Game mode of its own** item.
- **Loading code.** The function table at a `rtoc` offset and the separately loaded MexTK relocatable `.dat` files (`TmDt.dat`, `TM/Ev*.dat` via `MEX_LoadRelArchive`) become ordinary linked C in our build.
- **Per-frame update.** The `HSD_Update->onFrame` hook becomes our own GObj proc.
- **Menu graphics.** The `evMenu.dat` models are UnclePunch's assets, which we may not copy, and we have no custom-model pipeline. Do the backdrop and highlight another way: with `HSD_Text` `bg_color` behind a text block, with a coloured highlight line drawn as text, or with GX primitives. Start with text-only.

### Recommendation for [drill-select](../drill-select/spec.md)

Build drill-select's "small reusable menu component" as a cut-down `EventMenu` in our own code, inside Training Mode (ADR-0003), and leave the Event Match screen alone. Hooking `mnEvent` belongs to the later **Game mode of its own** item, if that item takes over Event Match the way UnclePunch did.

Specifically:

- **Data and layout.** Use a static item table (label, description, enabled flag, optional value list, confirm callback), generated from the drill table. Draw it on one SisLib canvas as four `HSD_Text` objects: title, names, values and a description area. Update rows in place with `HSD_SisLib_803A70A0`, and grey out disabled rows such as wrong-stage drills.
- **Rows.** Show a 9-row window that scrolls, with the cursor skipping disabled rows.
- **Controls.** Use rapid-held D-pad/stick input from the pausing port: A to confirm, B to go back, Start to toggle. Play `lbAudioAx_80024030` sounds 2, 1 and 0 for move, confirm and back.
- **Pausing.** Freeze and unfreeze the way [Pause](#pause) describes.
- **Highlight.** Start text-only, with the highlight shown by text colour. Draw backdrop boxes later if needed.

One thing to verify while implementing: whether bit 1 of the debug-pause flag fully freezes Training Mode's own procs and CPU, and how it interacts with Training Mode's Start handling.

## Per-rep feedback

<!-- ticket 03 -->

Paths below are relative to `ref/Training-Mode/`; `OSD/` means `ASM/training-mode/Onscreen Display/`, and `events.c`/`events.h` mean `patch/tmdata/source/events.{c,h}`. UnclePunch names game functions as in `MexTK/melee.link`; the decomp name is given next to each.

UnclePunch gives per-rep feedback in three ways: **messages** (short coloured pop-ups above a player's damage HUD, used by the "OSD" toggles in any match), **tips** (one coaching box at a time), and **event HUDs** (a fixed panel with a timing bar, in its own C events: L-cancel, ledgedash, wavedash).

### Messages (the OSD system)

- **One function, called from game hooks.** `Message_Display(kind, queue, colour, fmt, ...)` (`events.c:2505-2608`) is exported to the ASM through a function table (`events.c:1188`; `ASM/training-mode/Globals.s:2121-2125`, `TM_MessageDisplay` at `:1321`). Each OSD is an ASM hook in a fighter state function that checks its toggle bit in the memcard bitfield (`Globals.s:1166`, IDs at `:1178-1206`) and then calls it. Examples: frame advantage in the shield-hit callback (`OSD/Additional Playerblock Variables/Frame Advantage/Initialize Frame Advantage Think.asm`, hook `80092e68` in decomp `ftCo_80092E50`, `src/melee/ft/kinds/ftCommon/ftCo_Guard.c:631`); L-cancel on aerial landing (`OSD/L Cancel Display NEW.asm`, hook `8008d770` in `ftCo_LandingAir_EnterWithMsidLag`, `ftCo_LandingAir.c:59`); wavedash in `ftCo_80099D70` (`OSD/Wavedash Display/Wavedash Timing Display.asm`, `ftCo_EscapeAir.c:114`); SDI in `ftCo_Damage_OnEveryHitlag` (`ftCo_Damage.c:500`); fastfall in `ftCommon_CheckFallFast` (`src/melee/ft/ftcommon.c:513`).
- **Set up on every match start, not only in events.** `Message_Init` runs from `OnStartMelee` (`events.c:1556-1562`), hooked at `8016e8c8` (`ASM/training-mode/Hooks/OnStartMelee.asm:1`), which is inside decomp `fn_8016E730` (`src/melee/gm/gmvs.c:1994`). So OSD messages are **not** part of the Event Match takeover: they work in any match, and their toggles are set from UnclePunch's custom stage-select screen (`OSD/Toggle UI/`). Only the event HUDs below live inside the Event Match events ([verdict](#verdict-on-the-event-match-hunch)).
- **Placement.** One queue per player (6) plus one general queue (`events.h:839-857`). A player's queue is anchored at that player's damage HUD position plus 8 units up (`Match_GetPlayerHUDPos` = decomp `ifAll_GetPlayerHUDPosition`, `src/melee/if/ifall.c:69`; `MSG_HUDYOFFSET`, `events.h:898`; used at `events.c:2701-2707`). The general queue sits top-left at (-21, 18.5) (`events.h:878`). Up to 5 messages stack, newest at the anchor, older ones pushed up 5.15 units each (`events.h:877`, `events.c:2809-2875`). Text is centred (`align = 1`), kerned, and scaled 1.4 (`events.c:2536-2548`, `events.h:903`). Drawn through its own camera on GX link 13 so it sits over the game (`events.c:2474-2503`, `events.h:910-914`).
- **Format.** Up to 3 lines of 32 characters (`events.h:896-897`), each line its own subtext (`events.c:2597-2601`). Every OSD uses two lines, a label then a value: "Frame Advantage / N Frames", "Act OoShield / Frame N", "L-Cancel N% / Frame N/7" or "No Press", "Wavedash Frame: N / Angle: X", "SDI Inputs N/M" (string literals at the bottom of each `OSD/*.asm`). Frame numbers are shown 1-based (for example `addi r7,r7,1` in `Display OOS Frame Count Standalone.asm:87`, `+1` in `L Cancel Display NEW.asm:75`).
- **Colours.** Four: white (255,255,255), green (141,255,110), red (255,162,186), yellow (255,240,0) (`events.h:880-891`, mirrored in `Globals.s:2129-2132`). They are soft pastels, not pure RGB. Green means success or frame-perfect, red failure, yellow a partial (UCF-only dashback), white neutral information. Frame advantage is green at -6 or better, else white (`Initialize Frame Advantage Think.asm:250-257`); act-out-of-shield is green only when frame-perfect (`Display OOS Frame Count Standalone.asm:70-78`). Where a message has a label, only one line carries the colour: L-cancel colours the message then resets line 0 to white (`L Cancel Display NEW.asm:78-85`); dashback does the reverse and colours only line 0 (`OSD/Dashback/Dashback Display - Static Function.asm:83-106`). Per-line colour is `Text_SetColor` = decomp `HSD_SisLib_803A74F0(text, subtext, GXColor*)` (`src/sysdolphin/baselib/sislib.h:111`).
- **Duration and motion.** Each message lives 120 frames (`MSG_LIFETIME`, `events.h:895`). It slides in over 6 frames with a smoothstep curve and collapses vertically over 6 frames on expiry (`events.h:893-894`; `BezierBlend`, `events.c:2886-2889`; manager `events.c:2610-2774`). A bar in the message's background shrinks with remaining life (`events.c:2730-2733`). A new message of the same kind replaces the old one; if the old one was newest, the new one appears without animating (`events.c:2818-2843`). Under frame advance nothing animates and expired messages vanish at once (`events.c:2656`, `:2727`). The message camera does not draw while the game is paused (`events.c:2877-2883`). The pause test `Pause_CheckStatus` is decomp `gm_GetDbPauseFlag` (`src/melee/gm/gmscene.h:9`), which `src/training/tech_chase.c:1072` already uses.
- **Background.** The current version loads a backdrop model from UnclePunch's own data file (`stc_event_vars.menu_assets->message`, `events.c:2518`). The older ASM version drew the backdrop with the text library alone: a "-" glyph coloured black and stretched to a box behind the text (`OSD/Message Display System/Display Message 2.0.asm:192-213`).

### Tips

`Tip_Display(lifetime, fmt, ...)` (`events.c:2971-3082`) shows one left-aligned box of up to 5 lines of 48 characters, which animates in, waits, and animates out (`events.c:2902-2969`). A new tip is refused while one is on screen (`events.c:2983-2996`). Events use tips for coaching that appears only after a mistake repeats: the L-cancel event shows "press earlier" or "don't forget to fastfall" after the same mistake 3 times, for 5 s (`patch/events/lcancel/source/lcancel.c:550-608`). The ledgedash event uses short 1.7 s tips for misinputs with a frame count, for example "Fell N frames early" and "Jumped N frame(s) early" (`patch/events/ledgedash/source/ledgedash.c:1176-1206`, `ledgedash.h:9`).

### Event HUDs (timing bar)

The L-cancel, ledgedash and wavedash events each draw a fixed panel (a model from the event's data file) with a row of frame boxes and an arrow, plus text lines that are updated in place on each rep and stay until the next one (`lcancel.c:245-317`, `:345-438`).

- **Early/late wording.** Wavedash: "Nf Early", "Perfect", "Nf Late", measured from the centre of a 15-frame bar (`patch/events/wavedash/source/wavedash.c:411-416`, `wavedash.h:9`). L-cancel: "Nf/7f", "No Press" (30 frames or more), "Auto-canceled" (`lcancel.c:368-412`). Each also shows a running success rate ("%.1f%%", `lcancel.c:397-398`; `wavedash.c:423`).
- **Arrow and bar.** The arrow slides to the box for the input frame over 4 frames (`lcancel.h:52-54`, `lcancel.c:380-434`). Ledgedash colours each of its 30 boxes by what the player did on that frame (wait, fall, jump, airdodge, attack, landing) and paints the frames still intangible in blue (`ledgedash.c:4-20`, `:368-398`).
- **Flash.** The whole panel plays a success or failure colour animation (`hudmatanim[is_fail]`, `lcancel.c:400-403`; `wavedash.c:290`, `:306`).
- **Sound.** Success plays sound 303, failure the common sound 3 (`lcancel.c:361-365`; `wavedash.c:297`, `:307`; `ledgedash.c:414`). `SFX_Play`, `SFX_PlayRaw` and `SFX_PlayCommon` are decomp `Ground_801C53EC`, `HSD_AudioSFXStartParam` and `lbAudioAx_80024030` (`src/melee/gr/ground.h:152`, `src/sysdolphin/baselib/axdriver.h:56`, `src/melee/lb/lbaudio_ax.h:38`). Sound is out of scope for rep-feedback, but this is the route if it comes back.

### Text library and decomp names

UnclePunch draws all of this with the game's own text library (SisLib):

| UnclePunch (`MexTK/melee.link`) | Decomp | Note |
|---|---|---|
| `Text_CreateCanvas` 803a611c | `HSD_SisLib_803A611C` (`sislib.c:490`) | The decomp names its arguments (`font_idx, parent_gobj, class_id, p_link, p_prio, gx_link, gx_prio, render_prio`); `src/training/score.c` treats them as unknown. UnclePunch passes `(2, gobj, 14, 15, 0, 13, 81, 19)` (`events.c:2494`), the same shape as the game's own HUD canvas in `un_802FF498` (`src/melee/if/if_2FF2.c:183`). |
| `Text_CreateText` 803a6754 | `HSD_SisLib_803A6754` | |
| `Text_AddSubtext` 803a6b98 | `HSD_SisLib_803A6B98` (`hsd_3A64.c`) | One subtext per line. |
| `Text_SetText` 803a70a0 | `HSD_SisLib_803A70A0` | UnclePunch makes lines longer than the first text, for example "-" to "Auto-canceled" (`lcancel.c:306`, `:411`), and the decomp shows a path that grows the buffer (`hsd_3A64.c:376-401`). The "an update cannot exceed the text first drawn" placeholder rule in `src/training/score.c` may be unnecessary; worth retesting. |
| `Text_SetColor` 803a74f0 | `HSD_SisLib_803A74F0` | Colour for one subtext. |
| scale 803a7548 | `HSD_SisLib_803A7548` | |
| `Text_Destroy` 803a5cc4 | `HSD_SisLib_803A5CC4` | |
| `Match_GetPlayerHUDPos` 802f3424 | `ifAll_GetPlayerHUDPosition` | Anchor above the damage HUD. |
| `Pause_CheckStatus` 801a45e8 | `gm_GetDbPauseFlag` | |

UnclePunch writes plain ASCII format strings such as `"%df Early"`. It can do that because it patches the game's ASCII-to-SIS converter, decomp `HSD_SisLib_803A67EC` (`src/sysdolphin/baselib/hsd_3A64.c:136`). It adds a lookup for `! / . = + % ' > < _ ^ ? | ( )` and newline, and changes how spaces and numbers are encoded (`ASM/training-mode/Misc/Text Modifications/`, hooks `803a684c`, `803a6860`, `803a6868`, `803a69c0`, all inside that function). Without the patch our code writes full-width Shift-JIS by hand (`src/training/score.c:13-21`, `tech_chase.c:753`).

### What carries over and what needs translating

- **Carries over directly** (game functions and facts that our build calls by name): the SisLib calls in the table; anchoring per-player text at `ifAll_GetPlayerHUDPosition`; per-line colour with `HSD_SisLib_803A74F0`; the colour scheme; the "label / value" two-line format with 1-based frame numbers; the "Nf Early / Perfect / Nf Late" wording; the lifetime, the same-kind replacement, and frame-advance behaviour as a design; and the stretched-glyph backdrop, which needs only the text library.
- **Needs translating:** ASM hooks at fixed addresses become `TRAINING_BUILD` calls in the decomp functions named above (see `docs/developer-workflow.md`). The C events reach the game only through MexTK's function table and `event_vars`, which become plain function calls. The backdrop and HUD-panel models, lifetime bar and colour flash come from UnclePunch's `.dat` files (`evMenu`, event assets), which we neither have nor may copy, so they would need our own assets or a text-only stand-in. The ASCII converter patch would be a `TRAINING_BUILD` change inside `HSD_SisLib_803A67EC` (a matching-build function), or we keep converting strings ourselves. A dedicated camera and GX link are only needed if our canvas turns out to be hidden under the game; ours already draws.

### Recommendation for rep-feedback

For [rep-feedback](../rep-feedback/spec.md):

1. **Place the per-rep result above the player's damage HUD**, anchored at `ifAll_GetPlayerHUDPosition(player)` plus a small upward offset, centred. This keeps it off the fighters (story 9) and is where UnclePunch players look.
2. **Use the two-line format with soft colours:** line 1 the tech option and outcome, coloured (green success, red failure, yellow void); line 2 the timing in white, for example "Frame 2/6", "8f Early", "4f Late", "No attack". Use UnclePunch's RGB values as a starting point and tune by eye. Number frames from 1.
3. **Duration:** show it for about 120 frames, or until the next rep's result replaces it, rather than clearing it at reset. Our inter-rep pause is only 60 frames (`PAUSE_FRAMES`, `src/training/tech_chase.c:63`), which is short to read two lines. UnclePunch's messages and event HUDs both outlive the reset. This changes the spec's "cleared at reset" decision, so the developer should confirm. Don't block the next rep (story 8).
4. **Optional:** a stretched black "-" glyph as a backdrop for legibility, and a running success rate in the existing score text. A UnclePunch-style tip after the same mistake 3 times in a row ("early 3 times: wait for the landing") fits as a follow-up, not v1.
5. **Text encoding:** keep the hand-written Shift-JIS helpers for v1 and move them into one shared helper as the strings grow. Patching `HSD_SisLib_803A67EC` under `TRAINING_BUILD` is the cleaner long-term fix but changes a matching function, so the [Summary](#the-toolchain-gap-in-one-rule) decides against it.

## Mode entry, pause, restart and return to menu

<!-- ticket 04 -->

Paths starting `ASM/`, `patch/`, `MexTK/`, `Build TM Codeset/` or `README.md` are under `ref/Training-Mode/`. Each `.asm` file names its injection address on line 1 ("To be inserted at …"). The addresses were mapped to our decomp's functions using `config/GALE01/symbols.txt` and the disassembly in `build/GALE01/asm/`. Everything below comes from reading the source. No live run was needed.

### Route through Event Match

UnclePunch's events are retail Event Match with its data and hooks swapped out ([verdict](#verdict-on-the-event-match-hunch)). The event list is the retail ESS with new text and pages, not a new menu. The author says so in `README.md` lines 5–6: the mod uses Melee's event mode and reworks the events. The code agrees, step by step:

| Step | Retail behaviour (our decomp) | What UnclePunch changes (`ref/`) |
|---|---|---|
| Boot | `bootOnLeave` (`src/melee/gm/gmboot.c`) calls `gm_ChangeGameModeAfterCurrentScene(mode_id)`, normally to the title screen | `ASM/Additional Codes/Boot to Event Mode/Boot to Main Menu Or CSS.asm` (at `bootOnLeave`+0x78, 801BFA20) swaps in `GM_MENU`, or `GM_VS` if X is held, so boot skips the title screen. |
| Main menu opens on the ESS | `onEnter` in `src/melee/gm/gmmenumode.c` opens the menu as `MENU_KIND_EVENT` when the previous mode was `GM_EVENT`. `mnMain_Scene_OnEnter` (`src/melee/mn/mnmain.c`, `case MENU_KIND_EVENT`) then opens the ESS directly with `mnEvent_8024E838` | `ASM/Additional Codes/Boot to Event Mode/Main Menu Starts at Event Mode.asm` (in `gm_Scene_MemCard_OnEnter`, 801B02DC) writes `GM_EVENT` (0x2B) into the scene controller's current-mode byte, so the first main menu thinks it is coming back from an event and opens on the ESS. |
| Event list | `mnEvent_8024D864` (`src/melee/mn/mnevent.c`): A calls `gm_801BEB74(event)` and then `mn_80229860(GM_EVENT)`. X/Y page by 9 | `ASM/training-mode/Custom Events/Event Select Screen/` reworks the same screen: custom names, descriptions and highscore types (`Event Text/*`), left/right to change page and L to open the OSD toggles (`Custom ESS Button Actions.asm`, at `mnEvent_8024D864`+0xC8). The page index is kept in UnclePunch's memory card data. The pages themselves are C tables in `patch/tmdata/source/events.c` lines 1112–1163, put in order at 1165–1173. |
| Mode's scene list | `gm_Mode_Event_States` (`src/melee/gm/gmevent.c`) holds CSS (`onEnterCss`/`onExitCss`) then VS (`onEnterVs`/`onExitVs`), referenced from `modes[]` in `src/melee/gm/gmscdata.c` | `ASM/training-mode/Custom Events/CSS and SSS/Add SSS To Event Match Scene List.asm` lines 6–61 (at `runGameMode`+0x11C) writes a new states pointer into `modes[GM_EVENT]` (803DB010): CSS, VS and a third state, the retail stage select screen (SSS; `gm_801B1EB8`/`gm_801B1EEC`, `GS_SSS`). The same file swaps `GM_MENU`'s states (lines 101–132) to add a credits state. |
| CSS | `onEnterCss` calls `gm_801B06B0(css, 0xE, …)` (the Event CSS) | `CSS and SSS/Load CSS + Preload CPU.asm` (at `onEnterCss`+0x20) uses CSS type 0x17 for events that choose a CPU. That is Training Mode's player+CPU CSS (`gm_801B06B0(css, 0x17U, …)` in `src/melee/gm/gmtrainingmode.c`:88). It also preloads the event's fixed CPU and stage. `CSS and SSS/Load SSS For Certain Events.asm` (at `onExitCss`+0x58) sets `next_state_id` so the SSS state comes next for events that pick a stage. `ASM/training-mode/Hooks/OnCSSLoad.asm` (in `mnCharSel_802640A0`) runs an optional per-event `cssFunction` from the event's file. |
| Match setup | `onEnterVs` reads `GmEvent.dat` (`sqEventInitDataLevelTbl`) and builds `StartMeleeData` from it | `ASM/training-mode/Custom Events/Custom Event Code - Rewrite.asm` lines 1–20 (at `onEnterVs`+0x3B8, 801BB128, just after the stage block) calls the C `EventInit` and jumps to 801BB738 (`gm_LoadRumbleEnabled`). This skips retail's player/bonus/preload setup. `EventInit` (`patch/tmdata/source/events.c` 1204–1367) fills the rules from the event's `EventMatchData`, takes fighters and stage from the CSS/SSS or the event, and points `rules.on_match_start` at its own `EventLoad` (line 1224, retail's is `fn_801BBFE8`). Events with no file still go through the old ASM path (`LegacyEvent`, line 22). |
| Event start | `rules.on_match_start` runs during VS scene setup | `EventLoad` (events.c 1369–1435) loads `TM/<event>.dat` with m-ex's `MEX_LoadRelArchive`, creates the event GObj and timer, builds the pause menu (`EventMenu_Init`, line 1407), and sets the per-frame callback to `EventUpdate` (line 1432). That callback slot is `HSD_UPDATE->onFrame`, MexTK `0x80479d58`+0x30, which is our `gm_80479D58`, set by `gm_SetPreGObjProcCallback`. Retail VS sets it to `db_RunEveryFrame` (`src/melee/gm/gmvs.c`:2002), and `EventUpdate` falls back to calling it (`Develop_UpdateMatchHotkeys`, line 1464). |

The other global hooks are in `ASM/training-mode/Hooks/`. `OnSceneChange.asm` is in `gm_801A4BD4`, which runs at every scene start (watermark, debug console). `OnStartMelee.asm` is in `fn_8016E730` during VS setup (message and tip systems). `OnBoot.asm` is in `bootOnLeave`. `Build TM Codeset/codes.json` lines 21–25 inject the whole `ASM/` folder. The assembler takes `.asm` files (`README.md` line 13), so the `.s` siblings, such as `Boot to CSS/Boot to Main Menu Or CSS.s`, are include-only or replaced blocks.

So the menu entry, list screen, CSS, SSS and results routing all belong to Event Match, kept or lightly redirected; the in-event menus do not (below, and [Menus](#menus)). Taking over a retail screen is a repeated pattern, though: the OSD-toggle menu reuses the Random Stage Select screen. `Custom ESS Button Actions.asm` lines 43–67 call `mnStageSw_80237410` with a flag in the rules struct. `ASM/training-mode/Onscreen Display/Toggle UI/Return to Event Screen When Pressing Start.asm` (in `fn_80235F80`, `src/melee/mn/mnstagesw.c`) sends Start back to the ESS on the last event with `mnEvent_8024E838`.

### Pause

There are two kinds of event, chosen per event by `EventMatchData` (`patch/tmdata/source/events.c`):

- **C events with a menu** (Lab, L-Cancel, Ledgedash, Wavedash; `Lab_MatchData` lines 27–55) set `isDisablePause = true`. This maps to `rules.disable_pausing`, which turns off both `gm_DoPauseChecksAndRoutine` and `gm_DoUnpauseChecksAndRoutine` (`src/melee/gm/gmvs.c` 1115–1231), so retail's Start does nothing. `EventMenu_Update` (events.c 3156–3281), called every frame from `EventUpdate`, takes over Start:
  - Any human port's Start toggles the menu (3182–3213). In develop mode it is X + D-pad up, like retail's debug pause. The port that paused drives the menu (3305).
  - On pause (3239–3243) it does what retail's pause routine does, minus the pause camera and the pause HUD. `Match_FreezeGame(1)` is our `gm_SetDbPauseFlag(1)` (`src/melee/gm/gmscene.c:48`), the same flag retail VS pause sets (`gm_DoPauseChecksAndRoutine(tmp, 1)`, gmvs.c:1396). `SFX_PlayCommon(5)` is the retail pause sound (`lbAudioAx_80024030(5)`). `Match_HideHUD` is `ifAll_802F3394`, and `Match_AdjustSoundOnPause(1)` is `lbAudioAx_80024E84(1)`. Unpausing (3254–3257) reverses each of these (`gm_ClearDbPauseFlag(1)`, `ifAll_802F33CC`, `lbAudioAx_80024E84(0)`). The HUD and sound calls are the ones Melee's own pause camera makes: `gmCamera_801A2800` (`src/melee/gm/gmcamera.c:264-284`) on pause and `gmCamera_801A26C0` (`gmcamera.c:229-248`) on unpause, as well as the retail unpause routine (gmvs.c 1217–1219). Addresses come from `MexTK/melee.link` lines 167–171 and 405.
  - In the menu, A opens a submenu or runs an option's function and B goes back one submenu (3489–3598). B on the top menu does **not** unpause: that code is commented out (3600–3616), so only Start closes the menu.
- **Legacy events** (the rest, e.g. `Combo_MatchData` lines 218–232) keep `isDisablePause = false`, so retail VS pause handles Start, with `isCheckForZRetry` and `isCheckForLRAStart` set. UnclePunch sets `isHidePauseHUD`, which is `rules.x3_6`. That gates `gm_801A0FEC`, the retail pause prompts, in `gm_DoPauseChecksAndRoutine`.

The bit layout of byte 3 in `EventMatchData` (the comments in `patch/tmdata/source/events.c` 36–43) matches our `rules.x3_*` fields with MSB-first bitfields: `x3_2` is Z-retry, `x3_3` shows the Z-retry prompt, `x3_4` is LRA+Start, `x3_5` shows the LRA+Start prompt, `x3_6` hides the pause HUD.

### Restart

UnclePunch has no "Restart" menu item. Restart comes in three forms:

1. **Retail Z-retry (legacy events).** While paused, Z gives `OUTCOME_RETRY` (`fn_8016CFE0`, gmvs.c ~1378–1386, gated by `x3_2`). `onExitVs` (`src/melee/gm/gmevent.c` 636–674) clears the per-run state and calls `gm_SetNextGameModeStateId(1)`, so the VS scene reloads and the CSS is skipped.
2. **"Runback" with A+B.** `ASM/training-mode/Custom Events/A+B to Reset Event/Check for Reset on LRAStart.asm` (at `onExitVs`+0xEC, the `OUTCOME_NO_CONTEST` → `GM_MENU` branch) and `Check for Reset on Game End.asm` (at `onExitVs`+0x2E0, the normal-end → `GM_MENU` branch) read all ports' held buttons (`gm_GetButtonsPressed(4)`). If A and B are both held, they jump into the retry branch (801BB7A0) instead of going to the menu. So quitting while holding A+B restarts the event. That covers LRA+Start+B, or choosing Exit with A while holding B.
3. **In-event resets in C events.** These never reload the scene. Lab saves and loads savestates on D-pad right/left (`patch/events/lab/source/lab.c` 5320–5328, using `Savestate_Save`/`Savestate_Load`, events.c 1569 and 1772). Ledgedash has an "Auto-Reset" option (`patch/events/ledgedash/source/ledgedash.c`:44).

### Return to menu

- **Exit option.** Every C event's top menu ends with "Exit", described as returning to the Event Select Screen (`patch/events/lab/source/lab.c` 980–990; `patch/events/lcancel/source/lcancel.c` 52–63). Its handler (`lab.c` 1918–1926, `lcancel.c` 225–237) sets `MATCH->state = 3` and calls `Match_EndVS`, which is our `fn_8016C7F0`. MexTK's `MATCH` is our `controller` in gmvs.c, at 8046B6A0. The match ends and `onExitVs` takes the normal-end path to `gm_ChangeGameModeAfterCurrentScene(GM_MENU)`. Because the previous mode is `GM_EVENT`, `gmmenumode.c` `onEnter` reopens the ESS on the last event (`gm_801BEB80`). Back on the ESS goes to the main menu. B on the CSS also returns to the ESS (`onExitCss`: `pending_scene_change == 2` → `GM_MENU`).
- **LRA+Start.** While the game is frozen with flag 1, retail's paused branch in `fn_8016CFE0` (gmvs.c 1351–1375) still checks LRA+Start and gives `OUTCOME_NO_CONTEST` → `onExitVs` → `GM_MENU` → the ESS. That check is gated only by `x3_4` (`isCheckForLRAStart`), not by `disable_pausing`. UnclePunch's own menu pause uses that same flag, so LRA+Start presumably also quits from UnclePunch's menu in C events, since all of them set `isCheckForLRAStart = true`.

### Ideas that carry over directly vs ideas that need translating

**Carry over directly** (these are facts about the game, and we can call the functions by name):
- Entering through `GM_EVENT`, with `gm_Mode_Event_States` and `onEnterCss`/`onExitCss`/`onEnterVs`/`onExitVs`. Retail's own Event Match label, ESS, back-out routing and "reopen on the last event" behaviour (`gmmenumode.c` `onEnter`) come for free.
- Reusing Training Mode's player+CPU CSS by passing type 0x17 to `gm_801B06B0`, and adding a `GS_SSS` state.
- Replacing `rules.on_match_start` with our own setup function, and owning the per-frame slot through `gm_SetPreGObjProcCallback`. Our existing `TRAINING_BUILD` hook at the end of VS setup (`src/melee/gm/gmvs.c`:2167) already does what UnclePunch's `OnStartMelee` does.
- Pause without the retail pause UI: set `rules.disable_pausing`, then on our own Start make the calls listed in [Pause](#pause), and reverse them on unpause.
- Restart as `OUTCOME_RETRY` → `gm_SetNextGameModeStateId(1)`, which reloads the match scene without the CSS. Quit as a normal end or `OUTCOME_NO_CONTEST` → `GM_MENU`.

**Need translating** (artefacts of injecting into a retail binary):
- Hooking mid-function by address and jumping past retail code (the `onEnterVs`+0x3B8 → 801BB738 jump, the A+B branches into `onExitVs`) becomes an ordinary `#ifdef TRAINING_BUILD` branch in `gmevent.c`, written against named locals.
- Patching the `modes[]` states pointer and building a scene list from raw words at runtime becomes editing or adding a `GameModeState` array in `src/melee/gm/gmevent.c`/`gmscdata.c`.
- Per-event `.dat` files loaded with m-ex (`MEX_LoadRelArchive`, `TM/<event>.dat`), the rtoc function table (`ASM/training-mode/Globals.s` 1298–1321) and MexTK's struct offsets become linked C (our drill table) with no relocatable archives.
- Spoofing the current-mode byte so the main menu opens on the ESS becomes a guarded change in `gmmenumode.c` `onEnter`, or setting the mode directly.
- The hidden A+B runback chord exists because UnclePunch had no restart item. We should give restart an explicit menu item.

### Recommendation

**For Game mode of its own (a "Later" item in `.scratch/priorities.md`, needs-triage, no spec yet; ADR-0003 names Event Match as an option):** take over `GM_EVENT` the way UnclePunch does. It is a proven route that needs no new main-menu art: the 1P menu's Event Match label, the ESS and the "back to the list after a run" routing are already there.
- In a training build, guard `gm_Mode_Event_States` and the four `onEnter*`/`onExit*` functions in `src/melee/gm/gmevent.c`. Use CSS type 0x17 (player + CPU), add a `GS_SSS` state so the stage can be picked (or set it from the drill), and build `StartMeleeData` from the drill table in place of `GmEvent.dat`.
- Either keep the ESS as the drill list (new names and descriptions; see [Menus](#menus) for the text hooks), or skip it and show our in-match drill-select.
- Start from UnclePunch's boot behaviour: boot to the main menu and open on the event list.
- Keep these as guarded branches, not address patches. A new `GameModeKind` plus a `modes[]` entry is possible in our toolchain, but it still needs a main-menu entry, which is the hard part ADR-0003 already flags.

**For drill-select's in-drill controls (`.scratch/drill-select/spec.md`):** follow UnclePunch's conventions, which players already know.
- Start from any human port opens the pause menu, and only Start closes it.
- B backs out of a submenu but does not unpause.
- The pausing port drives the menu.
- The game is frozen with the game's own pause flag, sound and HUD, with no retail pause UI ([Pause](#pause)).
- The menu ends with an exit item.

Two differences from UnclePunch are deliberate:
- Offer an explicit **Restart drill** item rather than a hidden A+B chord.
- Restart in place (our `start_drill` in `src/training/tech_chase.c`) rather than reloading the scene, as UnclePunch's C events do with savestates and auto-reset. Under Training Mode, `gm_Scene_Training_OnFrame` (gmvs.c ~1450) pauses with flag 2 (`DB_PAUSE_FLAG_TRAINING` in `tech_chase.c`).

The spec asks for "the smallest guarded hook" to suppress Training Mode's own pause and menu. The best candidate is UnclePunch's `isDisablePause` equivalent: set `rules.disable_pausing` in the Training VS enter data (`src/melee/gm/gmtrainingmode.c` ~212, where `on_pause_override` is set). That stops `gm_DoPauseChecksAndRoutine` and `gm_DoUnpauseChecksAndRoutine` from running, and then we own Start. Check in a scenario that Training's menu has no other trigger. **Quit to character select** maps to ending the match the way UnclePunch's Exit does (`fn_8016C7F0`), or with `gm_8016B328` (sets `terminate_match`), which retail's event fail paths in `gmevent.c` use. Check which `onExitVs`-equivalent route Training Mode takes back to its CSS.

## Saving settings and progress

<!-- ticket 05 -->

**In short:** UnclePunch has no save system of its own for settings or progress. It parks a few bytes of its own in unused space inside Melee's normal save data and reuses Event Match's save fields for its events, then lets the game save as usual. The only files it writes itself are Lab input recordings, saved as separate memory card files. Paths below are relative to `ref/Training-Mode/` (UnclePunch) and to the repo root (our decomp).

### How Melee saves (our decomp)

- The whole game state hangs off one pointer, `gmMainLib_804D3EE0` (`src/melee/gm/gmmain_lib.h:167`, `.sdata` 0x804D3EE0). With Melee's r13 base that is the `-0x77C0(r13)` UnclePunch reads everywhere (`ASM/training-mode/Globals.s:1263` `MemcardData`; `MexTK/include/offsets.h:6` `MEMCARD`). UnclePunch's `Memcard` struct (`MexTK/include/memcard.h:25`) is a hand-written overlay of our `struct gmm_x0` (`src/melee/gm/types.h`).
- Only `gmm_x0.thing` (`struct GmCardData`: `GmSaveData` plus the name tag banks) goes to the card. `lbCardGame_Init` points the save manifest at `gmMainLib_GetCardData()->save_data` and the name tag banks (`src/melee/lb/lbcardgame.c:331-344`, manifest at `:64-75`). `GmSaveData` is fixed at 0x1790 bytes (`src/melee/gm/types.h:336`) and sits at offset 0x1868 in `gmm_x0`, because `GameRules` is 0x18 bytes (`types.h:244`). The `0x1898` comment at `types.h:434` looks stale.
- Saving is "mark dirty, the game writes later": `lbCardGame_SaveChanges` sets `dirty` (`lbcardgame.c:253-258`), and `lb_8001CC84` / `lb_8001CDB4` start the write and step it through `lbCardNew_CompleteNextTask` (`lbcardgame.c:210-251`; the per-file write is `taskWrite`, `src/melee/lb/lbcardnew.c:665`). The game calls `lbCardGame_SaveChanges` at its own save points, for example on every main menu entry (`mnMain_Scene_OnEnter`, `src/melee/mn/mnmain.c:2964`), on leaving the memory card scene (`gm_Scene_MemCard_OnFrame`, `src/melee/gm/gmscmemcard.c:318`) and when entering Training mode (`src/melee/gm/gmtrainingmode.c:86`).
- The boot-time card check and prompt is `gm_Scene_MemCard_OnFrame` (`src/melee/gm/gmscmemcard.c:268`). The save file name is `SuperSmashBros0110290334` (`lbcardgame.c:121`), its comment comes from `lb_8001C658` (`:86-106`) and its icon from `getCurrentIcon` (`:108-119`). Card files are matched by the disc's game code and company (`lbcardnew.c:745-761`, `:801-802`; `lb_8001C2D8` at `:1147`).

### What UnclePunch saves and where

1. **Global settings in unused save padding.** The OSD enable bitfield, the maximum number of OSD windows, the current event page and a "recommended OSDs" flag live at card-data offsets 0x1F24, 0x1F28, 0x1F29 and 0x1F2A (`ASM/training-mode/Globals.s:1165-1169`; `MexTK/include/memcard.h:2018-2021`, `TM_OSDEnabled`, `TM_OSDMax`, `TM_EventPage`, `TM_OSDRecommended`). In our decomp those bytes are `GmSaveData.padding_trophy_flags` (`src/melee/gm/types.h:333`: save-data offsets 0x6B6-0x6C3, absolute 0x1F1E-0x1F2B), the 14 bytes between the trophy flags and the per-fighter data. Nothing in our decomp uses them. They are inside `GmSaveData`, so the game's normal save writes them without UnclePunch doing anything.
2. **Event progress in Event Match's own fields** ([verdict](#verdict-on-the-event-match-hunch)). Event high scores use the vanilla event score array (card-data offset 0x1A70): UnclePunch hooks the getter and setter, `gmMainLib_8015CF5C` and `gmMainLib_8015CF70` (`src/melee/gm/gmmain_lib.c:344-352`), at 0x8015CF60 and 0x8015CF74 and adds a per-page base offset from a small table, so that events on all pages share the one array (`ASM/training-mode/Custom Events/Event Select Screen/Event High Scores/Save Event High Scores.asm:1,17-30`, `.../Load Event High Scores.asm:1`, table `Globals.s:1072-1082`). Our decomp names that array `x1A70[4]` plus `padding_x1A70[0xBC]` (`types.h:318-319`). Together they are 0xCC bytes, which is 51 event slots, read and written by `gmevent.c:694-706` and `mnevent.c:303`. The "event cleared" bits are `x1A68` (`types.h:317`; `gmMainLib_8015CEB4` / `gmMainLib_8015CEFC`, `gmmain_lib.c:330-342`). UnclePunch's save setup clears both (`Custom ESS Button Actions.asm:838-845`).
3. **Last selected event: in RAM only.** The C event loader reads the page from `TM_EventPage` and the event number from `EventBackup.event` (`patch/tmdata/source/events.c:1370-1374`), and the player's name tag from `EventBackup` (`events.c:1251-1265`). `EventBackup` is at offset 0x530 (`memcard.h:359`), which is our `struct EventData unk_530` (`types.h:359`, `:403`), the vanilla event menu's selection. It is outside `GmSaveData`, so only the page number persists across power-off.
4. **Per-event OSD overrides are not saved.** When an event is chosen, UnclePunch copies the OSD bitfield to a spare rtoc word (`.../Event Select Screen/Backup FDD Backup on Event Choose.asm:1,63-66`, hooked in `mnEvent_8024D864`). On every main menu load it copies it back (`.../Restore FDD Backup on Main Menu Load.asm:1-24`, hooked in `mnMain_Scene_OnEnter`). "FDD" is UnclePunch's own name, never expanded in the source; it also labels the branch that opens the OSD toggle screen (`OpenFDD` in `Custom ESS Button Actions.asm`), so here it means the OSD settings. An event's recommended OSDs therefore never reach the card: the main menu restores the player's own OSDs before its `lbCardGame_SaveChanges`.
5. **Lab menu options are session only.** The Lab's options are static `EventMenu` tables in RAM (`patch/events/lab/source/lab.c`, for example `:1555-1570`). The only card-data accesses in the C code are the two reads above (`events.c:1251`, `:1372`), and no UnclePunch code calls `lbCardGame_SaveChanges` (0x8001CE00, `GTME01.map:375`).
6. **Defaults on a new save or no card.** In `gm_Scene_MemCard_OnFrame` (hook at 0x801AF6F4), "no save" and "no card" both run `OnSaveCreate`. "No card" also jumps to 0x801B01AC to skip the prompt with saving disabled (`ASM/training-mode/Misc/On No Save.asm:1-37`; prompt-only variant `ASM/Additional Codes/Skip Memcard Prompt/Skip Memcard Prompt.asm`). `OnSaveCreate` unlocks all trophies and sets the default settings: max OSD windows, page 1, recommended OSDs, UCF on (`Globals.s:1124-1152`).
7. **A save file of its own, by game code.** The UnclePunch ISO is game code GTME (`GTME01.map`; the checks for 0x47544D45 "GTME" in the files below), so its save is a different card file from vanilla Melee's (GALE), even with the same file name. It renames the save comment to "Training Mode by UnclePunch" by hooking the `sprintf` in `lb_8001C658` (`ASM/training-mode/Misc/Save File Renaming.asm:1,67-129`) and picks its own icon in `getCurrentIcon` (`ASM/Additional Codes/Use Save Banner #1.asm:1-19`).
8. **Lab recordings as separate card files.** The Lab's "Export" option (`lab.c:1564`) writes a match setup, savestate and inputs (`RecordingSave`, `patch/events/lab/source/lab.h:276-282`). The data is lz77-compressed (`lab.c:1611` onward, `:5437`) and saved as its own file named `TMREC_<date>_<time>` (`lab.c:1593-1594`, `:6386`), with a screenshot banner, through the game's snapshot API (the camera-mode photo files). The write is `Memcard_CreateSnapshot` at `lab.c:6449`, which is our `lb_8001BB48` (`src/melee/lb/lbcardnew.c:952`). The export flow polls `Memcard_CheckStatus` (our `lbCardNew_CompleteNextTask`) and calls the SDK `CARD*` functions directly to probe, mount, check free blocks and delete (`lab.c:6365-6470`). Import lists files whose name starts with `TMREC` (`patch/events/lab/source/lab_css.c:475-508`) and loads one with `Memcard_LoadSnapshot`, our `lb_8001BF04` (`lab.c:5097-5250`, `lbcardnew.c:1049`). The work area comes from `Memcard_InitWorkArea`, our `lbCardNew_AllocWorkArea` (`lab.c:4489`; address map `MexTK/melee.link:354-413`).
9. **Not about settings: the save exploit installer.** The "Create Save" option on the event select screen (`Custom ESS Button Actions.asm:654-690`, marked "(Temp Disabled)") builds a vanilla-Melee save that carries a code-loading exploit (`:732-870`, exploit code from `:1052`). The patch that skips saving name tag chunks 2 and 3 when running on a GALE disc (`ASM/training-mode/Misc/Do Not Save Nametag Region to Memcard for GALE01.asm:1,66-82`, in `taskWrite`) protects that exploit payload, which sits in the name tag region. Neither applies to us.

### Carries over directly vs. needs translating

- **Directly:** the data model. The settings that must survive power-off are small, so they fit in a few bytes of `GmSaveData` and ride on the game's own saves (`lbCardGame_SaveChanges`), with no save UI of our own. Defaults go in at "new save" or "no card" time. Temporary per-drill overrides are restored before the next save point. Larger data, like recordings or custom drills, goes in a separate card file through the game's own card library (`lb_8001BB48` / `lb_8001BF04` / `lbCardNew_CompleteNextTask`).
- **Needs translating:**
  - UnclePunch reaches the padding through raw offsets and an overlay struct. We compile into the game, so we can name the bytes in `GmSaveData` itself (as a `TRAINING_BUILD` field in place of part of `padding_trophy_flags`) and keep `ASSERT_SIZE(GmSaveData, 0x1790)` holding. We must not grow `GmSaveData`: the card file layout is fixed by the manifest sizes (`lbcardgame.c:64-75`), and an existing save would no longer match. That is what makes the 14 padding bytes valuable. UnclePunch already uses 7 of them for its own build, but that does not limit us, because its save is a different file.
  - UnclePunch hooks instructions with injected branches (`On No Save.asm`, `Save File Renaming.asm`). We edit `gm_Scene_MemCard_OnFrame`, `lb_8001C658` and `getCurrentIcon` directly under `TRAINING_BUILD`.
  - **Not carried over:** a separate game code. Our training build runs as GALE01 and so shares vanilla Melee's save file with the player. Anything we write into `GmSaveData` sits in the player's real save. If we never use those padding bytes for anything else, vanilla ignores them, but that is an inference from the decomp (nothing references them), not something tested. It should be checked live before relying on it.

### Recommendation

For the later **Memory card saves** item (under "Later" in `.scratch/priorities.md`, needs-triage, spec not yet written):

- Keep small settings and progress (for example drill unlocks or best scores) in named fields carved out of `GmSaveData.padding_trophy_flags` under `TRAINING_BUILD`. Write them on the game's existing save points, set defaults where `gm_Scene_MemCard_OnFrame` handles "no save", and never change `sizeof(GmSaveData)`.
- Put anything bigger, such as custom drills, in our own card file through `lb_8001BB48` / `lb_8001BF04`, the way the Lab saves recordings.
- Do not copy UnclePunch's save code. Reimplement against the named decomp functions above.
- Check live that a vanilla save edited this way still loads in an unmodified game.

## Tech-chase events and vanish

<!-- ticket 06 -->

Paths below are relative to `ref/Training-Mode/` unless they start with `src/`, `docs/` or `.scratch/` (our repo). `Legacy.s` is `ASM/training-mode/Custom Events/Custom Event Code - Legacy.s`; `fighter.h` is `MexTK/include/fighter.h`.

### Short answer

- **No event drills punishing a CPU's tech.** None of UnclePunch's 23 events is a tech-chase drill in our sense (the player reads a CPU's [tech option](../../docs/glossary.md#glossary_tech_option) and punishes it). The event table is `patch/tmdata/source/events.c:1112-1163` (Minigames, General Tech and Spacie Tech pages). The nearest are:
  - **Training Lab** (`events.c:57-71`, code `patch/events/lab/source/lab.c`): free practice whose CPU menu has a "Tech Option" (Random, Neutral, Away, Towards, None) and a "Get Up Option" (Random, Stand, Away, Towards, Attack) (`lab.c:1270-1271`, `lab.c:1361-1380`). The player can tech-chase by hand, but there is no drop, no rep, no scoring and no punish window.
  - **Combo Training** (`events.c:216-260`, code `Legacy.s:5528` onward): the same CPU tech choice as a menu option, for teching out of combos (`Legacy.s:5590`, `Legacy.s:6045-6160`).
  - **Escape Sheik Techchase** (`events.c:907-921`, code `Legacy.s:10155` onward): the reverse of our drill. The *player* (Fox) is the one teching, and a scripted Sheik chases with jab and regrab. It shows early/late frames for the player's shine out of the tech states (`Legacy.s:10316-10400`). It does not force anyone's tech.
- **CPU techs are forced by writing the tech-input timers directly**, not by timing an L press. This is the approach ADR-0004 considered and rejected, but UnclePunch writes them every frame, which avoids ADR-0004's objection ("knowing the exact landing frame").
- **UnclePunch never hides a fighter.** Nothing in the training events sets the invisible flag (`0x221E` bit `0x80`, `fighter.h:2425`). The only related code is a codeset patch, `ASM/Additional Codes/Hide Nametag When Invisible.asm`, that makes the name tag and player indicator honour that flag. There is nothing like tech vanish to compare with.
- **Event Match: no bearing on tech chasing.** The Lab and Combo Training are entries in the patched Event Match list (`events.c:1128-1163`, read with the page saved on the memory card, `Legacy.s:115-130`), entered by the route in [Mode entry](#route-through-event-match). That route says nothing about how to drill tech chasing ([verdict](#verdict-on-the-event-match-hunch)).

### How the CPU is driven

**Silencing the AI.** The Lab sets the opponent's CPU kind to 15 every frame (`cpu_data->cpu.ai = 15`, commented "noact", `lab.c:2626-2627`), and back to 0 to let the game's AI recover (`lab.c:3318-3319`, init at `lab.c:6555`). The MexTK name `cpu.ai` is our `Fighter.cpu.kind` (`CpuFighter`, `src/melee/ft/types.h:1131-1138`; `CpuKind_15`, `src/melee/ft/forward.h:449`, unnamed). Legacy events instead zero the CPU command words `0x1A88`/`0x1A8C`/`0x1A90` each frame (`Legacy.s:12813-12824`; setting kind 15 there is commented out). In both, the event then writes `cpu.held` and `cpu.lstickX/Y` (`fighter.h:806-814`, at `fp+0x1A88`, our `fp->cpu.buttons` / `fp->cpu.lstick`). The Lab's CPU logic runs from its event think (`lab.c:6788-6790`), a GObj proc at the event's `callbackPriority` (3 for the Lab, `events.c:69`, attached at `events.c:1391-1395`).

Ours (ADR-0004) does the same job differently: `training_cpu_input` (`src/training/tech_chase.c:1291-1308`) runs inside `Fighter_procCpu` right after the CPU tick `ftCo_800B3900` (`src/melee/ft/fighter.c:1705-1713`), clears the CPU command and feeds our input. That needs no CPU-kind switch and no ordering assumption between GObj procs, but it does need the `TRAINING_BUILD` hook in the game's code, which UnclePunch's injection approach avoids by using a GObj proc instead.

**Forcing the tech.** In the Lab's `CPUSTATE_TECH` (entered when the CPU's hitlag ends while in hitstun, `lab.c:2800-2808`, `lab.c:2915-2921`), every frame (`lab.c:3066-3125`):

- `input.timer_LR` (`fp+0x680`, `fighter.h:1950`) is set to 0, meaning "L/R pressed this frame", or to -1 (255) for None.
- `input.sinceRapidLR` (`fp+0x684`, `fighter.h:1954`) is set to -1 (255), meaning "no tech lockout".
- `cpu.lstickX` is set to ±127 toward or away from the human (`Fighter_GetOpponentDir`, `lab.c:2369`), or 0 for Neutral.
- Random rerolls one of the four (including None) every frame (`lab.c:3090-3094`), so the tech that happens is whichever was rolled on the landing frame.

Combo Training does the same in ASM, plus holding L in the CPU buttons (`Legacy.s:6059-6160`: `0x680`=0 and `0x684`=0xFF each frame, `0x1A88`=0xC0, stick byte `0x1A8C`=±127). It waits out hitlag before writing the stick so as not to spoil the CPU's DI (`Legacy.s:6065-6067`).

In our decomp, `fp->x680` is the frames since the last L/R press and `fp->x684` is the gap between the last two presses, both kept by `Fighter_procInput` (`src/melee/ft/fighter.c:2114-2119`). The game's tech check is `ftCo_800986B0` (`src/melee/ft/kinds/ftCommon/ftCo_DownAttack.c:87-96`: a tech if `x680 < p_ftCommonData->x250` and `x684 >= p_ftCommonData->x1C`), called from `ftCo_PassiveStand.c:25`, `ftCo_PassiveWall.c:39` and `ftCo_PassiveCeil.c:23`. So UnclePunch keeps that check true on every frame of the fall, and any landing techs. Ours instead predicts the landing (`predict_landing_frames`, `src/training/tech_chase.c:700-715`), presses L half a tech window early, and respects the lockout (`update_tech_press`, `tech_chase.c:719-733`; `feed_option_input`, `tech_chase.c:1234-1285`).

Trade-offs, for our use:

- UnclePunch's way is robust to timing. It needs no landing prediction and works for wall and ceiling techs and for landings the prediction gets wrong (platform edges, a hit during the fall).
- It also has costs. The CPU never shows the real tech lockout, so it cannot portray a player who mashed L too early. Writing `x680` each frame also feeds other readers of that field: `ftCo_Damage_CheckAirMotion` (`src/melee/ft/kinds/ftCommon/ftCo_Damage.c:472-498`) treats a recent L/R press while airborne as a reason to scale incoming knockback by `p_ftCommonData->x190` (`ftCo_Damage.c:321-326`; the value was not checked). A CPU kept at "L pressed this frame" while falling would take that scaling if the player hit it mid-fall. ADR-0004's reason to feed real input (the game's own landing logic sees exactly what a human produces) still holds.
- The Lab only forces the tech while the CPU is in hitstun. When hitstun ends it moves to its counter-action state (`lab.c:3070-3078`). A CPU still tumbling after hitstun, which is how our drills drop the opponent (from `DamageFall`, `tech_chase.c:726`), would not be forced to tech by the Lab's logic.

**Rolls and missed tech.** Like ours, UnclePunch holds the stick toward or away from the player and lets the game pick the forward or back roll. It reads the direction every frame (`lab.c:3100-3110`), which matches our finding that the direction must not be latched (`tech_chase.c:1246-1256`). Its Away/Towards are relative to the human, the same as our [tech in](../../docs/glossary.md#glossary_tech_in)/[tech away](../../docs/glossary.md#glossary_tech_away) naming (`src/training/tech_options.h:5-14`). For a [missed tech](../../docs/glossary.md#glossary_missed_tech), the Lab moves to `CPUSTATE_GETUP` on any down state (`lab.c:2673-2676`). On `DownWait` it then feeds the chosen getup every frame as CPU input: stick up for stand, stick ±127 for the rolls, A for getup attack (`lab.c:3127-3190`). That is ADR-0004's mechanism unchanged, and it already covers the getup attack and getup rolls that "Missed-tech getup options" in `.scratch/priorities.md` wants. Unlike ours (`MISSED_TECH_DOWN_TIME`, `src/training/tech_options.h:20-25`), it gets up on the first `DownWait` frame with no lie-down beat.

### Hiding fighters

No UnclePunch event hides a fighter, so tech vanish has no counterpart. The repo reads `0x221E` only in two non-training game modes (`ASM/Additional Codes/Game Modes/Tag Melee.asm:384`, `Chess Melee.asm:192`) and in the name-tag patch.

`Hide Nametag When Invisible.asm` is inserted at `0x802FCCD8`, which is inside our `fn_802FCC44` (`src/melee/if/ifnametag.c:205-240`), the per-frame proc that places each player's indicator and name tag. When the fighter's invisible bit is set (or Mewtwo is in action state `0xEC`), it branches to the game's own "hide" path at `0x802FCCC8`, which hides the indicator joint and moves the tag text off-screen (`ifnametag.c:218-224`). It is built into the codeset: `Build TM Codeset/codes.json:20-24` injects the whole `ASM` folder recursively, `Additional Codes` included. This matches the tech-vanish finding that the name tag has no invisible check (`.scratch/tech-vanish/issues/02-magnifier-and-name-tag-check.md:16`). We got away with it because only human slots have a tag and the opponent's indicator is gone by mid-vanish. Our hide sets the same invisible flag plus the no-shadow bit `x221E_b1` every frame (`set_opponent_hidden`, `src/training/tech_chase.c:886-890`), as `.scratch/tech-vanish/spec.md` decided.

### What carries over and what needs translating

Carries over directly (facts about the game, reimplemented in our C against named fields):

- Holding the roll stick relative to the player, re-read every frame. We already do this.
- Feeding getup options (stand, rolls, attack) as CPU stick and button input on `DownWait`. This fits `feed_option_input`'s `TechOption_Miss` case as more cases.
- Hiding the indicator and name tag through the invisible bit, if a drill ever shows the opponent's indicator during a vanish. In our build this is a one-line `fp->invisible` check in `fn_802FCC44` behind `TRAINING_BUILD`, not a branch patch.
- The meaning of `fp->x680`/`fp->x684` and the `ftCo_800986B0` tech check, for any future option that writes the timers (for example, to portray tech lockout on purpose).

Needs translating from the injection toolchain:

- **Where the CPU logic runs.** UnclePunch attaches a GObj proc from a separately loaded event file (`MEX_LoadRelArchive`, `events.c:1387`) and relies on proc ordering plus CPU kind 15. We call ours from a `TRAINING_BUILD` hook in `Fighter_procCpu`, which runs after the CPU tick. Copying UnclePunch's proc-based ordering would be a step back.
- **Raw offsets to named fields.** `0x680`, `0x684`, `0x1A88`, `0x1A8C` and `0x221E` in the ASM, and MexTK's `timer_LR`, `sinceRapidLR`, `cpu.held`, `cpu.ai` and `invisible`, become `fp->x680`, `fp->x684`, `fp->cpu.buttons`, `fp->cpu.lstick`, `fp->cpu.kind` and `fp->invisible` in `src/melee/ft/types.h`.
- **Address patches to source edits.** `Hide Nametag When Invisible.asm` is an inject-at-address branch. We would edit `ifnametag.c` directly.

### Recommendation

- **Tech-options ([`.scratch/tech-options/spec.md`](../tech-options/spec.md); ADR-0004): keep feeding real input.** UnclePunch shows the timer-writing alternative works in practice and is simpler. But in its form it hides tech lockout and leaves the CPU marked as having pressed L/R, which feeds knockback scaling. The only reason to switch would be failures of our landing prediction. If drills with platforms or mid-fall hits show missed forced techs, the fallback is to write `fp->x680 = 0` and `fp->x684 = 255` on the frames inside our predicted window only, not for the whole fall. Record that in ADR-0004 as a known-working fallback, citing `lab.c:3119-3123`.
- **Missed-tech getup options (a "Later" item in `.scratch/priorities.md`, needs-triage, with no spec yet): take the Lab's getup handling as prior art when it is specced.** Getup attack is A on `DownWait` and the getup rolls are the stick ±127 relative to the player, all through `feed_option_input`. Our `MISSED_TECH_DOWN_TIME` lie-down beat stays, since UnclePunch's instant getup is less like a real player.
- **Tech-vanish ([`.scratch/tech-vanish/spec.md`](../tech-vanish/spec.md)): no change needed now.** UnclePunch has nothing to borrow for hiding fighters. If a later drill shows the opponent's player indicator or a name tag (for example, a human-controlled opponent in custom drills), add the invisible check to `fn_802FCC44`, as UnclePunch's nametag patch does. Add this to the tech-vanish spec's out-of-scope notes so it is not rediscovered.
