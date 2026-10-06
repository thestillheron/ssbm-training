# Tech-chase drills start inside vanilla Training Mode

Tech-chase [drills](../glossary.md#glossary_drill) first run inside the game's own 1P Training Mode. We reuse its character select, stage select and match setup, take over its CPU, and draw our own text menus in-match. That gets a playable [rep](../glossary.md#glossary_rep) with a couple of one-line hooks and no menu art, so the [curriculum](../glossary.md#glossary_curriculum) can be tested in play before we know what its menus need. Later the drills move to a game mode of their own with a drill-select screen, entered from the 1P menu (the menu has an unused hidden slot, or we replace Event Match as UnclePunch does). Until then, expect Training Mode behaviour (its own pause menu, CPU settings) to leak into drills and need suppressing.

## Considered Options

- **A game mode of our own from the start**: the right end state, but it needs an upstream scene-table edit, a new main-menu label (the labels are 3D models and textures, so this means art or a text overlay) and a drill browser before the first rep can be played.
