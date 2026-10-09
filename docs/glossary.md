@page glossary Glossary

@section glossary_aabb AABB
Stands for "axis-aligned bounding box."

@section glossary_accel accel
Short for "acceleration."

@section glossary_amb amb
Short for "ambient."

@section glossary_atk atk
Short for "attack."

@section glossary_authoring_tools authoring tools
Features for designing @ref glossary_curriculum "curriculum" drills, such as printing a drill's layout, compiled into the @ref glossary_training_build "training build" only on request. Players never see them. Avoid "developer build" or "debug build".

@section glossary_cam cam
Short for "camera."

@section glossary_cb cb
Short for "callback."

@section glossary_chan chan
Short for "channel."

@section glossary_coll coll
Short for "collision(s)."

@section glossary_cstick cstick
Short for "camera stick," the yellow analog stick on the right side of the GameCube controller.

@section glossary_cur cur
Short for "current" or "cursor."

@section glossary_curriculum curriculum
The fixed, ordered set of @ref glossary_drill "drills" designed to teach tech chasing step by step, from one landing timing and one @ref glossary_tech_option "tech option" up to mixed timing and a random choice of options. Avoid "lesson plan" or "course".

@section glossary_custom_drill custom drill
A @ref glossary_drill "drill" whose settings the player chooses, as opposed to one from the @ref glossary_curriculum "curriculum".

@section glossary_deg deg
Short for "degrees."

@section glossary_dir dir
Short for "direction."

@section glossary_div div
Short for "divisor" or "divide."

@section glossary_dmg dmg
Short for "damage."

@section glossary_drill drill
One configured tech-chasing activity, played as a series of @ref glossary_rep "reps" for a set count or time: the stage, where the opponent drops from and from what heights, where the player starts, which @ref glossary_tech_option "tech options" the opponent picks from, and whether it vanishes mid-tech. Avoid "mini game", "activity" or "exercise".

@section glossary_drill_chooser drill chooser
The temporary authoring-build control (D-pad right, in Training Mode, until drill-select makes each drill an Event Match event and removes it) that switches to the next @ref glossary_drill "drill" in the @ref glossary_drill_table "drill table" that fits the stage in play, restarting from rep 1 with a clean score and clock. It also lists the drills that need another stage. Avoid "drill picker".

@section glossary_drill_summary drill summary
The screen shown when a @ref glossary_drill "drill" ends: its name, the score, the @ref glossary_pass_mark "pass mark", pass or fail, and for each @ref glossary_tech_option "tech option" the opponent did (not the one the drill aimed for) its success rate and how many of its reps were early or late (their @ref glossary_timing "timing"). Press A to restart the drill.

@section glossary_drill_table drill table
The const table of @ref glossary_drill "drills" in `src/training/drill.c`: the @ref glossary_curriculum "curriculum", plus test drills in authoring builds. The rep loop runs one entry, the current drill.

@section glossary_dst dst
Short for "destination."

@section glossary_dyn dyn
Short for "dynamic."

@section glossary_ecb ECB
Stands for "environmental collision box."

@section glossary_event_list event list
The Event Match select screen, which in the @ref glossary_training_build "training build" lists the @ref glossary_drill "drills" in @ref glossary_curriculum "curriculum" order, one event per drill, instead of the vanilla events (ADR-0005). Avoid "drill menu" or "ESS" outside code.

@section glossary_ft ft
Short for "fighter."

@section glossary_gr gr
Short for "ground."

@section glossary_grav grav
Short for "gravity."

@section glossary_hatena hatena
Japanese for "question [mark]."

@section glossary_id id
Short for "identifier." Not an acronym.

@section glossary_idx idx
Short for "index."

@section glossary_init init
Short for "initial" or any form of the verb "initialize."

@section glossary_invalid_layout invalid layout
A @ref glossary_layout "layout" that cannot run on the stage in play: no floor below the landing point or the player's start, or no usable drop heights. It is reported in the @ref glossary_rep_log "rep log" and on screen, and the @ref glossary_drill "drill" runs no reps. A stage that no drill in the @ref glossary_drill_table "drill table" fits is reported the same way (`no_drill_for_stage`).

