# Native toolchain behind one Python entry point

We develop on both macOS (Apple Silicon) and Windows. The build runs on each OS's native toolchain (Python + ninja everywhere, plus Wine on macOS to run the Windows-only MWCC compiler), and a single Python entry point gives both machines identical commands. It hides OS-specific details such as finding Wine and Dolphin.

## Considered Options

- **Nix** (what upstream CI uses): reproducible, but on Windows it needs WSL, where upstream notes objdiff loses filesystem notifications for auto-rebuilds.
- **Docker**: fine for building, but can't easily launch Dolphin's GUI, so play-testing would be native anyway and we'd maintain two environments.
