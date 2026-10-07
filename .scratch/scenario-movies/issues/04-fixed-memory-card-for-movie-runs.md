# 04: Fixed memory card for movie runs

**What to build:** every movie run of `dev.py run --scenario` starts from the same memory card contents, so the same build and the same scenario always produce the same input timing. Today the movie plays against the developer's real slot-A card, which Melee writes at each main menu. Ticket 03's live probing found main-menu input acceptance varying between runs. The suspected cause is lag frames during the card save, whose length depends on the card's contents. Dolphin consumes movie entries only on frames where the game polls, so varying lag shifts every later input.

Movie runs use a **snapshot** of the developer's card (the save with every character and stage unlocked). The snapshot is taken from the current slot-A card the first time it's needed, or by explicit request, and kept outside version control. Before each movie launch, dev.py copies the snapshot to a throwaway run copy and points Dolphin's slot A at that copy for this launch only (a command-line config override, not an edit to the developer's Dolphin config). The developer's real card is never written by movie runs. `--live` runs keep using the real card as today. See `../spec.md` and the research notes the orchestrator passes along.

First, confirm the diagnosis live: launch a movie run with Dolphin's frame-counter and lag overlays (`-C Dolphin.Movie.ShowFrameCount=True -C Dolphin.Movie.ShowLag=True`) and measure the lag frames around the main menu across several runs, with the real card, then with the fixed snapshot. Record the numbers in this ticket. If lag still varies with a fixed card, stop and report rather than building on a wrong diagnosis.

**Blocked by:** 02 (Play the movie in `run --scenario`)

**Status:** resolved

- [x] Lag frames around the main menu measured over several runs, with the real card and with a fixed card; findings recorded here
- [x] Movie runs use a fresh copy of a fixed card snapshot in slot A; the developer's real card is unchanged after a movie run (checked by hash)
- [x] The snapshot is created from the current slot-A card when missing, with a printed message saying where it is and how to refresh it; it's never committed
- [x] A clear error if slot A isn't a raw memory card file that can be snapshotted (e.g. a GCI folder or no card)
- [x] `--live` runs are unaffected
- [x] CLI-seam tests cover snapshot creation, refresh, the run copy and the launch override (fake Dolphin, as the existing run tests do); `dev.py check` passes
- [x] Developer workflow doc explains the snapshot and how to refresh it after unlocking things

## Findings (measured live on Dolphin 2609, training build at 2dbc9d60c)

Method: a probe movie (`start:2` at frames 330 and 392, then main-menu presses), launched through `dev.py run --scenario` with Dolphin's movie overlay on (`-C Dolphin.General.ShowFrameCount=True -C Dolphin.General.ShowLag=True -C Dolphin.Movie.ShowMovieWindow=True`; on 2609 the keys are in `General`, not `Movie`, and the overlay only shows with `ShowMovieWindow`). The game window was screen-captured every 40 ms and the overlay read off the captures. To keep the developer's card untouched, "real card" runs used one copy of it that every run kept writing to (exactly what the real card went through), and "fixed card" runs a fresh copy of the same starting card each time.

| | runs | Dolphin lag count | inputs consumed vs 2 x frame | first main-menu frame drawn (field) | 9 presses at movie frames 610..658 |
|---|---|---|---|---|---|
| evolving copy of the real card | 6 (overlay) / 4 (presses) | 1, constant all run | 36 short, constant from boot to movie end | 639-641 | cursor on Data every run |
| fresh copy of a fixed card | 3 (overlay), plus 12 with the built feature (single-press probes below) | 1, constant all run | 36 short, constant | 639-642 | cursor on Data in all 4 runs with these presses |

- **The diagnosis isn't confirmed.** Lag doesn't vary with either card: Dolphin's lag count is 1 (one boot field with no poll) and never moves, the card save at the main menu adds no lag frames, and the 36-entry boot shortfall (the once-per-field polling before Melee's pad init, about 18 frames) is the same in every run. The evolving card gave the same main-menu timing and the same press result in every run. So whatever ticket 03 saw varying, the memory card at the main menu wasn't it on these runs. The fixed card is still worth having: the developer's card is never written by movie runs, and with a fixed start the card Melee writes back is byte-identical on every run (SHA-256 `2e7b025b...` after every fixed run, versus a different hash after every evolving run), so nothing in later saves (play time, counters, unlock notices) can drift.
- **Probable source of ticket 03's confusion:** from field 412 (the title `start`) to about field 640 Melee draws nothing new while it loads the main menu and saves to the card. Emulation runs at full speed but the picture stays frozen on the title screen for about 3.9 s, so a wall-clock shot taken then shows the title screen, even though the movie (and the game) have moved on.
- **Movie frame vs field:** the overlay's frame (VI field) runs 18 ahead of the scenario frame after boot (36 entries short), a fixed offset.
- **Main menu input (fixed card, movie frames from power-on, `start:2` at 330 and 392):** a 2-frame `stick-down` starting at frame 636 or earlier is ignored; one starting at frame 637 moves the cursor (3 runs each at 636 and 637, all the same). So the main menu first reads input at movie frame 638. Presses every 6 frames from 640 all register.
- Real card SHA-256 `9314a305...` before and after all the runs above. Backup kept in the session scratchpad.

Built: `run --scenario` snapshots slot A to `build/memcard/snapshot.USA.raw` on first need (printing the path and `python dev.py snapshot-card` to refresh), copies it to `build/memcard/run.USA.raw` before each launch, and passes `-C Dolphin.Core.SlotA=1 -C Dolphin.Core.MemcardAPath=<run copy>`. The run copy is named with `.USA`, because Dolphin inserts a region into a card name without one and would otherwise use a different, blank file. Slot A that isn't an existing raw card (GCI folder, none, missing file, no Dolphin.ini) fails before the build. `--live` is unchanged. Tests: `tools/dev_tests/test_memcard.py`. The fake Dolphin in the run tests now records its arguments through Python, because cmd split `-C Key=Value` at the `=`.