@section glossary_it it
Short for "item."

@section glossary_kb kb
Short for "knockback."

@section glossary_layout layout
Where a @ref glossary_drill "drill" happens: the landing point (a spot on a floor or platform), the drop heights above it that the opponent falls from in tumble (a list of heights picked at random, or a range with a uniform pick), and the player's start: an x offset from the point beside the opponent and a y offset from the landing surface, snapped to the floor below (on a platform landing, 0 puts the player on the floor beneath it). One drop height means every @ref glossary_rep "rep" has the same timing. Avoid "spawn config" or "positions".

@section glossary_lstick lstick
Short for "left stick," the gray analog stick on the GameCube controller.

@section glossary_lr lr
Stands for "left/right." Synonymous with "facing direction."

@section glossary_mag mag
Short for "magnitude."

@section glossary_matching_build matching build
The build of this repo whose `main.dol` is byte-identical to the original game, with no training features. Not to be confused with the @ref glossary_training_build "training build".

@section glossary_memory_card_snapshot memory card snapshot
A copy of the developer's slot A memory card (the save with every character and stage unlocked) that every @ref glossary_movie "movie" run plays against, kept in gitignored `build/memcard/` and never committed. `dev.py` takes it the first time a movie run needs it; `dev.py snapshot-card` refreshes it. Each run plays against a fresh @ref glossary_run_copy "run copy" of it, so the real card is never written and every run starts from the same card. Avoid "fixed card" or "card backup".

@section glossary_min min
Short for "minimum."

@section glossary_missed_tech missed tech
The @ref glossary_tech_option "tech option" where the opponent lands without teching and lies on the ground before getting up. Avoid "no tech" or "knockdown".

@section glossary_movie movie
A Dolphin input recording (`.dtm`) generated from a @ref glossary_scenario "scenario" by `dev.py`: the controller state for every input poll, so playback is frame-exact. A movie always starts at power-on and is build output, regenerated on every run and never committed; never record or play one from a savestate, which would restore the old build's code. Not to be confused with the game's own intro movie. The scenario is the script; the movie is what it compiles to.

@section glossary_mul mul
Short for "multiplier" or "multiply."

@section glossary_mv mv
Stands for "motion variables." Used by #HSD_GObj::user_data structs (such as #Fighter::mv) to store polymorphic variables specific to an #MotionState.

@section glossary_pass_mark pass mark
The number of successful @ref glossary_rep "reps" a @ref glossary_drill "drill" needs for the player to pass it, shown in the @ref glossary_drill_summary "drill summary".

@section glossary_phys phys
Short for "physics."

@section glossary_piloting piloting
Driving the running training build with controller input, step by step, to reach a game state. Avoid "playtesting" or "driving the emulator": the input goes to the game, and the point is to reach a state, not to play.

@section glossary_plat plat
Short for "platform."

@section glossary_ply ply
Short for "player."

@section glossary_pos pos
Short for "position."

@section glossary_prev prev
Short for "previous."

@section glossary_punish_window punish window
The frames from the opponent landing up to, but not including, its first actionable frame. A hit or grab that connects in the punish window makes the @ref glossary_rep "rep" a success; once it is actionable, the rep is a failure, so a hit on the actionable frame itself is late (by 0 frames).

@section glossary_rad rad
Short for "radians."

@section glossary_rep rep
One cycle of a @ref glossary_drill "drill": the opponent drops in tumble, lands, carries out its @ref glossary_tech_option "tech option", and the rep ends as a success (the player hit or grabbed it in the @ref glossary_punish_window "punish window") or a failure (it became actionable unpunished). A rep is void, and doesn't count, if the player hits the opponent before it lands. Each rep ends with a @ref glossary_rep_result "rep result". Avoid "round", "attempt" or "instance".

@section glossary_rep_log rep log
The text record the @ref glossary_training_build "training build" writes when @ref glossary_authoring_tools "authoring tools" are on: one line per @ref glossary_rep "rep" event, such as the tech option chosen, the frames of the @ref glossary_punish_window "punish window" and the outcome. It lets a rep be checked as text instead of from screenshots.

