# 01: History graph for this session (dots)

**What to build:** X on a curriculum drill's event in the event list opens its history graph full screen, built from this session's attempts. Each drill record gets a stable drill ID that history is keyed by, so reordering or retuning drills keeps it. An attempt is recorded, in RAM only, when its drill summary opens. Nothing is recorded for an attempt left early (Restart drill, Change characters, Back to event list, or Edit layout restarting), for the Custom drill, for an attempt played with an edited layout, or for test drills.

The graph shows the drill name and total attempts, the y axis labelled at 0, the pass mark and the top (0 to the drill's length for a rep-count drill; 0 to the best score so far, rounded up, for a timed drill), the x axis labelled at its first and last attempt numbers, a dot per attempt, and the pass-mark line in a contrasting colour (none for a drill without a pass mark). With no attempts it shows "No attempts yet". B or X closes it, back on the same drill. The description panel shows "X: History" on curriculum drills, not on Custom. Dots, axes and the pass-mark line need only the build's existing rectangles and text; the line between dots is 02 ([spec](../spec.md)).

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] A scenario plays a short drill to its summary twice, restarts a third attempt mid-drill, returns to the event list and presses X: the graph shows two dots at the summaries' scores and the pass-mark line (screenshot).
- [x] X on a drill with no attempts shows "No attempts yet"; X on Custom does nothing and Custom has no hint.
- [x] B and X both close the graph with the cursor still on that drill.
- [x] An attempt played after Edit layout changed the layout is not recorded.
- [x] Every drill in the drill table has a distinct stable ID, checked at build or boot.
