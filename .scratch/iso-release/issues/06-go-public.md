# 06: Make the repo public and publish v0.1.0

**What to build:** Repo-settings and release steps only the developer can take ([spec](../spec.md)): make `thestillheron/ssbm-training` public, turn on Issues, run `dev.py release`, and upload `rtc-training.zip` to a GitHub release tagged `v0.1.0`. Agents keep using `.scratch/` for their own work (`docs/agents/issue-tracker.md` is unchanged: `gh` still defaults to upstream).

**Blocked by:** 02, 03, 05

**Status:** ready-for-human

- [ ] Repo is public, Issues are on.
- [ ] Release `v0.1.0` has `rtc-training.zip` attached, and the README's download link reaches it.

**Note:** `orig/GALE01/` was re-extracted from a Redump-verified 1.02 ISO (the earlier NKit image failed verification, see ticket 02), and `dev.py release` now passes verification and writes `build/release/rtc-training.zip`; its patch applied to the vanilla ISO gives a byte-identical `rtc-training.iso`. The zip is ready to upload once the repo is public.