@section glossary_rep_result rep result
What the player is shown during the pause after a @ref glossary_rep "rep": what the opponent did (the tech option it carried out, or a wall or ceiling tech), the outcome (success, failure or void) and the rep's @ref glossary_timing "timing". Avoid "feedback" or "message" for the result itself.

@section glossary_rot rot
Short for "rotation."

@section glossary_run_copy run copy
The throwaway copy of the @ref glossary_memory_card_snapshot "memory card snapshot" (`build/memcard/run.USA.raw`) that `dev.py` makes before each @ref glossary_movie "movie" launch and puts in slot A for that launch only. Melee saves to it during the run; the next run replaces it.

@section glossary_scenario scenario
A committed, named input script that takes the training build from boot to a known game state, optionally taking screenshots along the way. A scenario is usually piloting that has been written down, and is played as a @ref glossary_movie "movie". Avoid "macro" or "pre-canned results": the scenario is the script, not the screenshots it produces or the movie it compiles to.

@section glossary_sfx sfx
Short for "sound effect(s)."

@section glossary_spd spd
Short for "speed."

@section glossary_sq sq
Short for "square."

@section glossary_src src
Short for "source."

@section glossary_sz sz
Short for "size."

@section glossary_tech_away tech away
The @ref glossary_tech_option "tech option" where the opponent rolls away from the player. Defined relative to the player, not the opponent's facing; avoid "tech back" or "roll back", which the game uses relative to facing.

@section glossary_tech_in tech in
The @ref glossary_tech_option "tech option" where the opponent rolls toward the player. Defined relative to the player, not the opponent's facing; avoid "tech forward" or "roll forward".

@section glossary_tech_in_place tech in place
The @ref glossary_tech_option "tech option" where the opponent techs without moving. Avoid "neutral tech" or "tech" on its own.

@section glossary_tech_option tech option
What the opponent does on landing: @ref glossary_tech_in_place "tech in place", @ref glossary_tech_in "tech in", @ref glossary_tech_away "tech away" or @ref glossary_missed_tech "missed tech". Wall and ceiling techs are not tech options.

@section glossary_tgt tgt
Short for "target."

@section glossary_timing timing
How a @ref glossary_rep "rep"'s punish lined up with the @ref glossary_punish_window "punish window", one class per rep:
- success k/n: the hit or grab landed on vulnerable frame k of the n there were (counted from the first @ref glossary_vulnerable_frames "vulnerable frame" to the last, from 1). Shown "Frame k/n".
- early: a failure in which the player had a hitbox or grab box out while the opponent was intangible in the window; N frames early is how long before the next vulnerable frame the last such frame was. Shown "Nf early".
- late: a failure whose hit or grab connected N frames after the opponent's first actionable frame, during the pause. Wins over early. Shown "Nf late".
- none: a failure with no attack out while the opponent was intangible and no late hit. Shown "No attack".
- void: a hit before the landing (a void rep). Shown "Early: hit before landing".

@section glossary_training_build training build
The game code plus this fork's training features, compiled without the byte-identical check so it can differ from the original. Avoid calling it the "modded" or "non-matching" build: upstream uses "non-matching" for code that is merely equivalent, and "debug" already names a different `configure.py` mode.

@section glossary_unk unk
Short for "unknown."

@section glossary_vec vec
Short for "vector."

@section glossary_veg veg
Short for "vegetable."

@section glossary_vel vel
Short for "velocity."

@section glossary_vtx vtx
Short for "vertex."

@section glossary_vulnerable_frames vulnerable frames
The part of the @ref glossary_punish_window "punish window" in which the opponent can be hit or grabbed, i.e. after any tech intangibility has ended. The actionable frame is not one of them: it closes the window. A tech's vulnerable frames are short and come at the end; a @ref glossary_missed_tech "missed tech" is vulnerable almost throughout.

@section glossary_vic vic
Short for "victim."

@section glossary_yakumono yakumono
Japanese for "punctuation marks" or special characters. Might just refer to "symbols" as in named members within an archive file. Used by #StageInfo::yakumono_param.

@section glossary_zako zako
Zako (雑魚) is Japanese for "trash mob" in video games, literally "small fish." In Melee, this is used to refer to Fighting Wire Frames and possibly other enemies.
