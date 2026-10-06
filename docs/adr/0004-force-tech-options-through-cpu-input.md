# Force tech options by feeding the opponent controller input

The opponent's [tech option](../glossary.md#glossary_tech_option) is produced by silencing the game's CPU AI and supplying the opponent's controller input ourselves each frame: L/R pressed inside the tech window before landing, the stick held toward or away from the player for a roll, or nothing for a [missed tech](../glossary.md#glossary_missed_tech). The game's own landing logic then picks the state, so sounds, the tech flash, face-up/face-down variants and every character's frame data behave exactly as they do against a human. It costs one guarded hook where CPU input is read, and the same mechanism later drives getup options and drift.

## Considered Options

- **Calling the tech-state entry functions directly**: skips the input timing, but the tech-roll entry is `static` (it would need an upstream wrapper or copied sound/effect calls) and it bypasses the landing decision, so it can drift from real behaviour.
- **Writing the fighter's tech-input timer (`x680`/`x684`) just before landing**: works, but depends on undocumented fields and on knowing the exact landing frame.
