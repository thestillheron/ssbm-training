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

@section glossary_dst dst
Short for "destination."

@section glossary_dyn dyn
Short for "dynamic."

@section glossary_ecb ECB
Stands for "environmental collision box."

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

@section glossary_it it
Short for "item."

@section glossary_kb kb
Short for "knockback."

@section glossary_layout layout
Where a @ref glossary_drill "drill" happens: the landing point (a spot on a floor or platform), the drop heights above it that the opponent falls from in tumble, and the player's start relative to the landing point. One drop height means every @ref glossary_rep "rep" has the same timing. Avoid "spawn config" or "positions".

@section glossary_lstick lstick
Short for "left stick," the gray analog stick on the GameCube controller.

@section glossary_lr lr
Stands for "left/right." Synonymous with "facing direction."

@section glossary_mag mag
Short for "magnitude."

@section glossary_matching_build matching build
The build of this repo whose `main.dol` is byte-identical to the original game, with no training features. Not to be confused with the @ref glossary_training_build "training build".

@section glossary_min min
Short for "minimum."

@section glossary_missed_tech missed tech
The @ref glossary_tech_option "tech option" where the opponent lands without teching and lies on the ground before getting up. Avoid "no tech" or "knockdown".

@section glossary_mul mul
Short for "multiplier" or "multiply."

@section glossary_mv mv
Stands for "motion variables." Used by #HSD_GObj::user_data structs (such as #Fighter::mv) to store polymorphic variables specific to an #MotionState.

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
The frames from the opponent landing to its first actionable frame. A hit or grab that connects in the punish window makes the @ref glossary_rep "rep" a success; once it is actionable, the rep is a failure.

@section glossary_rad rad
Short for "radians."

@section glossary_rep rep
One cycle of a @ref glossary_drill "drill": the opponent drops in tumble, lands, carries out its @ref glossary_tech_option "tech option", and the rep ends as a success or failure. A rep is void, and doesn't count, if the player hits the opponent before it lands. Avoid "round", "attempt" or "instance".

@section glossary_rep_log rep log
The text record the @ref glossary_training_build "training build" writes when @ref glossary_authoring_tools "authoring tools" are on: one line per @ref glossary_rep "rep" event, such as the tech option chosen, the frames of the @ref glossary_punish_window "punish window" and the outcome. It lets a rep be checked as text instead of from screenshots.

@section glossary_rot rot
Short for "rotation."

@section glossary_scenario scenario
A committed, named input script that takes the training build from boot to a known game state, optionally taking screenshots along the way. A scenario is usually piloting that has been written down. Avoid "macro" or "pre-canned results": the scenario is the script, not the screenshots it produces.

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
The part of the @ref glossary_punish_window "punish window" in which the opponent can be hit or grabbed, i.e. after any tech intangibility has ended. A tech's vulnerable frames are short and come at the end; a @ref glossary_missed_tech "missed tech" is vulnerable almost throughout.
@section glossary_vic vic
Short for "victim."

@section glossary_yakumono yakumono
Japanese for "punctuation marks" or special characters. Might just refer to "symbols" as in named members within an archive file. Used by #StageInfo::yakumono_param.

@section glossary_zako zako
Zako (雑魚) is Japanese for "trash mob" in video games, literally "small fish." In Melee, this is used to refer to Fighting Wire Frames and possibly other enemies.
