<p align="center"><img src="../assets/banner/banner.png" alt="RTC Training" width="576"></p>

# RTC Training

A Super Smash Bros. Melee training disc for practising reaction tech chasing, made from your own copy of Melee and played in Dolphin.

**[Download the latest release](https://github.com/thestillheron/ssbm-training/releases/latest)** (`rtc-training.zip`)

## What's in it

- **RTC 101**: a curriculum of drills that teaches reaction tech chasing step by step.
- **Custom Drill**: choose a drill's settings and stage yourself and practise it for as long as you like.
- **Drill history**: every attempt at an RTC 101 drill is scored and kept, and you can see a graph of your scores over time (press X on a drill in its event list).

RTC Training keeps **its own save**. Dolphin treats it as a separate game with its own game ID (`GTCE01`), so it never reads or overwrites your vanilla Melee save.

It is **tested on Dolphin**. It hasn't been tested on a real GameCube or Wii (Nintendont, Swiss), so it may or may not work there.

## What you need

- [Dolphin](https://dolphin-emu.org/), with a controller set up for Port 1.
- Your own copy of **Super Smash Bros. Melee, NTSC (US) version 1.02**, as a plain `.iso` file. The download doesn't contain the game; it's a patch that turns your ISO into RTC Training.

Nothing else is supported: PAL and Japanese discs, other revisions, already-modded ISOs and **NKit images** (`.nkit.iso`, or an RVZ made from one) won't work, because the patch only applies to a clean, full 1.02 dump. With any of those, the patch script stops with `ERROR: The ISO is not a valid v1.02 NTSC melee iso`.

### If your Melee is an RVZ

Convert it to ISO in Dolphin first (this is lossless): right-click the game in Dolphin's game list, choose **Convert File...**, set **Format** to **ISO** and click **Convert...**. Use the `.iso` it writes in the steps below.

## Making the RTC Training ISO

Download `rtc-training.zip` from the [latest release](https://github.com/thestillheron/ssbm-training/releases/latest) and unzip it. You get a folder called `rtc-training`. Your original ISO is only read, never changed.

### Windows

1. Drag your Melee ISO onto **`DRAG VANILLA MELEE HERE.bat`** in the `rtc-training` folder.
2. When the window says `rtc-training.iso has been successfully created!`, press any key to close it.

`rtc-training.iso` is now in the `rtc-training` folder.

### macOS and Linux

1. Install xdelta if you don't have it:
   - macOS: `brew install xdelta` (with [Homebrew](https://brew.sh/))
   - Debian or Ubuntu: `sudo apt install xdelta3`
   - Other distributions: the `xdelta3` (or `xdelta`) package from your package manager
2. In a terminal, run the script with the path to your Melee ISO:

   ```sh
   cd path/to/rtc-training
   ./build_linux_and_mac.sh path/to/melee.iso
   ```

When it prints `rtc-training.iso has been successfully created`, `rtc-training.iso` is in the `rtc-training` folder. If the script says xdelta isn't installed, go back to step 1.

### Playing it

Open `rtc-training.iso` in Dolphin: drag it onto Dolphin's window, or add the `rtc-training` folder under **Config > Paths** so it appears in the game list as **RTC Training**, next to your vanilla Melee. Start it, press Start on the title screen, and you're at RTC Training's main menu: RTC 101, Custom Drill, Options and Data.

## Reporting bugs

Please [open an issue](https://github.com/thestillheron/ssbm-training/issues) saying what you did, what you expected and what happened. Include which release you're on (for example `v0.1.0`), your Dolphin version and your OS.

## Building from source

RTC Training is built on [doldecomp/melee](https://github.com/doldecomp/melee), the matching decompilation of Melee, with the training features added on top. To build it yourself you need your own Melee disc image and the toolchain that `python dev.py setup` downloads; `python dev.py iso` then builds the RTC Training ISO. See the [developer workflow](../docs/developer-workflow.md) for the rest, including how to make a release.

## Credits

See [credits](../credits.md), including UnclePunch, whose training tools inspired this, and Alex Harrison's [Training Mode - Community Edition](https://github.com/AlexanderHarrison/TrainingMode-CommunityEdition), whose release model this follows.
